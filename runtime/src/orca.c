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
#include <string.h>
#include <gccore.h>
#include "fst.h"
#include "mem.h"
#include "pak.h"
#include "render.h"

void __SYS_PreInit(void) {
	mem_preinit();
}

static struct Node* first_node_name(struct Model* m, char* name) {
	for (struct Node* n = m->nodes; n < m->nodes + m->numNodes; n++) {
		if (n->name == NULL) continue;
		if (strcmp(name, n->name) == 0) return n;
	}
	return NULL;
}

int main(void) {
#ifdef DEBUG
	CON_EnableBarnacle(EXI_CHANNEL_0, EXI_DEVICE_1);
#endif
	printf("ORCA Runtime built " __DATE__ " " __TIME__ "\n");
	printf("Arena: %p - %p\n", SYS_GetArenaLo(), SYS_GetArenaHi());

	mem_init(0x100000); // 1MB
	render_init();
	fst_init();

	struct Level* const level = pak_load("~default");
	if (level == NULL) {
		printf("ERROR: Could not locate default level (~default.PAK)\n");
		exit(1);
	}
	struct Model* model = NULL;
	for (struct Asset* a = level->assets; a < level->assets + level->numAssets; a++) {
		if (a->type == ASSET_MODEL && strcmp(a->name, "~default") == 0) {
			model = a->addr;
			break;
		}
	}

	if (model != NULL) {
		struct Node* const camera = first_node_name(model, "Camera");
		struct Node* const target = first_node_name(model, "Target");
		if (camera != NULL && target != NULL) {
			Mtx mtx;
			guLookAt(mtx, &camera->translation, &(guVector){0, 1, 0}, &target->translation);
			render_set_camera(mtx);
		}
	}

	render_ready();
	while (1) {
		render_tick(model);
	}

	return 0;
}
