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
#include <gccore.h>

enum AssetType { ASSET_MODEL, ASSET_SCRIPT, ASSET_SOUND };

enum ComponentType {
	COMPONENT_F32 = GX_F32,
	COMPONENT_S8 = GX_S8,
	COMPONENT_U8 = GX_U8,
	COMPONENT_S16 = GX_S16,
	COMPONENT_U16 = GX_U16,
	COMPONENT_U32 // Not a supported GX component type. Used for accessors containing indices.
};

enum TexFormat {
	TF_I4 = GX_TF_I4,
	TF_I8 = GX_TF_I8,
	TF_IA4 = GX_TF_IA4,
	TF_IA8 = GX_TF_IA8,
	TF_RGB565 = GX_TF_RGB565,
	TF_RGB5A3 = GX_TF_RGB5A3,
	TF_RGBA8 = GX_TF_RGBA8,
	TF_CMPR = GX_TF_CMPR
};

enum PrimitiveMode {
	MODE_POINTS = GX_POINTS,
	MODE_LINES = GX_LINES,
	MODE_LINE_LOOP, // Not a supported GX rendering mode.
	MODE_LINE_STRIP = GX_LINESTRIP,
	MODE_TRIANGLES = GX_TRIANGLES,
	MODE_TRIANGLE_STRIP = GX_TRIANGLESTRIP,
	MODE_TRIANGLE_FAN = GX_TRIANGLEFAN
};

enum WrapMode { WRAP_CLAMP = GX_CLAMP, WRAP_REPEAT = GX_REPEAT, WRAP_MIRROR = GX_MIRROR };

enum ElementType { ELEM_SCALAR, ELEM_VEC2, ELEM_VEC3, ELEM_VEC4, ELEM_MAT2, ELEM_MAT3, ELEM_MAT4 };

struct Accessor {
	char const*        name;
	void*              buffer;
	size_t             count;
	size_t             stride;
	enum ComponentType componentType;
	enum ElementType   elementType;
};

struct Material {
	char const*    name;
	GXTexObj*      texture;
	enum WrapMode  wrapS;
	enum WrapMode  wrapT;
	enum TexFormat format;
	uint8_t        texCoord;
};

struct MeshPrimitive {
	struct Accessor*   attrPos;
	struct Accessor*   attrNormal;
	struct Accessor*   attrTangent;
	struct Accessor*   attrTexCoord0;
	struct Accessor*   attrTexCoord1;
	struct Accessor*   attrColor;
	struct Accessor*   attrJoints;
	struct Accessor*   attrWeights;
	struct Accessor*   indices;
	struct Material*   material;
	enum PrimitiveMode mode;
};

struct Mesh {
	char const* name;
	uint32_t*   primitivesIdxs;
	size_t      numPrimitives;
};

struct Node {
	char const*  name;
	struct Mesh* mesh;
	uint32_t*    childrenIdxs;
	size_t       numChildren;
	guQuaternion rotation;
	guVector     scale;
	guVector     translation;
};

struct Scene {
	char const* name;
	uint32_t*   nodesIdxs;
	size_t      numNodes;
};

struct Model {
	uint32_t*             idxs;
	struct Node*          nodes;
	struct Mesh*          meshes;
	struct Material*      materials;
	struct MeshPrimitive* primitives;
	struct Accessor*      accessors;
	struct Scene*         scenes;
	size_t                numIdxs;
	size_t                numNodes;
	size_t                numMeshes;
	size_t                numMaterials;
	size_t                numPrimitives;
	size_t                numAccessors;
	size_t                numScenes;
};

struct Asset {
	char const*    name;
	void*          addr;
	enum AssetType type;
};

struct Level {
	char*         stringTable;
	struct Asset* assets;
	size_t        numAssets;
};

struct Level* pak_load(char* levelName);
