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

package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"image"
	_ "image/jpeg"
	_ "image/png"
	"path/filepath"
	"reflect"

	"github.com/qmuntal/gltf"
	gltf_binary "github.com/qmuntal/gltf/binary"
	"github.com/qmuntal/gltf/modeler"
)

const UINT32_MAX uint32 = ^uint32(0)

type BinPakHeader struct {
	Signature         uint32
	StringTableLength uint32
	StringTableOffset uint32
	DirectoryCount    uint32
	DirectoryOffset   uint32
}

type BinDirectoryEntry struct {
	Name   uint32
	Offset uint32
	Type   uint8
	_      [3]uint8
}

type BinModelDirectory struct {
	IndexTableCount      uint32
	IndexTableOffset     uint32
	NodeTableCount       uint32
	NodeTableOffset      uint32
	MeshTableCount       uint32
	MeshTableOffset      uint32
	MaterialTableCount   uint32
	MaterialTableOffset  uint32
	PrimitiveTableCount  uint32
	PrimitiveTableOffset uint32
	AccessorTableCount   uint32
	AccessorTableOffset  uint32
	SceneTableCount      uint32
	SceneTableOffset     uint32
}

type BinScene struct {
	Name       uint32
	NodesCount uint32
	Nodes      uint32
}

type BinAccessor struct {
	Name          uint32
	BufferOffset  uint32
	Count         uint32
	ComponentType uint8
	ElementType   uint8
	_             [2]uint8
}

type BinMaterial struct {
	Name                   uint32
	BaseColorTextureOffset uint32
	TexCoord               uint8
	Format                 uint8
	WrapS                  uint8
	WrapT                  uint8
}

type BinMeshPrimitive struct {
	AttrPos      uint32
	AttrNormal   uint32
	AttrTangent  uint32
	AttrSt0      uint32
	AttrSt1      uint32
	AttrVc0      uint32
	AttrJoints0  uint32
	AttrWeights0 uint32
	Indices      uint32
	Material     uint32
	Mode         uint8
	_            [3]uint8
}

type BinMesh struct {
	Name            uint32
	PrimitivesCount uint32
	Primitives      uint32
}

type BinNode struct {
	Name          uint32
	Rotation      [4]float32
	Scale         [3]float32
	Translation   [3]float32
	ChildrenCount uint32
	Children      uint32
	Mesh          uint32
}

type Model struct {
	Asset      *gltf.Document
	Directory  *BinModelDirectory
	IndexTable *[]uint32
}

type Pak struct {
	Buffer      *[]byte
	Directory   *[]BinDirectoryEntry
	StringTable *[]byte
}

func packScenes(model Model, pak Pak) (err error) {
	var scenes []BinScene = []BinScene{}

	for _, scene := range model.Asset.Scenes {
		name := uint32(len(*pak.StringTable))
		*pak.StringTable = append(*pak.StringTable, ToCString(scene.Name)...)

		nodes := uint32(len(*model.IndexTable))
		for _, node := range scene.Nodes {
			*model.IndexTable = append(*model.IndexTable, uint32(node))
		}

		scenes = append(scenes, BinScene{
			Name:       name,
			NodesCount: uint32(len(scene.Nodes)),
			Nodes:      nodes,
		})
	}

	*pak.Buffer, err = AlignPad(*pak.Buffer, 4)
	if err != nil {
		return err
	}
	model.Directory.SceneTableOffset = uint32(len(*pak.Buffer))
	model.Directory.SceneTableCount = uint32(len(scenes))
	*pak.Buffer = AppendOrPanic(*pak.Buffer, binary.BigEndian, scenes)

	return
}

