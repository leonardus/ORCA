/*
ORCA
Copyright (C) 2025 leonardus

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
#include <stdalign.h>
#include <string.h>
#include "orca.h"
#include "fst.h"
#include "mem.h"
#include "pak.h"

struct PAKAccessor {
	uint32_t name; // index into string table
	uint32_t buffer_offset;
	uint32_t count;
	uint8_t  component_type;
	uint8_t  element_type;
	uint8_t  _pad[2];
} __attribute__((__packed__));

struct PAKMaterial {
	uint32_t name; // index into string table
	uint32_t baseColorTexture_offset;
	uint32_t baseColorTexture_length;
	uint16_t width;
	uint16_t height;
	uint8_t  tex_coord;
	uint8_t  format;
	uint8_t  wrapS;
	uint8_t  wrapT;
} __attribute__((__packed__));

struct PAKMeshPrimitive {
	/* indices into accessors table */
	uint32_t attr_pos;
	uint32_t attr_normal;
	uint32_t attr_tangent;
	uint32_t attr_st_0;
	uint32_t attr_st_1;
	uint32_t attr_vc_0;
	uint32_t attr_joints_0;
	uint32_t attr_weights_0;
	uint32_t indices;
	/* end indices into accessors table */
	uint32_t material;
	uint8_t  mode;
	uint8_t  _pad[3];
} __attribute__((__packed__));

struct PAKMesh {
	uint32_t name; // index into string table
	uint32_t primitives_count;
	uint32_t primitives; // index into index table
} __attribute__((__packed__));

struct PAKNode {
	uint32_t name;        // index into string table
	float    rotation[4]; // quaternion rotation (x,y,z,w)
	float    scale[3];
	float    translation[3];
	uint32_t children_count;
	uint32_t children; // index into index table
	uint32_t mesh;     // index into mesh table
} __attribute__((__packed__));

struct PAKScene {
	uint32_t name; // index into string table
	uint32_t nodes_count;
	uint32_t nodes; // index into index table
} __attribute__((__packed__));

struct PAKModel {
	uint32_t index_table_count;
	uint32_t index_table_offset;
	uint32_t node_table_count;
	uint32_t node_table_offset;
	uint32_t mesh_table_count;
	uint32_t mesh_table_offset;
	uint32_t material_table_count;
	uint32_t material_table_offset;
	uint32_t primitive_table_count;
	uint32_t primitive_table_offset;
	uint32_t accessor_table_count;
	uint32_t accessor_table_offset;
	uint32_t scene_table_count;
	uint32_t scene_table_offset;
} __attribute__((__packed__));

struct PAKDirectoryEntry {
	uint32_t name; // index into string table
	uint32_t offset;
	uint8_t  type;
	uint8_t  _pad[3];
} __attribute__((__packed__));

struct PAKHeader {
	char     signature[4];
	uint32_t string_table_length;
	uint32_t string_table_offset;
	uint32_t directory_count;
	uint32_t directory_offset;
} __attribute__((__packed__));

/*
46:17:698 Core\HW\EXI\EXI_DeviceIPL.cpp:307 N[OSREPORT]: loading...
46:17:745 Core\HW\EXI\EXI_DeviceIPL.cpp:307 N[OSREPORT]: init_asset
46:17:746 Core\HW\EXI\EXI_DeviceIPL.cpp:307 N[OSREPORT]: init_model
46:17:746 Core\HW\EXI\EXI_DeviceIPL.cpp:307 N[OSREPORT]: init_node
46:17:747 Core\HW\EXI\EXI_DeviceIPL.cpp:307 N[OSREPORT]: init_node
46:17:747 Core\HW\EXI\EXI_DeviceIPL.cpp:307 N[OSREPORT]: init_node
46:17:748 Core\HW\EXI\EXI_DeviceIPL.cpp:307 N[OSREPORT]: init_mesh
46:17:748 Core\HW\EXI\EXI_DeviceIPL.cpp:307 N[OSREPORT]: init_material
46:17:971 Core\HW\EXI\EXI_DeviceIPL.cpp:307 N[OSREPORT]: init_primitive
46:18:027 Core\HW\EXI\EXI_DeviceIPL.cpp:307 N[OSREPORT]: init_accessor
46:18:110 Core\HW\EXI\EXI_DeviceIPL.cpp:307 N[OSREPORT]: init_accessor
46:18:126 Core\HW\EXI\EXI_DeviceIPL.cpp:307 N[OSREPORT]: init_accessor
46:18:128 Core\HW\EXI\EXI_DeviceIPL.cpp:307 N[OSREPORT]: init_accessor
46:18:190 Core\HW\EXI\EXI_DeviceIPL.cpp:307 N[OSREPORT]: init_scene
46:18:190 Core\HW\EXI\EXI_DeviceIPL.cpp:307 N[OSREPORT]: done loading
*/

