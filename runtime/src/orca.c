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

#include <stdio.h>
#include <gccore.h>
#include "fst.h"
#include "mem.h"
#include "pak.h"
#include "render.h"

void __SYS_PreInit(void) {
	mem_preinit();
}

static void done_pak(s32 bytes_read, void* orcamem) {
	pak_init(orcamem);
}

int main(void) {
	CON_EnableBarnacle(EXI_CHANNEL_0, EXI_DEVICE_1);
	printf("ORCA Runtime built " __DATE__ " " __TIME__ "\n");
	printf("Arena: %p - %p\n", SYS_GetArenaLo(), SYS_GetArenaHi());

	struct MemoryLayout mem = mem_init(0x100000); // 1MB
	render_init(mem.RenderXFB, mem.RenderFIFO);
	fst_init();

	render_ready();
	while (1) {
		render_tick();
	}

	return 0;
}