func packNodes(model Model, pak Pak) (err error) {
	var nodes []BinNode = []BinNode{}

	for _, node := range model.Asset.Nodes {
		name := uint32(len(*pak.StringTable))
		*pak.StringTable = append(*pak.StringTable, ToCString(node.Name)...)

		children := uint32(len(*model.IndexTable))
		for _, child := range node.Children {
			*model.IndexTable = append(*model.IndexTable, uint32(child))
		}

		mesh := UINT32_MAX
		if node.Mesh != nil {
			mesh = uint32(*node.Mesh)
		}

		// TODO: Matrix decomposition
		rotation := node.RotationOrDefault()
		scale := node.ScaleOrDefault()
		translation := node.TranslationOrDefault()
		nodes = append(nodes, BinNode{
			Name: name,
			Rotation: [4]float32{
				float32(rotation[0]),
				float32(rotation[1]),
				float32(rotation[2]),
				float32(rotation[3]),
			},
			Scale: [3]float32{
				float32(scale[0]),
				float32(scale[1]),
				float32(scale[2]),
			},
			Translation: [3]float32{
				float32(translation[0]),
				float32(translation[1]),
				float32(translation[2]),
			},
			ChildrenCount: uint32(len(node.Children)),
			Children:      children,
			Mesh:          mesh,
		})
	}

	*pak.Buffer, err = AlignPad(*pak.Buffer, 4)
	if err != nil {
		return err
	}
	model.Directory.NodeTableOffset = uint32(len(*pak.Buffer))
	model.Directory.NodeTableCount = uint32(len(nodes))
	*pak.Buffer = AppendOrPanic(*pak.Buffer, binary.BigEndian, nodes)

	return
}

func packMeshes(model Model, pak Pak, idxs map[*gltf.Primitive]int) (err error) {
	var meshes []BinMesh = []BinMesh{}

	for _, mesh := range model.Asset.Meshes {
		name := uint32(len(*pak.StringTable))
		*pak.StringTable = append(*pak.StringTable, ToCString(mesh.Name)...)

		primitives := uint32(len(*model.IndexTable))
		for _, prim := range mesh.Primitives {
			idx, ok := idxs[prim]
			if !ok {
				return fmt.Errorf(`missing index for primitive %p`, prim)
			}
			*model.IndexTable = append(*model.IndexTable, uint32(idx))
		}

		meshes = append(meshes, BinMesh{
			Name:            name,
			PrimitivesCount: uint32(len(mesh.Primitives)),
			Primitives:      primitives,
		})
	}

	*pak.Buffer, err = AlignPad(*pak.Buffer, 4)
	if err != nil {
		return err
	}
	model.Directory.MeshTableOffset = uint32(len(*pak.Buffer))
	model.Directory.MeshTableCount = uint32(len(meshes))
	*pak.Buffer = AppendOrPanic(*pak.Buffer, binary.BigEndian, meshes)

	return
}

