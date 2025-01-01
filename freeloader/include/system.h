/*
 * system.h
 *
 * Copyright (C) 2005-2006 The GameCube Linux Team
 * Copyright (C) 2005,2006 Albert Herranz
 * Copyright (C) 2024 leonardus
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 */
#ifndef __SYSTEM_H
#define __SYSTEM_H

#define L1_CACHE_LINE_SIZE 32
#define LG_L1_CACHE_LINE_SIZE 5

#define GCN_VIDEO_LINES 480

#define GCN_RAM_SIZE (24 * 1024 * 1024)
#define GCN_TOP_OF_RAM (0x01200000) /* up to apploader code */

#define GCN_XFB_SIZE (2 * 640 * GCN_VIDEO_LINES)

// #define GCN_XFB_END             (GCN_TOP_OF_RAM-1)
// #define GCN_XFB_START           (GCN_XFB_END-GCN_XFB_SIZE+1)
#define GCN_XFB_START 0x00f00000
#define GCN_XFB_END (GCN_XFB_START + GCN_XFB_SIZE)

#ifndef __ASSEMBLY__

#define GCN_VI_TFBL ((void*)0xcc00201c)
#define GCN_VI_BFBL ((void*)0xcc002024)

#define FLIPPER_ICR ((void*)0xcc003000)

#define GCN_SI_C0OUTBUF ((void*)0xcc006400)
#define GCN_SI_SR ((void*)0xcc006438)

#define SICINBUFH(x) ((void*)(0xcc006404 + (x) * 12))
#define SICINBUFL(x) ((void*)(0xcc006408 + (x) * 12))

#define PAD_Y (1 << 27)
#define PAD_X (1 << 26)
#define PAD_B (1 << 25)
#define PAD_A (1 << 24)
#define PAD_Z (1 << 20)

extern void (*report)(char* text, ...);

extern void flush_dcache_range(void* start, void* stop);
extern void invalidate_dcache_range(void* start, void* stop);
extern void invalidate_icache_range(void* start, void* stop);

void* xmemcpy(void* dest, const void* src, int count);
int   xmemcmp(const void* cs, const void* ct, int count);
void* xmemset(void* s, int c, int count);

extern void rumble(int enable);
extern void rumble_on(void);
extern void panic(char* text);

#endif /* __ASSEMBLY__ */

#endif /* __SYSTEM_H */