static void init_node(struct Level* level, struct Model* model, struct Node* node, struct PAKNode* PAKNode) {
	node->name = PAKNode->name == UINT32_MAX ? "" : level->stringTable + PAKNode->name;
	node->mesh = PAKNode->mesh == UINT32_MAX ? NULL : model->meshes + PAKNode->mesh;
	node->childrenIdxs = model->idxs + PAKNode->children;
	node->numChildren = PAKNode->children_count;
	node->rotation.x = PAKNode->rotation[0];
	node->rotation.y = PAKNode->rotation[1];
	node->rotation.z = PAKNode->rotation[2];
	node->rotation.w = PAKNode->rotation[3];
	node->scale.x = PAKNode->scale[0];
	node->scale.y = PAKNode->scale[1];
	node->scale.z = PAKNode->scale[2];
	node->translation.x = PAKNode->translation[0];
	node->translation.y = PAKNode->translation[1];
	node->translation.z = PAKNode->translation[2];
}

static void init_mesh(struct Level* level, struct Model* model, struct Mesh* mesh, struct PAKMesh* PAKMesh) {
	mesh->name = PAKMesh->name == UINT32_MAX ? NULL : level->stringTable + PAKMesh->name;
	mesh->primitivesIdxs = model->idxs + PAKMesh->primitives;
	mesh->numPrimitives = PAKMesh->primitives_count;
}

static inline enum WrapMode get_wrap_mode(uint8_t mode) {
	switch (mode) {
	case 0:
		return WRAP_CLAMP;
		break;
	case 1:
		return WRAP_MIRROR;
		break;
	case 2:
		return WRAP_REPEAT;
		break;
	default:
		printf("WARNING: Unrecognized wrap mode '%u', defaulting to WRAP_CLAMP.\n", mode);
		return WRAP_CLAMP;
		break;
	}
}

static void init_material(struct FSTEntry* file, struct Level* level, struct Material* material,
                          struct PAKMaterial* PAKMaterial) {
	material->name = PAKMaterial->name == UINT32_MAX ? "" : level->stringTable + PAKMaterial->name;
	material->texture = mem_alloc_scratch(sizeof(GXTexObj), 32);
	material->wrapS = get_wrap_mode(PAKMaterial->wrapS);
	material->wrapT = get_wrap_mode(PAKMaterial->wrapT);
	switch (PAKMaterial->format) {
	case 0:
		material->format = TF_I4;
		break;
	case 1:
		material->format = TF_I8;
		break;
	case 2:
		material->format = TF_IA4;
		break;
	case 3:
		material->format = TF_IA8;
		break;
	case 7:
		material->format = TF_RGB565;
		break;
	case 8:
		material->format = TF_RGB5A3;
		break;
	case 9:
		material->format = TF_RGBA8;
		break;
	case 10:
		material->format = TF_CMPR;
		break;
	default:
		printf("ERROR: Unrecognized texture format '%u'\n", PAKMaterial->format);
		exit(1);
	}
	material->texCoord = PAKMaterial->tex_coord;

	size_t const texbufsz = ROUNDUP32(PAKMaterial->baseColorTexture_length);
	void*        texture = mem_alloc_scratch(texbufsz, 32);
	fst_read_sync(file, texture, texbufsz, PAKMaterial->baseColorTexture_offset);

	GX_InitTexObj(material->texture, texture, PAKMaterial->width, PAKMaterial->height, material->format,
	              material->wrapS, material->wrapT, FALSE);
}

