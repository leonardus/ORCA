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
#include <stdio.h>
#include <stdint.h>
#include <gccore.h>
#include "mem.h"
#include "render.h"

void*         g_XFB0;
void*         g_FIFO;
static void*  scratchLow;  // Address where scratch memory area starts
static void*  scratchNext; // Next free address in scratch memory
static size_t scratchCapacity;

void mem_checkOOM(void* p) {
	if (p == NULL) {
		printf("ORCA exhausted heap memory.\n");
		exit(1);
	}
}

void mem_checkalign(void* p, size_t alignment, char const* info) {
	if ((uintptr_t)p % alignment != 0) {
		printf("FATAL: Memory address %p not aligned to %uB", p, (uint32_t)alignment);
		if (info != NULL) printf(" (%s)", info);
		printf("\n");
		exit(1);
	}
}

void* mem_alloc_scratch(size_t n, size_t align) {
	uint8_t* addr = scratchNext;
	if (align != 0 && n != 0 && (uintptr_t)addr % align != 0) {
		addr += align - (uintptr_t)scratchNext % align;
	}

	scratchNext = addr + n;
	if ((uintptr_t)scratchNext > (uintptr_t)scratchLow + scratchCapacity) {
		printf("ERROR: Ran out of scratch memory\n");
		exit(1);
	}

	return addr;
}

void mem_reset_scratch(void) {
	scratchNext = scratchLow;
}

void mem_init(size_t heapSize) {
	static int init = 0;
	if (init != 0) return;
	init = 1;

	g_XFB0 = SYS_AllocArenaMemHi(render_get_xfbsz(), 32);
	g_FIFO = SYS_AllocArenaMemHi(render_get_fifosz(), 32);

	void* const scratchHigh = SYS_GetArenaHi();
	scratchLow = SYS_AllocArenaMemHi((uintptr_t)SYS_GetArenaHi() - (uintptr_t)SYS_GetArenaLo() - heapSize, 32);
	scratchNext = scratchLow;
	scratchCapacity = (uintptr_t)scratchHigh - (uintptr_t)scratchLow;
}