func packPrimitives(model Model, pak Pak) (idxs map[*gltf.Primitive]int, err error) {
	var primitives []BinMeshPrimitive = []BinMeshPrimitive{}

	/* qmuntal/gltf doesn't supply a document.MeshPrimitives table,
	so instead we must traverse all meshes and their primitives;
	since glTF spec allows primitives to be reused, keep track of each
	primitive to make sure we're not adding one that was already done.
	also store the index so it can be retrieved later from a *gltf.Primitive */
	idxs = map[*gltf.Primitive]int{}

	for _, mesh := range model.Asset.Meshes {
		for _, primitive := range mesh.Primitives {
			_, ok := idxs[primitive]
			if ok {
				continue
			}
			idxs[primitive] = len(idxs)

			/* supply a default index of n/a for each attribute (PAK specifies
			an index of UINT_MAX represents an index that is n/a), then
			for each attribute that actually exists in primitive.Attributes,
			assign the appropriate index */
			var attributes map[string]uint32 = map[string]uint32{
				"POSITION":   UINT32_MAX,
				"NORMAL":     UINT32_MAX,
				"TANGENT":    UINT32_MAX,
				"TEXCOORD_0": UINT32_MAX,
				"TEXCOORD_1": UINT32_MAX,
				"COLOR_0":    UINT32_MAX,
				"JOINTS_0":   UINT32_MAX,
				"WEIGHTS_0":  UINT32_MAX,
			}
			for name, idx := range primitive.Attributes {
				attributes[name] = uint32(idx)
			}
			var indices uint32 = UINT32_MAX
			if primitive.Indices != nil {
				indices = uint32(*primitive.Indices)
			}
			var material uint32 = UINT32_MAX
			if primitive.Material != nil {
				material = uint32(*primitive.Material)
			}

			/* the enum values provided by qmuntal/gltf
			do not correspond to the values in the gltf spec :(
			...map them back to the correct values */
			mode := map[gltf.PrimitiveMode]uint8{
				gltf.PrimitivePoints:        0,
				gltf.PrimitiveLines:         1,
				gltf.PrimitiveLineLoop:      2,
				gltf.PrimitiveLineStrip:     3,
				gltf.PrimitiveTriangles:     4,
				gltf.PrimitiveTriangleStrip: 5,
				gltf.PrimitiveTriangleFan:   6,
			}[primitive.Mode]

			primitives = append(primitives, BinMeshPrimitive{
				AttrPos:      attributes["POSITION"],
				AttrNormal:   attributes["NORMAL"],
				AttrTangent:  attributes["TANGENT"],
				AttrSt0:      attributes["TEXCOORD_0"],
				AttrSt1:      attributes["TEXCOORD_1"],
				AttrVc0:      attributes["COLOR_0"],
				AttrJoints0:  attributes["JOINTS_0"],
				AttrWeights0: attributes["WEIGHTS_0"],
				Indices:      indices,
				Material:     material,
				Mode:         mode,
			})
		}
	}

	*pak.Buffer, err = AlignPad(*pak.Buffer, 4)
	if err != nil {
		return nil, err
	}
	model.Directory.PrimitiveTableOffset = uint32(len(*pak.Buffer))
	model.Directory.PrimitiveTableCount = uint32(len(primitives))
	*pak.Buffer = AppendOrPanic(*pak.Buffer, binary.BigEndian, primitives)

	return
}

func toRGB5A3(im image.Image) (out []uint16) {
	const TILE_SIZE int = 4
	out = []uint16{}

	numTilesX := im.Bounds().Max.X / TILE_SIZE
	if im.Bounds().Max.X%TILE_SIZE != 0 {
		numTilesX++
	}
	numTilesY := im.Bounds().Max.Y / TILE_SIZE
	if im.Bounds().Max.Y%TILE_SIZE != 0 {
		numTilesY++
	}
	for tileX := range numTilesX {
		for tileY := range numTilesY {
			for texelX := range TILE_SIZE {
				for texelY := range TILE_SIZE {
					var texel uint16 = 0
					srcX := TILE_SIZE*tileX + texelX
					srcY := TILE_SIZE*tileY + texelY
					r, g, b, a := im.At(srcX, srcY).RGBA()
					/* squash from 0-65535 to 0-255 */
					r /= 256
					g /= 256
					b /= 256
					a /= 256

					/* squash from 0-255 to 0-(bit depth) */
					r /= 16
					g /= 16
					b /= 16
					a /= 32

					a <<= 12
					r <<= 8
					g <<= 4
					texel = uint16(a | r | g | b)
					out = append(out, texel)
				}
			}
		}
	}

	return
}

