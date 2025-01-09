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
#include <string.h>
#include <gccore.h>
#include "fst.h"
#include "mem.h"
#include "pak.h"
#include "render.h"

static struct PAKHeader* g_pak = NULL;
static Mtx               g_camera;

void __SYS_PreInit(void) {
	mem_preinit();
}

static struct Node* first_node_name(struct Model* m, char* name) {
	for (size_t i = 0; i < m->node_table_count; i++) {
		if (strcmp(name, m->node_table[i].name) == 0) return &m->node_table[i];
	}
	return NULL;
}

static void done_pak(s32 bytesRead, void* orcamem) {
	pak_init(orcamem);
	struct PAKHeader* pak = orcamem;

	struct Node* duck = first_node_name(pak->directory[0].offset, "Duck");
	struct Node* camera = first_node_name(pak->directory[0].offset, "Camera");
	if (duck == NULL || camera == NULL) {
		printf("Could not find node\n");
		exit(1);
	}
	// camera->translation[0] /= 3;
	// camera->translation[1] /= 3;
	// camera->translation[2] /= 3;
	printf("Camera: (%f, %f, %f)\tTarget: (%f, %f, %f)\n", ((guVector*)camera->translation)->x,
	       ((guVector*)camera->translation)->y, ((guVector*)camera->translation)->z, ((guVector*)duck->translation)->x,
	       ((guVector*)duck->translation)->y, ((guVector*)duck->translation)->z);
	guLookAt(g_camera, (guVector*)camera->translation, &(guVector){0, 1, 0}, (guVector*)duck->translation);

	g_pak = pak;
}

int main(void) {
#ifdef DEBUG
	CON_EnableBarnacle(EXI_CHANNEL_0, EXI_DEVICE_1);
#endif
	printf("ORCA Runtime built " __DATE__ " " __TIME__ "\n");
	printf("Arena: %p - %p\n", SYS_GetArenaLo(), SYS_GetArenaHi());

	struct MemoryLayout mem = mem_init(0x100000); // 1MB
	render_init(mem.RenderXFB, mem.RenderFIFO);
	fst_init();

	fst_read_file(fst_resolve_path("duck.PAK"), mem.LevelData, done_pak, mem.LevelData);

	render_ready();
	while (1) {
		render_tick(g_camera, g_pak);
	}

	return 0;
}
