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
#include <gccore.h>

struct FSTEntry {
	u32 ident; /* byte 1    : flags; 0: file 1: directory                */
	/* bytes 2-4 : filename, offset into string table         */
	u32 offset; /* file_offset or parent_offset (dir)                     */
	u32 length; /* file_length or num_entries (root) or next_offset (dir) */
};

typedef void (*fstreadcb)(s32 bytes_read, void* ud);

bool             fst_is_directory(struct FSTEntry* entry);
size_t           fst_get_bufsz(struct FSTEntry* entry);
char*            fst_get_filename(struct FSTEntry* entry);
struct FSTEntry* fst_resolve_path(char* path);
int              fst_read_file(struct FSTEntry* entry, void* buffer, fstreadcb cb, void* ud);
void             fst_init(void);
