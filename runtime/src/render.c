/*
ORCA
Copyright (C) 2024,2025 leonardus

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <string.h>
#include <gccore.h>
#include "mem.h"
#include "pak.h"

static float const VERY_FAR = 10E+18F; // Sufficiently large value safe for GameCube lighting hardware (<10e19)
static void*       currentXFB = NULL;
static Mtx         currentCamera;

static GXRModeObj* get_rmode(void) {
	static GXRModeObj* rmode = NULL;
	if (rmode == NULL) rmode = VIDEO_GetPreferredMode(NULL);
	return rmode;
};

size_t render_get_xfbsz(void) {
	return VIDEO_GetFrameBufferSize(get_rmode());
}

size_t render_get_fifosz(void) {
	return 0x20000; // 128KiB
}

static void set_xfb(void* xfb) {
	currentXFB = xfb;
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_Flush();
}

void render_init(void) {
#ifdef DEBUG
	if (g_XFB0 == NULL || g_FIFO == NULL) {
		printf("ERROR: Render memory not initialized\n");
		exit(1);
	}
	mem_checkalign(g_XFB0, 32, "XFB");
	mem_checkalign(g_FIFO, 32, "FIFO");
#endif
	GXRModeObj* const rmode = get_rmode();

	VIDEO_Init();
	VIDEO_Configure(rmode);
	VIDEO_SetBlack(true);
	set_xfb(g_XFB0);
	VIDEO_Flush();

	memset(g_FIFO, 0, render_get_fifosz()); // Clear FIFO so there's no garbage data present
	GX_Init(g_FIFO, render_get_fifosz());
	GX_SetDispCopyYScale(GX_GetYScaleFactor(rmode->efbHeight, rmode->xfbHeight));
	GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
	GX_SetDispCopyDst(rmode->fbWidth, rmode->xfbHeight);
#ifdef DEBUG
	GX_SetCopyClear((GXColor){255, 0, 255, 255}, GX_MAX_Z24); // Magenta
#else
	GX_SetCopyClear((GXColor){0, 0, 0, 0}, GX_MAX_Z24);
#endif
	GX_SetViewport(0.0F, 0.0F, rmode->fbWidth, rmode->xfbHeight, 0.0F, 1.0F);
	GX_SetCullMode(GX_CULL_FRONT);
	GX_SetClipMode(GX_CLIP_ENABLE);

	GX_SetNumTevStages(1);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);

	GX_SetNumChans(1);
	GX_SetChanAmbColor(GX_COLOR0A0, (GXColor){96, 96, 96, 255});
	GX_SetChanMatColor(GX_COLOR0A0, (GXColor){255, 255, 255, 255});

	GXLightObj sun;
	guVector   sunPos = {VERY_FAR, VERY_FAR, VERY_FAR};
	GX_InitLightColor(&sun, (GXColor){255, 255, 255, 255});
	GX_InitLightPos(&sun, sunPos.x, sunPos.y, sunPos.z);
	GX_LoadLightObj(&sun, GX_LIGHT0);

	guMtxIdentity(currentCamera);

	Mtx44 proj;
	guPerspective(proj, 70, (float)rmode->fbWidth / (float)rmode->xfbHeight, 1.0F, 10000.0F);
	GX_LoadProjectionMtx(proj, GX_PERSPECTIVE);
}

void render_ready(void) {
	/* Put some sensible data (clear color) in XFB before displaying anything */
	GX_CopyDisp(currentXFB, GX_TRUE);
	VIDEO_WaitVSync();
	VIDEO_SetBlack(false);
	VIDEO_Flush();
}

void render_set_camera(Mtx camera) {
	memcpy(currentCamera, camera, sizeof(Mtx));
}

static void send_corrected_color(struct Accessor* acr, uint16_t idx) {
	float*    rgba_f32 = (float*)(acr->buffer + (idx * acr->stride));
	uint16_t* rgba_u16 = (uint16_t*)(acr->buffer + (idx * acr->stride));
	switch (acr->componentType) {
	case COMPONENT_F32:
		switch (acr->elementType) {
		case ELEM_VEC4:
			GX_Color4u8(rgba_f32[0] * 255, rgba_f32[1] * 255, rgba_f32[2] * 255, rgba_f32[3] * 255);
			break;
		case ELEM_VEC3:
			GX_Color3u8(rgba_f32[0] * 255, rgba_f32[1] * 255, rgba_f32[2] * 255);
			break;
		default:
			printf("ERROR: Invalid COLOR_n element type: '%d'", acr->elementType);
			exit(1);
			break;
		}
		break;
	case COMPONENT_U16:
		switch (acr->elementType) {
		case ELEM_VEC4:
			GX_Color4u8(rgba_u16[0] >> 8, rgba_u16[1] >> 8, rgba_u16[2] >> 8, rgba_u16[3] >> 8);
			break;
		case ELEM_VEC3:
			GX_Color3u8(rgba_u16[0] >> 8, rgba_u16[1] >> 8, rgba_u16[2] >> 8);
			break;
		default:
			printf("ERROR: Invalid COLOR_n element type: '%d'", acr->elementType);
			exit(1);
			break;
		}
		break;
	default:
		printf("ERROR: Invalid COLOR_n component type: '%d'", acr->componentType);
		exit(1);
		break;
	}
}

