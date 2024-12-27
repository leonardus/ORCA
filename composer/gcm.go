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
	"encoding/binary"
	"fmt"
	"io/fs"
	"os"
	"path/filepath"
)

/*
 * info sourced from:
 * https://github.com/emukidid/swiss-gc/blob/master/cube/swiss/include/gcm.h
 * and YAGCD
 */

const GCM_MAGIC uint32 = 0xC2339F3D
const REGION_CODE_JP uint32 = 0
const REGION_CODE_US uint32 = 1
const REGION_CODE_EU uint32 = 2

type DiscID struct {
	ConsoleID      uint8
	Gamecode       [2]uint8
	CountryCode    uint8
	MakerCode      [2]uint8
	DiscID         uint8
	Version        uint8
	AudioStreaming uint8
	StreamBufSize  uint8
	_              [18]uint8
	DVDMagicWord   uint32
	GameName       [64]uint8
	_              [928]uint8
}

// ignored by retail IPL?
type BB1 struct {
	ApploaderSize  uint32
	ApploaderFunc1 uint32
	ApploaderFunc2 uint32
	ApploaderFunc3 uint32
	_              [16]uint8
}

type BB2 struct {
	DOLOffset  uint32
	FSTOffset  uint32
	FSTSize    uint32
	MaxFSTSize uint32
	FSTAddress uint32 // ignored by retail-compatible apploaders
	UserPos    uint32 // ignored by retail-compatible apploaders
	UserLength uint32 // ignored by retail-compatible apploaders
	_          uint32
}

type BI2 struct {
	DebugMonSize uint32
	SimMemSize   uint32
	ArgOffset    uint32
	DebugFlag    uint32
	TRKLocation  uint32
	TRKSize      uint32
	RegionCode   uint32
	TotalDisc    uint32
	LongFileName uint32
	PADSpec      uint32
	DOLLimit     uint32
	_            [8148]uint8
}

type DiscSystemArea struct {
	DiscID DiscID
	// 0x0400
	BB1 BB1
	// 0x0420
	BB2 BB2
	// 0x0440
	BI2 BI2
}

type FSTEntry struct {
	Ident uint32 /* byte 1    : flags; 0: file 1: directory
	   bytes 2-4 : filename, offset into string table */
	Offset uint32 // file_offset or parent_offset (dir)
	Length uint32 // file_length or num_entries (root) or next_offset (dir)
}

type FST struct {
	StringTable []byte
	FilesBin    []byte
	Entries     []FSTEntry
}

type GameID struct {
	ConsoleID   uint8
	Gamecode    [2]uint8
	CountryCode uint8
	MakerCode   [2]uint8
	Version     uint8
	GameName    [64]uint8
}

func recurseFST(state *FST, file os.DirEntry, path string, parentIdx int) (err error) {
	entryIdx := len(state.Entries)
	state.Entries = append(state.Entries, FSTEntry{})

	stringIdx := uint32(len(state.StringTable))
	state.StringTable = append(state.StringTable, ToCString(file.Name())...)
	if len(state.StringTable) > 16777216 {
		return fmt.Errorf(`string table grew past 2^24 bytes`)
	}
	state.Entries[entryIdx].Ident = stringIdx

	if file.IsDir() {
		dir, err := os.ReadDir(path)
		if err != nil {
			return err
		}
		for _, child := range dir {
			err = recurseFST(state, child, filepath.Join(path, child.Name()), entryIdx)
			if err != nil {
				return err
			}
		}

		state.Entries[entryIdx].Ident |= (uint32(1) << 24)
		state.Entries[entryIdx].Offset = uint32(parentIdx)
		state.Entries[entryIdx].Length = uint32(len(state.Entries) - entryIdx - 1)
	} else {
		// all files on disk aligned to 4B
		state.FilesBin, err = AlignPad(state.FilesBin, 4)
		if err != nil {
			return err
		}

		offset := uint32(len(state.FilesBin))
		contents, err := os.ReadFile(path)
		if err != nil {
			return fmt.Errorf(`failed to read file "%s"`, path)
		}
		state.FilesBin = append(state.FilesBin, contents...)

		state.Entries[entryIdx].Offset = offset
		state.Entries[entryIdx].Length = uint32(len(contents))
	}

	return
}

func buildFST(rootPath string) (fst *FST, err error) {
	fmt.Println("Building FST")

	dir, err := os.Stat(rootPath)
	if err != nil {
		return nil, fmt.Errorf(`failed to stat FST root dir (%w)`, err)
	}
	state := new(FST)
	err = recurseFST(state, fs.FileInfoToDirEntry(dir), rootPath, 0)
	if err != nil {
		return nil, fmt.Errorf(`failed to build FST (%w)`, err)
	}

	state.Entries[0].Length += 1 // FST root entry includes itself in length

	state.StringTable, err = AlignPad(state.StringTable, 4)
	if err != nil {
		return nil, err
	}

	return state, nil
}