func packMaterials(model Model, pak Pak) (err error) {
	var materials []BinMaterial = []BinMaterial{}

	wrapMode := map[gltf.WrappingMode]uint8{
		gltf.WrapClampToEdge:    0,
		gltf.WrapMirroredRepeat: 1,
		gltf.WrapRepeat:         2,
	}
	for _, material := range model.Asset.Materials {
		var wrapS, wrapT uint8
		sampler := model.Asset.Textures[material.PBRMetallicRoughness.BaseColorTexture.Index].Sampler
		if sampler != nil {
			wrapS = wrapMode[model.Asset.Samplers[*sampler].WrapS]
			wrapT = wrapMode[model.Asset.Samplers[*sampler].WrapT]
		} else {
			/* glTF spec 5.29 : Texture.sampler: When undefined, a sampler
			with repeat wrapping and auto filtering SHOULD be used */
			wrapS = wrapMode[gltf.WrapRepeat]
			wrapT = wrapMode[gltf.WrapRepeat]
		}

		name := uint32(len(*pak.StringTable))
		*pak.StringTable = append(*pak.StringTable, ToCString(material.Name)...)

		*pak.Buffer, err = AlignPad(*pak.Buffer, 32)
		if err != nil {
			return err
		}
		texOffset := uint32(len(*pak.Buffer))
		texture := model.Asset.Textures[material.PBRMetallicRoughness.BaseColorTexture.Index]
		var source []byte
		if model.Asset.Images[*texture.Source].BufferView == nil {
			source, err = model.Asset.Images[*texture.Source].MarshalData()
			if err != nil {
				return fmt.Errorf(`could not unmarshal texture source image (%w)`, err)
			}
		} else {
			bv := model.Asset.BufferViews[*model.Asset.Images[*texture.Source].BufferView]
			source = model.Asset.Buffers[bv.Buffer].Data[bv.ByteOffset : bv.ByteOffset+bv.ByteLength]
		}
		im, _, err := image.Decode(bytes.NewBuffer(source))
		if err != nil {
			return fmt.Errorf(`could not decode image (%w)`, err)
		}
		*pak.Buffer = AppendOrPanic(*pak.Buffer, binary.BigEndian, toRGB5A3(im))

		materials = append(materials, BinMaterial{
			Name:                   name,
			BaseColorTextureOffset: texOffset,
			TexCoord:               uint8(material.PBRMetallicRoughness.BaseColorTexture.TexCoord),
			Format:                 uint8(8), // RGB5A3. TODO: support different formats
			WrapS:                  wrapS,
			WrapT:                  wrapT,
		})
	}

	*pak.Buffer, err = AlignPad(*pak.Buffer, 4)
	if err != nil {
		return err
	}
	model.Directory.MaterialTableOffset = uint32(len(*pak.Buffer))
	model.Directory.MaterialTableCount = uint32(len(materials))
	*pak.Buffer = AppendOrPanic(*pak.Buffer, binary.BigEndian, materials)

	return
}

func getAccessorContents(doc *gltf.Document, acr *gltf.Accessor) (contents any, err error) {
	contents, err = modeler.ReadAccessor(doc, acr, nil)
	if err != nil {
		return nil, fmt.Errorf(`failed to read accessor (%w)`, err)
	}
	// glTF matrices are column-major; must transpose to row-major matrices
	if acr.Type == gltf.AccessorMat2 || acr.Type == gltf.AccessorMat3 || acr.Type == gltf.AccessorMat4 {
		c, t, count := gltf_binary.Type(contents)
		transposed, _ := gltf_binary.MakeSlice(c, t, count)

		for mat := range reflect.ValueOf(contents).Len() {
			dimensions := reflect.ValueOf(contents).Index(mat).Len() // can assume matrix is square
			for col := range dimensions {
				for row := range dimensions {
					reflect.ValueOf(transposed).Index(row).Index(col).Set(reflect.ValueOf(transposed).Index(col).Index(row))
				}
			}
		}

		contents = transposed
	}
	return
}

