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

static void* currentXFB = NULL;
static Mtx   currentCamera;

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
	GX_SetCullMode(GX_CULL_NONE);
	GX_SetClipMode(GX_CLIP_DISABLE);

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

static void draw_primitive(struct MeshPrimitive* const p) {
	if (p->attrPos != NULL && p->indices == NULL) {
		printf("Not yet implemented/%s:%u\n", __func__, __LINE__);
		exit(1);
		return;
	}
	if (p->attrPos == NULL) return;

	bool const             isTextured = p->material && (p->attrTexCoord0 || p->attrTexCoord1);
	struct Accessor* const texCoord = p->attrTexCoord0 ? p->attrTexCoord0 : p->attrTexCoord1;
	if (isTextured) GX_LoadTexObj(p->material->texture, GX_TEXMAP0);

	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_INDEX16);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	if (isTextured) GX_SetVtxDesc(GX_VA_TEX0, GX_INDEX16);

	GX_SetArray(GX_VA_POS, p->attrPos->buffer, p->attrPos->stride);
	if (isTextured) GX_SetArray(GX_VA_TEX0, texCoord->buffer, texCoord->stride);

	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, p->attrPos->componentType, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGB, GX_RGB8, 0);
	if (isTextured) GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, texCoord->componentType, 0);

	GX_Begin(p->mode, GX_VTXFMT0, p->indices->count);
	for (size_t i = 0; i < p->indices->count; i++) {
		GX_Position1x16(((uint16_t*)p->indices->buffer)[i]);
		GX_Color3u8(255, 255, 255);
		if (isTextured) GX_TexCoord1x16(((uint16_t*)p->indices->buffer)[i]);
	}
	GX_End();
}

static void draw_model(Mtx camera, struct Model* m) {
	for (size_t i = 0; i < m->numNodes; i++) {
		struct Node* const n = &m->nodes[i];
		if (n->mesh == NULL) continue;

		Mtx mv;
		guMtxIdentity(mv);
		c_guMtxQuat(mv, &n->rotation);
		guMtxScaleApply(mv, mv, n->scale.x, n->scale.y, n->scale.z);
		guMtxTransApply(mv, mv, n->translation.x, n->translation.y, n->translation.z);
		guMtxConcat(camera, mv, mv);
		GX_LoadPosMtxImm(mv, GX_PNMTX0);
		GX_SetCurrentMtx(GX_PNMTX0);

		for (size_t j = 0; j < n->mesh->numPrimitives; j++) {
			draw_primitive(&m->primitives[j]);
		}
	}
}

void render_tick(struct Model* model) {
	GX_CopyDisp(currentXFB, GX_TRUE);
	VIDEO_WaitVSync();
	if (model != NULL) {
		draw_model(currentCamera, model);
	}
}
