/*
ORCA
Copyright (C) 2024 leonardus

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
#include <gccore.h>
#include "mem.h"
#include "pak.h"

static void* g_XFB = NULL;

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
	g_XFB = xfb;
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_Flush();
}

void render_init(void* xfb, void* fifo) {
#ifdef DEBUG
	mem_checkalign(xfb, 32, "XFB");
	mem_checkalign(fifo, 32, "FIFO");
#endif
	GXRModeObj* const rmode = get_rmode();

	VIDEO_Init();
	VIDEO_Configure(rmode);
	VIDEO_SetBlack(true);
	set_xfb(xfb);
	VIDEO_Flush();

	memset(fifo, 0, render_get_fifosz()); // Clear FIFO so there's no garbage data present
	GX_Init(fifo, render_get_fifosz());
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

	static Mtx44 proj;
	guPerspective(proj, 90, (float)rmode->fbWidth / (float)rmode->xfbHeight, 1.0F, 10000.0F);
	GX_LoadProjectionMtx(proj, GX_PERSPECTIVE);
}

void render_ready(void) {
	/* Put some sensible data (clear color) in XFB before displaying anything */
	GX_CopyDisp(g_XFB, GX_TRUE);
	VIDEO_WaitVSync();
	VIDEO_SetBlack(false);
	VIDEO_Flush();
}

static void draw_primitive(struct MeshPrimitive* const p) {
	if (p->indices == NULL) {
		printf("Not yet implemented/%s:%u\n", __func__, __LINE__);
		exit(1);
		return;
	}

	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_INDEX16);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);

	size_t stride = 0;
	switch (p->attr_pos->component_type) {
	case GX_U8:
		stride = sizeof(uint8_t);
		break;
	case GX_S8:
		stride = sizeof(int8_t);
		break;
	case GX_U16:
		stride = sizeof(uint16_t);
		break;
	case GX_S16:
		stride = sizeof(int16_t);
		break;
	case GX_F32:
		stride = sizeof(float);
		break;
	}
	stride *= 3;

	GX_SetArray(GX_VA_POS, p->attr_pos->buffer, stride);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, p->attr_pos->component_type, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGB, GX_RGB8, 0);

	GX_Begin(p->mode, GX_VTXFMT0, p->indices->count);
	for (size_t i = 0; i < p->indices->count; i++) {
		GX_Position1x16(((uint16_t*)p->indices->buffer)[i]);
		GX_Color3u8(255, 255, 255);
	}
	GX_End();
}

static void draw_model(Mtx camera, struct Model* m) {
	for (size_t i = 0; i < m->node_table_count; i++) {
		struct Node* const n = &m->node_table[i];
		if (n->mesh == NULL) continue;
		Mtx mv;
		guMtxIdentity(mv);
		c_guMtxQuat(mv, (guQuaternion*)n->rotation);
		guMtxScaleApply(mv, mv, n->scale[0], n->scale[1], n->scale[2]);
		guMtxTransApply(mv, mv, n->translation[0], n->translation[1], n->translation[2]);
		guMtxConcat(camera, mv, mv);
		GX_LoadPosMtxImm(mv, GX_PNMTX0);
		GX_SetCurrentMtx(GX_PNMTX0);

		for (size_t j = 0; j < n->mesh->primitives_count; j++) {
			draw_primitive(&m->primitive_table[n->mesh->primitives[j]]);
		}
	}
}

void render_tick(Mtx camera, struct PAKHeader* pak) {
	if (pak != NULL) {
		for (size_t i = 0; i < pak->directory_count; i++) {
			struct DirectoryEntry* e = &pak->directory[i];
			if (e->type != ASSET_TYPE_MODEL) continue;
			draw_model(camera, e->offset);
		}
	}
	GX_CopyDisp(g_XFB, GX_TRUE);
	VIDEO_WaitVSync();
}