static void draw_primitive(struct MeshPrimitive* const p) {
	/* 3.2.7.1 When positions are not specified, client implementations SHOULD skip primitive’s rendering  */
	if (p->attrPos == NULL) return;

	if (p->indices == NULL) {
		printf("Not yet implemented/%s:%u\n", __func__, __LINE__);
		exit(1);
		return;
	}

	GX_ClearVtxDesc();

	GX_SetVtxDesc(GX_VA_POS, GX_INDEX16);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, p->attrPos->componentType, 0);
	GX_SetArray(GX_VA_POS, p->attrPos->buffer, p->attrPos->stride);

	bool const hasNormal = p->attrNormal != NULL;
	bool const hasColor = p->attrColor != NULL;
	/*
	 * If color components are float or u16, they must be corrected at runtime to u8 and sent with GX_DIRECT; the
	 * only other valid component type for COLOR_n is u8, which can be sent with GX_INDEX16 as it's already the
	 * correct format
	 */
	bool const indexColor = hasColor && p->attrColor->componentType == COMPONENT_U8;
	bool const hasTexture = (p->material != NULL) && ((p->attrTexCoord0 != NULL) || (p->attrTexCoord1 != NULL));
	if (hasNormal) {
		GX_SetVtxDesc(GX_VA_NRM, GX_INDEX16);
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, p->attrNormal->componentType, 0);
		GX_SetArray(GX_VA_NRM, p->attrNormal->buffer, p->attrNormal->stride);
	}
	if (hasColor) {
		if (indexColor) {
			GX_SetVtxDesc(GX_VA_CLR0, GX_INDEX16);
			GX_SetArray(GX_VA_CLR0, p->attrColor->buffer, p->attrColor->stride);
		} else {
			GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
		}

		int const compCount = p->attrColor->elementType == ELEM_VEC4 ? GX_CLR_RGBA : GX_CLR_RGB;
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, compCount, GX_U8, 0);
	}
	if (hasTexture) {
		struct Accessor* const texCoord = p->attrTexCoord0 ? p->attrTexCoord0 : p->attrTexCoord1;
		GX_SetVtxDesc(GX_VA_TEX0, GX_INDEX16);
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, texCoord->componentType, 0);
		GX_SetArray(GX_VA_TEX0, texCoord->buffer, texCoord->stride);
		GX_LoadTexObj(p->material->texture, GX_TEXMAP0);
	}

	GX_SetChanCtrl(GX_COLOR0A0, GX_TRUE, GX_SRC_REG, hasColor ? GX_SRC_VTX : GX_SRC_REG, GX_LIGHT0, GX_DF_CLAMP,
	               GX_AF_NONE);

	GX_Begin(p->mode, GX_VTXFMT0, p->indices->count);
	for (size_t i = 0; i < p->indices->count; i++) {
		uint16_t const idx = ((uint16_t*)p->indices->buffer)[i];
		GX_Position1x16(idx);
		if (hasNormal) GX_Normal1x16(idx);
		if (indexColor) {
			GX_Color1x16(idx);
		} else if (hasColor) { // (&& !indexColor) Float or u16, components must be corrected and sent direct
			send_corrected_color(p->attrColor, idx);
		}
		if (hasTexture) GX_TexCoord1x16(idx);
	}
	GX_End();
}

static void draw_tree(struct Node* node, Mtx _parentM, struct Model* model) {
	Mtx parentM;
	if (_parentM != NULL) {
		memcpy(parentM, _parentM, sizeof(Mtx));
	} else {
		guMtxIdentity(parentM);
	}

	Mtx m;
	guMtxScale(m, node->scale.x, node->scale.y, node->scale.z);
	Mtx rot;
	guMtxQuat(rot, &node->rotation);
	guMtxConcat(rot, m, m);
	guMtxTransApply(m, m, node->translation.x, node->translation.y, node->translation.z);
	guMtxConcat(parentM, m, m);

	if (node->mesh != NULL) {
		Mtx mv;
		guMtxConcat(currentCamera, m, mv);
		GX_LoadPosMtxImm(mv, GX_PNMTX0);
		GX_LoadNrmMtxImm(mv, GX_PNMTX0);
		GX_SetCurrentMtx(GX_PNMTX0);

		for (size_t i = 0; i < node->mesh->numPrimitives; i++) {
			draw_primitive(model->primitives + node->mesh->primitivesIdxs[i]);
		}
	}

	for (size_t i = 0; i < node->numChildren; i++) {
		draw_tree(model->nodes + node->childrenIdxs[i], m, model);
	}
}

static void draw_scene(struct Scene* scene, struct Model* model) {
	for (size_t i = 0; i < scene->numNodes; i++) {
		struct Node* const n = model->nodes + scene->nodesIdxs[i];
		draw_tree(n, NULL, model);
	}
}

static void draw_model(struct Model* model) {
	for (size_t s = 0; s < model->numScenes; s++) {
		draw_scene(model->scenes + s, model);
	}
}

void render_tick(struct Model* model) {
	GX_CopyDisp(currentXFB, GX_TRUE);
	VIDEO_WaitVSync();

	if (model != NULL) {
		draw_model(model);
	}
}
