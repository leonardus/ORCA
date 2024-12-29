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

#include "mem.h"

/*
 * libogc2's SYS_Init() calls __lowmem_init(), which sets ArenaHi and ArenaLo
 * to hardcoded values in devkitPro's linker script. This value can be overridden
 * by assigning values to the weak symbols (void*)__myArena1Lo/__myArena1Hi.
 * Because SYS_Init() calls __SYS_PreInit() prior to __lowmem_init(), we set the
 * __myArena1Lo/__myArena1Hi here to the values that are given by the apploader,
 * before it assumes to use the default values.
 */
void mem_preinit(void) {
	extern void* __myArena1Lo;
	extern void* __myArena1Hi;
	__myArena1Lo = *(void**)0x80000030;
	__myArena1Hi = *(void**)0x80000034;
}
