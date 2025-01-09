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
#include <stdio.h>
#include <stdint.h>
#include <gccore.h>
#include "mem.h"
#include "render.h"

void mem_checkalign(void* p, size_t alignment, char const* info) {
	if ((uintptr_t)p % alignment != 0) {
		printf("FATAL: Memory address %p not aligned to %uB", p, (uint32_t)alignment);
		if (info != NULL) printf(" (%s)", info);
		printf("\n");
		exit(1);
	}
}

/*
 * libogc2's SYS_Init() calls __lowmem_init(), which sets ArenaHi and ArenaLo
 * to hardcoded values in devkitPro's linker script. This value can be
 * overridden by assigning values to the weak symbols
 * (void*)__myArena1Lo/__myArena1Hi. Because SYS_Init() calls __SYS_PreInit()
 * prior to __lowmem_init(), we set the
 * __myArena1Lo/__myArena1Hi here to the values that are given by the apploader,
 * before it assumes to use the default values.
 */
void mem_preinit(void) {
	extern void* __myArena1Lo;
	extern void* __myArena1Hi;
	__myArena1Lo = *(void**)0x80000030; /* NULL with retail-compatible apploader.
	                                       Default value provided by linker script is OK. */
	__myArena1Hi = *(void**)0x80000034; /* Apploader sets this value to address of FST. */
}

struct MemoryLayout mem_init(size_t heapSize) {
	struct MemoryLayout mem;
	mem.renderXFB = SYS_AllocArenaMemHi(render_get_xfbsz(), 32);
	mem.renderFIFO = SYS_AllocArenaMemHi(render_get_fifosz(), 32);

	void* levelTop = SYS_GetArenaHi();
	mem.levelData = SYS_AllocArenaMemHi((uintptr_t)SYS_GetArenaHi() - (uintptr_t)SYS_GetArenaLo() - heapSize, 32);
	mem.levelCapacity = (uintptr_t)levelTop - (uintptr_t)mem.levelData;

	mem.heapCapacity = (uintptr_t)SYS_GetArenaHi() - (uintptr_t)SYS_GetArenaLo();

	return mem;
}
