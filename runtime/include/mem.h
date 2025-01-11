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

#pragma once
#include <stddef.h>

extern void* g_XFB0;
extern void* g_FIFO;

void  mem_checkOOM(void* p);
void  mem_checkalign(void* p, size_t alignment, char const* info);
void  mem_preinit(void);
void  mem_init(size_t heapSize);
void* mem_alloc_scratch(size_t n, size_t align);
void  mem_reset_scratch(void);
