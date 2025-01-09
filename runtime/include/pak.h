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

#include <gccore.h>

enum AssetType { ASSET_TYPE_MODEL = 0, ASSET_TYPE_SCRIPT, ASSET_TYPE_SOUND };

enum PrimitiveMode {
	MODE_POINTS,
	MODE_LINES,
	MODE_LINE_LOOP,
	MODE_LINE_STRIP,
	MODE_TRIANGLES,
	MODE_TRIANGLE_STRIP,
	MODE_TRAINGLE_FAN
};

enum ComponentType { FLOAT = 0, BYTE, UNSIGNED_BYTE, SHORT, UNSIGNED_SHORT, UNSIGNED_INT };

enum ElementType { SCALAR = 0, VEC2, VEC3, VEC4, MAT2, MAT3, MAT4 };

struct Accessor {
	char const* name;
	void*       buffer;
	size_t      count;
	u8          component_type; // enum ComponentType
	u8          element_type;   // enum ElementType
	u8          _pad[2];
} __attribute__((__packed__));

struct Material {
	char const* name;
	void*       texture;
	u8          tex_coord;
	u8          format;
	u8          wrap_s;
	u8          wrap_t;
} __attribute__((__packed__));

struct MeshPrimitive {
	struct Accessor* attr_pos;
	struct Accessor* attr_normal;
	struct Accessor* attr_tangent;
	struct Accessor* attr_st_0;
	struct Accessor* attr_st_1;
	struct Accessor* attr_vc_0;
	struct Accessor* attr_joints_0;
	struct Accessor* attr_weights_0;
	struct Accessor* indices;
	struct Material* material;
	u8               mode; // enum PrimitiveMode
} __attribute__((__packed__));

struct Mesh {
	char const* name;
	size_t      primitives_count;
	u32 const*  primitives;
} __attribute__((__packed__));

struct Node {
	char const*  name;
	f32          rotation[4];
	f32          scale[3];
	f32          translation[3];
	size_t       children_count;
	u32 const*   children;
	struct Mesh* mesh;
} __attribute__((__packed__));

struct Scene {
	char const* name;
	size_t      nodes_count;
	u32 const*  nodes;
} __attribute__((__packed__));

struct Model {
	size_t                index_table_count;
	u32 const*            index_table;
	size_t                node_table_count;
	struct Node*          node_table;
	size_t                mesh_table_count;
	struct Mesh*          mesh_table;
	size_t                material_table_count;
	struct Material*      material_table;
	size_t                primitive_table_count;
	struct MeshPrimitive* primitive_table;
	size_t                accessor_table_count;
	struct Accessor*      accessor_table;
	size_t                scene_table_count;
	struct Scene*         scene_table;
} __attribute__((__packed__));

struct DirectoryEntry {
	char const* name;
	void*       offset;
	u8          type; // enum AssetType
	u8          _pad[3];
} __attribute__((__packed__));

struct PAKHeader {
	char                   signature[4];
	size_t                 string_table_length;
	char const*            string_table;
	size_t                 directory_count;
	struct DirectoryEntry* directory;
} __attribute__((__packed__));

void pak_init(struct PAKHeader* hdr);
