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

#pragma once
#include <stddef.h>

struct MemoryLayout {
	void*  renderXFB;
	void*  renderFIFO;
	void*  levelData;
	size_t levelCapacity;
	size_t heapCapacity;
};

void mem_checkalign(void* p, size_t alignment, char const* info);

void mem_preinit(void);

struct MemoryLayout mem_init(size_t heapSize);
