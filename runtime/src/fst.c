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
#include <string.h>
#include <gccore.h>
#include "fst.h"
#include "orca.h"

static struct FSTEntry** const FSTBase = (void*)0x80000038;

/* String table is located immediately above FST entries */
#define STRING_TABLE ((char*)(*FSTBase + (*FSTBase)->length))

struct ExtDVDUD { /* extended DVD userdata */
	fstreadcb cb;
	void*     ud;
};

bool fst_isdir(struct FSTEntry* entry) {
	return entry->ident & 0xFF000000;
}

size_t fst_get_bufsz(struct FSTEntry* entry) {
	return ROUNDUP32(entry->length);
}

char* fst_get_filename(struct FSTEntry* entry) {
	return STRING_TABLE + (entry->ident & 0x00FFFFFF);
}

static struct FSTEntry* find_child(struct FSTEntry* dir, char const* name, size_t len) {
	if (len == 0) return NULL;

	struct FSTEntry* child = dir + 1;
	while (child < *FSTBase + dir->length) { /* length = next_offset */
		char* const childName = fst_get_filename(child);
		if (strlen(childName) == len && memcmp(childName, name, len) == 0) {
			return child;
		}
		if (fst_isdir(child)) {
			child = *FSTBase + child->length;
		} else {
			child++;
		}
	}

	return NULL;
}

struct FSTEntry* fst_resolve_path(char* path) {
	struct FSTEntry* result = NULL;

	struct FSTEntry* pwd = *FSTBase;
	char const*      segment = path;
	for (char* c = path;; c++) {
		if ((*c == '/') || (*c == '\\')) {
			if (c == path) { /* first character */
				segment++;
				continue;
			}
			pwd = find_child(pwd, segment, (size_t)(c - segment));
			if ((pwd == NULL) || !fst_isdir(pwd)) return NULL;
			segment = c + 1;
		}
		if (*c == '\0') {
			if ((size_t)(c - segment) == 0) { /* trailing "/" */
				return pwd;
			}
			return find_child(pwd, segment, (size_t)(c - segment));
		}
	}

	return result;
}

static void done_read_file(s32 bytes_read, dvdcmdblk* block) {
	struct ExtDVDUD* extud = (struct ExtDVDUD*)DVD_GetUserData(block);
	if (extud->cb != NULL) extud->cb(bytes_read, extud->ud);
	free(extud);
	free(block);
}

int fst_read_async(struct FSTEntry* entry, void* buffer, size_t length, off_t offset, fstreadcb cb, void* ud) {
#ifdef DEBUG
	if (offset % 4 != 0) {
		printf("DVD read offset must be multiple of 4\n");
		exit(1);
	}
#endif
	if (fst_isdir(entry)) return -1;

	struct ExtDVDUD* extud = calloc(1, sizeof(struct ExtDVDUD));
	extud->cb = cb;
	extud->ud = (void*)ud;

	dvdcmdblk* block = calloc(1, sizeof(dvdcmdblk));
	DVD_SetUserData(block, extud);
	DVD_ReadAbsAsync(block, buffer, ROUNDUP32(length), entry->offset + offset, done_read_file);

	return 0;
}

int fst_read_sync(struct FSTEntry* entry, void* buffer, size_t length, off_t offset) {
#ifdef DEBUG
	if (offset % 4 != 0) {
		printf("DVD read offset must be multiple of 4\n");
		exit(1);
	}
#endif
	if (fst_isdir(entry)) return -1;

	dvdcmdblk block;
	DVD_ReadAbs(&block, buffer, ROUNDUP32(length), entry->offset + offset);

	return 0;
}

static void check_fst(void) {
	for (struct FSTEntry* entry = *FSTBase; entry < *FSTBase + (*FSTBase)->length; entry++) {
		if (entry->offset % 4 != 0) {
			printf("WARNING: File \"%s\" has misaligned disc offset 0x%x\n", fst_get_filename(entry), entry->offset);
		}
	}
}

[[maybe_unused]] static void print_fst(void) {
	printf("[    FST    ] ");
	uint32_t numEntries = (*FSTBase)->length;
	printf("Total entries: %u\n", numEntries);
	for (uint32_t i = 0; i < numEntries; i++) {
		struct FSTEntry* entry = *FSTBase + i;
		printf("[%d] %s\n", i, fst_get_filename(entry));
		printf("Type: ");
		if (entry->ident & 0xFF000000) {
			printf("Directory\n");
		} else {
			printf("File\n");
		}
		printf("Offset: %u\n", entry->offset);
		printf("Length: %u\n", entry->length);
	}
}

void fst_init(void) {
	DVD_Init();
	DVD_Mount();
#ifdef DEBUG
	check_fst();
#endif
#ifdef DEBUG_FST
	print_fst();
#endif
}