static void init_primitive(struct Model* model, struct MeshPrimitive* primitive,
                           struct PAKMeshPrimitive* PAKMeshPrimitive) {
	primitive->attrPos =
	    PAKMeshPrimitive->attr_pos == UINT32_MAX ? NULL : model->accessors + PAKMeshPrimitive->attr_pos;
	primitive->attrNormal =
	    PAKMeshPrimitive->attr_normal == UINT32_MAX ? NULL : model->accessors + PAKMeshPrimitive->attr_normal;
	primitive->attrTangent =
	    PAKMeshPrimitive->attr_tangent == UINT32_MAX ? NULL : model->accessors + PAKMeshPrimitive->attr_tangent;
	primitive->attrTexCoord0 =
	    PAKMeshPrimitive->attr_st_0 == UINT32_MAX ? NULL : model->accessors + PAKMeshPrimitive->attr_st_0;
	primitive->attrTexCoord1 =
	    PAKMeshPrimitive->attr_st_1 == UINT32_MAX ? NULL : model->accessors + PAKMeshPrimitive->attr_st_1;
	primitive->attrColor =
	    PAKMeshPrimitive->attr_vc_0 == UINT32_MAX ? NULL : model->accessors + PAKMeshPrimitive->attr_vc_0;
	primitive->attrJoints =
	    PAKMeshPrimitive->attr_joints_0 == UINT32_MAX ? NULL : model->accessors + PAKMeshPrimitive->attr_joints_0;
	primitive->attrWeights =
	    PAKMeshPrimitive->attr_weights_0 == UINT32_MAX ? NULL : model->accessors + PAKMeshPrimitive->attr_weights_0;
	primitive->indices = PAKMeshPrimitive->indices == UINT32_MAX ? NULL : model->accessors + PAKMeshPrimitive->indices;
	primitive->material =
	    PAKMeshPrimitive->material == UINT32_MAX ? NULL : model->materials + PAKMeshPrimitive->material;

	switch (PAKMeshPrimitive->mode) {
	case 0:
		primitive->mode = MODE_POINTS;
		break;
	case 1:
		primitive->mode = MODE_LINES;
		break;
	case 3:
		primitive->mode = MODE_LINE_STRIP;
		break;
	case 4:
		primitive->mode = MODE_TRIANGLES;
		break;
	case 5:
		primitive->mode = MODE_TRIANGLE_STRIP;
		break;
	case 6:
		primitive->mode = MODE_TRIANGLE_FAN;
		break;
	default:
		printf("ERROR: Unrecognized primitive mode '%u'\n", PAKMeshPrimitive->mode);
		exit(1);
	}
}

static void init_accessor(struct FSTEntry* file, struct Level* level, struct Accessor* accessor,
                          struct PAKAccessor* PAKAccessor) {
	accessor->name = PAKAccessor->name == UINT32_MAX ? "" : level->stringTable + PAKAccessor->name;

	size_t compSize = 0;
	size_t compCount = 0;
	switch (PAKAccessor->component_type) {
	case 0:
		accessor->componentType = COMPONENT_F32;
		compSize = sizeof(float);
		break;
	case 1:
		accessor->componentType = COMPONENT_S8;
		compSize = sizeof(int8_t);
		break;
	case 2:
		accessor->componentType = COMPONENT_U8;
		compSize = sizeof(uint8_t);
		break;
	case 3:
		accessor->componentType = COMPONENT_S16;
		compSize = sizeof(int16_t);
		break;
	case 4:
		accessor->componentType = COMPONENT_U16;
		compSize = sizeof(uint16_t);
		break;
	case 5:
		accessor->componentType = COMPONENT_U32;
		compSize = sizeof(uint32_t);
		break;
	default:
		printf("ERROR: Unrecognized component type '%u'\n", PAKAccessor->component_type);
		exit(1);
	}
	switch (PAKAccessor->element_type) {
	case 0:
		accessor->elementType = ELEM_SCALAR;
		compCount = 1;
		break;
	case 1:
		accessor->elementType = ELEM_VEC2;
		compCount = 2;
		break;
	case 2:
		accessor->elementType = ELEM_VEC3;
		compCount = 3;
		break;
	case 3:
		accessor->elementType = ELEM_VEC4;
		compCount = 4;
		break;
	case 4:
		accessor->elementType = ELEM_MAT2;
		compCount = 4;
		break;
	case 5:
		accessor->elementType = ELEM_MAT3;
		compCount = 9;
		break;
	case 6:
		accessor->elementType = ELEM_MAT4;
		compCount = 16;
		break;
	default:
		printf("ERROR: Unrecognized element type '%u'\n", PAKAccessor->element_type);
		exit(1);
	}

	accessor->count = PAKAccessor->count;
	accessor->stride = compSize * compCount;
	size_t const bufsz = ROUNDUP32(accessor->stride * accessor->count);
	accessor->buffer = mem_alloc_scratch(bufsz, 32);
	fst_read_sync(file, accessor->buffer, bufsz, PAKAccessor->buffer_offset);
}

