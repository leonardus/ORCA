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
#include "fst.h"
#include "mem.h"
#include "pak.h"

void __SYS_PreInit(void) {
	mem_preinit();
}

static void done_pak(s32 bytes_read, void* orcamem) {
	pak_init(orcamem);
}

int main(void) {
	SYS_Report("ORCA Runtime built " __DATE__ " " __TIME__ "\n");
	SYS_Report("Arena: %p - %p\n", SYS_GetArenaLo(), SYS_GetArenaHi());

	fst_init();

	void* orcamem = (void*)((uintptr_t)SYS_GetArenaLo() + 0x100000); // 1MB
	if (orcamem > SYS_GetArenaHi()) {
		SYS_Report("Not enough memory.\n");
		return -2;
	}
	size_t capacity = (uintptr_t)SYS_GetArenaHi() - (uintptr_t)orcamem;
	SYS_SetArenaHi(orcamem);

	struct FSTEntry* q = fst_resolve_path("duck.PAK");
	if (q == NULL) {
		SYS_Report("Could not resolve duck.PAK.\n");
		return -1;
	}

	if (capacity < fst_get_bufsz(q)) {
		SYS_Report("Not enough memory.\n");
		return -2;
	}

	fst_read_file(q, orcamem, done_pak, orcamem);

	while (1)
		;

	return 0;
}