func packAccessors(model Model, pak Pak) (err error) {
	var accessors []BinAccessor = []BinAccessor{}

	for _, accessor := range model.Asset.Accessors {
		var elementType uint8
		switch accessor.Type {
		case gltf.AccessorScalar:
			elementType = 0
		case gltf.AccessorVec2:
			elementType = 1
		case gltf.AccessorVec3:
			elementType = 2
		case gltf.AccessorVec4:
			elementType = 3
		case gltf.AccessorMat2:
			elementType = 4
		case gltf.AccessorMat3:
			elementType = 5
		case gltf.AccessorMat4:
			elementType = 6
		}

		name := uint32(uint32(len(*pak.StringTable)))
		*pak.StringTable = append(*pak.StringTable, ToCString(accessor.Name)...)

		*pak.Buffer, err = AlignPad(*pak.Buffer, 4)
		if err != nil {
			return err
		}
		bufferOffset := uint32(len(*pak.Buffer))
		contents, err := getAccessorContents(model.Asset, accessor)
		if err != nil {
			return err
		}
		*pak.Buffer = AppendOrPanic(*pak.Buffer, binary.BigEndian, contents)

		accessors = append(accessors, BinAccessor{
			Name:          name,
			BufferOffset:  bufferOffset,
			Count:         uint32(accessor.Count),
			ComponentType: uint8(accessor.ComponentType - 5120),
			ElementType:   elementType,
		})
	}

	*pak.Buffer, err = AlignPad(*pak.Buffer, 4)
	if err != nil {
		return err
	}
	model.Directory.AccessorTableOffset = uint32(len(*pak.Buffer))
	model.Directory.AccessorTableCount = uint32(len(accessors))
	*pak.Buffer = AppendOrPanic(*pak.Buffer, binary.BigEndian, accessors)

	return
}

func PackLevel(level Level, root string) (buf []byte, err error) {
	pak := Pak{
		Buffer:      &buf,
		Directory:   &[]BinDirectoryEntry{},
		StringTable: &[]byte{},
	}

	header := BinPakHeader{
		Signature:         1328676866,
		StringTableLength: 0,
		StringTableOffset: 0,
		DirectoryCount:    0,
		DirectoryOffset:   0,
	}
	buf = AppendOrPanic(buf, binary.BigEndian, header)

	for _, path := range level.GLTF {
		fmt.Println("Packing", path)
		asset, err := gltf.Open(filepath.Join(root, "assets", path))
		if err != nil {
			return nil, fmt.Errorf(`failed to open or load glTF file "%s" (%w)`, path, err)
		}

		model := Model{
			Asset:      asset,
			Directory:  new(BinModelDirectory),
			IndexTable: &[]uint32{},
		}

		err = packAccessors(model, pak)
		if err != nil {
			return nil, fmt.Errorf(`failed to pack accessors (%w)`, err)
		}
		err = packMaterials(model, pak)
		if err != nil {
			return nil, fmt.Errorf(`failed to pack materials (%w)`, err)
		}
		idxs, err := packPrimitives(model, pak)
		if err != nil {
			return nil, fmt.Errorf(`failed to pack mesh primitives (%w)`, err)
		}
		err = packMeshes(model, pak, idxs)
		if err != nil {
			return nil, fmt.Errorf(`failed to pack meshes (%w)`, err)
		}
		err = packNodes(model, pak)
		if err != nil {
			return nil, fmt.Errorf(`failed to pack nodes (%w)`, err)
		}
		err = packScenes(model, pak)
		if err != nil {
			return nil, fmt.Errorf(`failed to pack scenes (%w)`, err)
		}

		buf, err = AlignPad(*pak.Buffer, 4)
		if err != nil {
			return nil, err
		}
		model.Directory.IndexTableCount = uint32(len(*model.IndexTable))
		model.Directory.IndexTableOffset = uint32(len(buf))
		buf = AppendOrPanic(buf, binary.BigEndian, *model.IndexTable)

		entry := BinDirectoryEntry{
			Name:   uint32(len(*pak.StringTable)),
			Offset: uint32(len(buf)),
			Type:   0,
		}
		*pak.Directory = append(*pak.Directory, entry)
	}

	buf, err = AlignPad(buf, 4)
	if err != nil {
		return nil, err
	}
	header.StringTableLength = uint32(len(*pak.StringTable))
	header.StringTableOffset = uint32(len(buf))
	buf = AppendOrPanic(buf, binary.BigEndian, pak.StringTable)

	buf, err = AlignPad(buf, 4)
	if err != nil {
		return nil, err
	}
	header.DirectoryCount = uint32(len(*pak.Directory))
	header.DirectoryOffset = uint32(len(buf))
	buf = AppendOrPanic(buf, binary.BigEndian, *pak.Directory)

	// overwrite placeholder values in header
	AppendOrPanic(buf[:0], binary.BigEndian, header)

	return
}
