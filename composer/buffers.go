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
	"syscall"
)

// takes a byte slice `buf` and integer `alignment`, and returns a byte slice
// of `buf`, with additional NULL padding at the end to make its length a
// multiple of `alignment`
func AlignPad(buf []byte, alignment uint) (out []byte, err error) {
	var padLen uint
	if uint(len(buf))%alignment == 0 { // already aligned
		padLen = 0
	} else {
		padLen = alignment - uint(len(buf))%alignment
	}

	out, err = binary.Append(buf, binary.NativeEndian, make([]byte, padLen))
	if err != nil {
		err = fmt.Errorf(`failed to pad buffer (%w)`, err)
	}
	return
}

func ToCString(s string) []byte {
	arr, err := syscall.ByteSliceFromString(s)
	if err != nil {
		panic(err)
	}
	return arr
}

func AppendOrPanic(in []byte, order binary.ByteOrder, data any) []byte {
	out, err := binary.Append(in, order, data)
	if err != nil {
		panic(err)
	}
	return out
}

func PackedSize(data any) int {
	return len(AppendOrPanic(nil, binary.BigEndian, data))
}