// correct file contents DVD offset for each entry
func patchFSTAddrs(fstState FST, filesOffset int) {
	for i := range fstState.Entries {
		if fstState.Entries[i].Ident>>24 != 1 { // 1 = directory, 0 = file
			fstState.Entries[i].Offset += uint32(filesOffset)
		}
	}
}

func GCM(id GameID, apploaderPath string, dolPath string, fstRootPath string) (gcm []byte, err error) {
	fmt.Println("Building GCM disc image")

	gcm = AppendOrPanic(gcm, binary.BigEndian, DiscSystemArea{})

	apploader, err := os.ReadFile(apploaderPath)
	if err != nil {
		return nil, err
	}
	gcm = AppendOrPanic(gcm, binary.BigEndian, apploader)

	fstState, err := buildFST(fstRootPath)
	if err != nil {
		return nil, err
	}
	fstSize := PackedSize(fstState.Entries) + len(fstState.StringTable)

	/*
	 * Game files are placed sequentially, directly after the FST.
	 * This is fine for random access storage such as SD cards, but
	 * could probably be optimized for optical media to determine file
	 * placement according to location on disc.
	 */
	patchFSTAddrs(*fstState, len(gcm)+fstSize)

	fstOffset := len(gcm)
	gcm = AppendOrPanic(gcm, binary.BigEndian, fstState.Entries)
	gcm = AppendOrPanic(gcm, binary.BigEndian, fstState.StringTable)
	gcm = AppendOrPanic(gcm, binary.BigEndian, fstState.FilesBin)

	dolOffset := len(gcm)
	dol, err := os.ReadFile(dolPath)
	if err != nil {
		return nil, err
	}
	gcm = AppendOrPanic(gcm, binary.BigEndian, dol)

	/*
	 * Region locking code is derived from country code in game ID.
	 * Defaults to USA if no match is found .
	 */
	regionCode, ok := map[uint8]uint32{
		uint8('D'): REGION_CODE_EU, // Germany
		uint8('E'): REGION_CODE_US, // USA
		uint8('F'): REGION_CODE_EU, // France
		uint8('H'): REGION_CODE_EU, // Netherlands / Europe alternate languages
		uint8('I'): REGION_CODE_EU, // Italy
		uint8('J'): REGION_CODE_JP, // Japan
		uint8('K'): REGION_CODE_JP, // Korea    *TODO: Is this correct?
		uint8('L'): REGION_CODE_EU, // Japanese import to Europe, Australia and other PAL regions
		uint8('M'): REGION_CODE_EU, // American import to Europe, Australia and other PAL regions
		uint8('N'): REGION_CODE_EU, // Japanese import to USA and other NTSC regions
		uint8('P'): REGION_CODE_EU, // Europe and other PAL regions such as Australia
		uint8('R'): REGION_CODE_EU, // Russia
		uint8('S'): REGION_CODE_EU, // Spain
		uint8('U'): REGION_CODE_EU, // Australia / Europe alternate languages
		uint8('V'): REGION_CODE_EU, // Scandinavia
		uint8('W'): REGION_CODE_EU, // Republic of China (Taiwan) / Hong Kong / Macau
	}[id.CountryCode]
	if !ok {
		regionCode = REGION_CODE_US
	}

	AppendOrPanic(gcm[:0], binary.BigEndian, DiscSystemArea{
		DiscID: DiscID{
			ConsoleID:      id.ConsoleID,
			Gamecode:       id.Gamecode,
			CountryCode:    id.CountryCode,
			MakerCode:      id.MakerCode,
			DiscID:         0,
			Version:        id.Version,
			AudioStreaming: 0,
			StreamBufSize:  0,
			DVDMagicWord:   GCM_MAGIC,
			GameName:       id.GameName,
		},
		BB1: BB1{}, // Ignored by retail IPL?
		BB2: BB2{
			DOLOffset:  uint32(dolOffset),
			FSTOffset:  uint32(fstOffset),
			FSTSize:    uint32(fstSize),
			MaxFSTSize: uint32(fstSize),
		},
		BI2: BI2{
			DebugMonSize: 0,
			SimMemSize:   0,
			ArgOffset:    0,
			DebugFlag:    0,
			TRKLocation:  0,
			TRKSize:      0,
			RegionCode:   regionCode,
			TotalDisc:    1,
			LongFileName: 1, // ignored?
			PADSpec:      6,
			DOLLimit:     0,
		},
	})

	return
}
