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

#include <gccore.h>
#include "mem.h"

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
}

void render_ready(void) {
	/* Put some sensible data (clear color) in XFB before displaying anything */
	GX_CopyDisp(g_XFB, GX_TRUE);
	VIDEO_WaitVSync();
	VIDEO_SetBlack(false);
	VIDEO_Flush();
}

void render_tick(void) {
	GX_CopyDisp(g_XFB, GX_TRUE);
	VIDEO_WaitVSync();
}
