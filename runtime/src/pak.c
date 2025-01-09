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
#include "pak.h"

static void patch_Model(struct Model* model, u8* const pakBase, struct PAKHeader* hdr) {
	model->index_table = (u32 const*)(pakBase + (uintptr_t)model->index_table);
	model->node_table = (struct Node*)(pakBase + (uintptr_t)model->node_table);
	model->mesh_table = (struct Mesh*)(pakBase + (uintptr_t)model->mesh_table);
	model->material_table = (struct Material*)(pakBase + (uintptr_t)model->material_table);
	model->primitive_table = (struct MeshPrimitive*)(pakBase + (uintptr_t)model->primitive_table);
	model->accessor_table = (struct Accessor*)(pakBase + (uintptr_t)model->accessor_table);
	model->scene_table = (struct Scene*)(pakBase + (uintptr_t)model->scene_table);

	for (struct Node* n = model->node_table; n < model->node_table + model->node_table_count; n++) {
		n->name = hdr->string_table + (uintptr_t)n->name;
		n->children = model->index_table + (uintptr_t)n->children;
		n->mesh = (uintptr_t)n->mesh == UINT32_MAX ? NULL : model->mesh_table + (uintptr_t)n->mesh;
	}

	for (struct Mesh* m = model->mesh_table; m < model->mesh_table + model->mesh_table_count; m++) {
		m->name = hdr->string_table + (uintptr_t)m->name;
		m->primitives = model->index_table + (uintptr_t)m->primitives;
	}

	for (struct Material* m = model->material_table; m < model->material_table + model->material_table_count; m++) {
		m->name = hdr->string_table + (uintptr_t)m->name;
		m->texture = pakBase + (uintptr_t)m->texture;
	}

	for (struct MeshPrimitive* p = model->primitive_table; p < model->primitive_table + model->primitive_table_count;
	     p++) {

		p->attr_pos = (uintptr_t)p->attr_pos == UINT32_MAX ? NULL : model->accessor_table + (uintptr_t)p->attr_pos;
		p->attr_normal =
		    (uintptr_t)p->attr_normal == UINT32_MAX ? NULL : model->accessor_table + (uintptr_t)p->attr_normal;
		p->attr_tangent =
		    (uintptr_t)p->attr_tangent == UINT32_MAX ? NULL : model->accessor_table + (uintptr_t)p->attr_tangent;
		p->attr_st_0 = (uintptr_t)p->attr_st_0 == UINT32_MAX ? NULL : model->accessor_table + (uintptr_t)p->attr_st_0;
		p->attr_st_1 = (uintptr_t)p->attr_st_1 == UINT32_MAX ? NULL : model->accessor_table + (uintptr_t)p->attr_st_1;
		p->attr_vc_0 = (uintptr_t)p->attr_vc_0 == UINT32_MAX ? NULL : model->accessor_table + (uintptr_t)p->attr_vc_0;
		p->attr_joints_0 =
		    (uintptr_t)p->attr_joints_0 == UINT32_MAX ? NULL : model->accessor_table + (uintptr_t)p->attr_joints_0;
		p->attr_weights_0 =
		    (uintptr_t)p->attr_weights_0 == UINT32_MAX ? NULL : model->accessor_table + (uintptr_t)p->attr_weights_0;
		p->indices = (uintptr_t)p->indices == UINT32_MAX ? NULL : model->accessor_table + (uintptr_t)p->indices;
		p->material = (uintptr_t)p->material == UINT32_MAX ? NULL : model->material_table + (uintptr_t)p->material;

		switch ((enum PrimitiveMode)p->mode) {
		case MODE_POINTS:
			p->mode = GX_POINTS;
			break;
		case MODE_LINES:
			p->mode = GX_LINES;
			break;
		case MODE_LINE_LOOP:
			p->mode = GX_LINESTRIP;
			break;
		case MODE_LINE_STRIP:
			p->mode = GX_LINESTRIP;
			break;
		case MODE_TRIANGLES:
			p->mode = GX_TRIANGLES;
			break;
		case MODE_TRIANGLE_STRIP:
			p->mode = GX_TRIANGLESTRIP;
			break;
		case MODE_TRAINGLE_FAN:
			p->mode = GX_TRIANGLEFAN;
			break;
		default:
			printf("WARNING: Unrecognized primitive mode.\n");
			break;
		}
	}

	for (struct Accessor* a = model->accessor_table; a < model->accessor_table + model->accessor_table_count; a++) {
		a->name = hdr->string_table + (uintptr_t)a->name;
		a->buffer = pakBase + (uintptr_t)a->buffer;

		switch ((enum ComponentType)a->component_type) {
		case BYTE:
			a->component_type = GX_U8;
			break;
		case UNSIGNED_BYTE:
			a->component_type = GX_S8;
			break;
		case SHORT:
			a->component_type = GX_S16;
			break;
		case UNSIGNED_SHORT:
			a->component_type = GX_U16;
			break;
		case UNSIGNED_INT:
			// GX has no corresponding component type. Handle this properly later.
			a->component_type = UINT8_MAX;
			break;
		case FLOAT:
			a->component_type = GX_F32;
			break;
		default:
			printf("WARNING: Unrecognized component type.\n");
			break;
		}
	}

	for (struct Scene* s = model->scene_table; s < model->scene_table + model->scene_table_count; s++) {
		s->name = hdr->string_table + (uintptr_t)s->name;
		s->nodes = model->index_table + (uintptr_t)s->nodes;
	}
}

static void patch_DirectoryEntry(struct DirectoryEntry* entry, u8* const pakBase, struct PAKHeader* hdr) {
	entry->name = hdr->string_table + (uintptr_t)entry->name;
	entry->offset = pakBase + (uintptr_t)entry->offset;

	switch (entry->type) {
	case ASSET_TYPE_MODEL:
		patch_Model((struct Model*)entry->offset, pakBase, hdr);
		break;
	case ASSET_TYPE_SCRIPT:
	case ASSET_TYPE_SOUND:
		// not yet implemented
		break;
	default:
#ifdef DEBUG
		printf("WARNING: Unrecognized asset type %u.\n", entry->type);
#endif
	}
}

static void patch(struct PAKHeader* hdr, u8* const pakBase) {
	hdr->string_table = (char*)(pakBase + (uintptr_t)hdr->string_table);
	hdr->directory = (struct DirectoryEntry*)(pakBase + (uintptr_t)hdr->directory);

	for (size_t i = 0; i < hdr->directory_count; i++) {
		patch_DirectoryEntry(hdr->directory + i, pakBase, hdr);
	}
}

void pak_init(struct PAKHeader* hdr) {
	if (*hdr->signature == 'I') return;
	*hdr->signature = 'I';
	u8* const base = (u8*)hdr;
	patch(hdr, base);
}