static void init_scene(struct Level* level, struct Model* model, struct Scene* scene, struct PAKScene* PAKScene) {
	scene->name = PAKScene->name == UINT32_MAX ? "" : level->stringTable + PAKScene->name;
	scene->nodesIdxs = model->idxs + PAKScene->nodes;
	scene->numNodes = PAKScene->nodes_count;
}

static void init_model(struct FSTEntry* file, struct Level* level, struct Model* model, struct PAKModel* PAKModel) {
	model->numIdxs = PAKModel->index_table_count;
	model->numNodes = PAKModel->node_table_count;
	model->numMeshes = PAKModel->mesh_table_count;
	model->numMaterials = PAKModel->material_table_count;
	model->numPrimitives = PAKModel->primitive_table_count;
	model->numAccessors = PAKModel->accessor_table_count;
	model->numScenes = PAKModel->scene_table_count;

	model->idxs = mem_alloc_scratch(model->numIdxs * sizeof(uint32_t), 32);
	model->nodes = mem_alloc_scratch(model->numNodes * sizeof(struct Node), alignof(struct Node));
	model->meshes = mem_alloc_scratch(model->numMeshes * sizeof(struct Mesh), alignof(struct Mesh));
	model->materials = mem_alloc_scratch(model->numMaterials * sizeof(struct Material), alignof(struct Material));
	model->primitives =
	    mem_alloc_scratch(model->numPrimitives * sizeof(struct MeshPrimitive), alignof(struct MeshPrimitive));
	model->accessors = mem_alloc_scratch(model->numAccessors * sizeof(struct Accessor), alignof(struct Accessor));
	model->scenes = mem_alloc_scratch(model->numScenes * sizeof(struct Scene), alignof(struct Scene));

	fst_read_sync(file, model->idxs, PAKModel->index_table_count * sizeof(uint32_t), PAKModel->index_table_offset);

	struct PAKNode* const PAKNodes = aligned_alloc(32, ROUNDUP32(PAKModel->node_table_count * sizeof(struct PAKNode)));
	mem_checkOOM(PAKNodes);
	fst_read_sync(file, PAKNodes, ROUNDUP32(PAKModel->node_table_count * sizeof(struct PAKNode)),
	              PAKModel->node_table_offset);
	for (uint32_t i = 0; i < PAKModel->node_table_count; i++) {
		init_node(level, model, &model->nodes[i], &PAKNodes[i]);
	}
	free(PAKNodes);

	struct PAKMesh* const PAKMeshes = aligned_alloc(32, ROUNDUP32(PAKModel->mesh_table_count * sizeof(struct PAKMesh)));
	mem_checkOOM(PAKMeshes);
	fst_read_sync(file, PAKMeshes, ROUNDUP32(PAKModel->mesh_table_count * sizeof(struct PAKMesh)),
	              PAKModel->mesh_table_offset);
	for (uint32_t i = 0; i < PAKModel->mesh_table_count; i++) {
		init_mesh(level, model, &model->meshes[i], &PAKMeshes[i]);
	}
	free(PAKMeshes);

	struct PAKMaterial* const PAKMaterials =
	    aligned_alloc(32, ROUNDUP32(PAKModel->material_table_count * sizeof(struct PAKMaterial)));
	mem_checkOOM(PAKMaterials);
	fst_read_sync(file, PAKMaterials, ROUNDUP32(PAKModel->material_table_count * sizeof(struct PAKMaterial)),
	              PAKModel->material_table_offset);
	for (uint32_t i = 0; i < PAKModel->material_table_count; i++) {
		init_material(file, level, &model->materials[i], &PAKMaterials[i]);
	}
	free(PAKMaterials);

	struct PAKMeshPrimitive* const PAKMeshPrimitives =
	    aligned_alloc(32, ROUNDUP32(PAKModel->primitive_table_count * sizeof(struct PAKMeshPrimitive)));
	mem_checkOOM(PAKMeshPrimitives);
	fst_read_sync(file, PAKMeshPrimitives, ROUNDUP32(PAKModel->primitive_table_count * sizeof(struct PAKMeshPrimitive)),
	              PAKModel->primitive_table_offset);
	for (uint32_t i = 0; i < PAKModel->primitive_table_count; i++) {
		init_primitive(model, &model->primitives[i], &PAKMeshPrimitives[i]);
	}
	free(PAKMeshPrimitives);

	struct PAKAccessor* const PAKAccessors =
	    aligned_alloc(32, ROUNDUP32(PAKModel->accessor_table_count * sizeof(struct PAKAccessor)));
	mem_checkOOM(PAKAccessors);
	fst_read_sync(file, PAKAccessors, ROUNDUP32(PAKModel->accessor_table_count * sizeof(struct PAKAccessor)),
	              PAKModel->accessor_table_offset);
	for (uint32_t i = 0; i < PAKModel->accessor_table_count; i++) {
		init_accessor(file, level, &model->accessors[i], &PAKAccessors[i]);
	}
	free(PAKAccessors);

	struct PAKScene* const PAKScenes =
	    aligned_alloc(32, ROUNDUP32(PAKModel->scene_table_count * sizeof(struct PAKScene)));
	mem_checkOOM(PAKScenes);
	fst_read_sync(file, PAKScenes, ROUNDUP32(PAKModel->scene_table_count * sizeof(struct PAKScene)),
	              PAKModel->scene_table_offset);
	for (uint32_t i = 0; i < PAKModel->scene_table_count; i++) {
		init_scene(level, model, &model->scenes[i], &PAKScenes[i]);
	}
	free(PAKScenes);
}

