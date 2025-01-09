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
#include <gccore.h>
#include "pak.h"

size_t render_get_xfbsz(void);
size_t render_get_fifosz(void);

void render_init(void* xfb, void* fifo);
void render_ready(void);
void render_tick(Mtx camera, struct PAKHeader* pak);