static void init_asset(struct FSTEntry* file, struct Level* level, struct Asset* asset,
                       struct PAKDirectoryEntry* PAKDirectoryEntry) {
	asset->name = PAKDirectoryEntry->name == UINT32_MAX ? "" : level->stringTable + PAKDirectoryEntry->name;
	asset->type = PAKDirectoryEntry->type;

	switch (asset->type) {
	case ASSET_MODEL:
		asset->addr = mem_alloc_scratch(sizeof(struct Model), alignof(struct Model));
		struct PAKModel* PAKModel = aligned_alloc(32, ROUNDUP32(sizeof(struct PAKModel)));
		fst_read_sync(file, PAKModel, ROUNDUP32(sizeof(struct PAKModel)), PAKDirectoryEntry->offset);
		init_model(file, level, asset->addr, PAKModel);
		free(PAKModel);
	case ASSET_SCRIPT:
	case ASSET_SOUND:
	default:
		break;
	}
}

struct Level* pak_load(char* levelName) {
	if (strlen(levelName) > 63) {
		printf("ERROR: Level name exceeded maximum of 63 characters\n");
		exit(1);
	}

	char filename[63 + 4 + 1];
	snprintf(filename, 63 + 4 + 1, "%s.PAK", levelName);
	struct FSTEntry* const file = fst_resolve_path(filename);
	if (file == NULL) return NULL;

	mem_reset_scratch();
	struct Level* const level = mem_alloc_scratch(sizeof(struct Level), alignof(struct Level));

	struct PAKHeader* const header = aligned_alloc(32, ROUNDUP32(sizeof(struct PAKHeader)));
	mem_checkOOM(header);
	fst_read_sync(file, header, ROUNDUP32(sizeof(struct PAKHeader)), 0);
	level->stringTable = mem_alloc_scratch(ROUNDUP32(header->string_table_length), 32);
	level->assets = mem_alloc_scratch(sizeof(struct Asset) * header->directory_count, alignof(struct Asset));
	level->numAssets = header->directory_count;
	fst_read_sync(file, level->stringTable, ROUNDUP32(header->string_table_length), header->string_table_offset);

	struct PAKDirectoryEntry* const directory =
	    aligned_alloc(32, ROUNDUP32(sizeof(struct PAKDirectoryEntry) * header->directory_count));
	mem_checkOOM(directory);
	fst_read_sync(file, directory, header->directory_count, header->directory_offset);
	for (size_t i = 0; i < header->directory_count; i++) {
		init_asset(file, level, &level->assets[i], &directory[i]);
	}
	free(directory);

	free(header);

	return level;
}
