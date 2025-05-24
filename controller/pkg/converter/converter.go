// Package converter implement utility for converting int to byte slice
// and byte slice to int.
package converter

import (
	"encoding/binary"
	"errors"
)

// IntToByteSlice convert int (8 byte) to byte slice (byte[], 8).
func IntToByteSlice(num int) []byte {
	buf := make([]byte, 8)
	binary.LittleEndian.PutUint64(buf, uint64(num))
	return buf
}

// ByteSliceToInt convert byte slice (byte[], 8) to int (8 byte).
func ByteSliceToInt(slice []byte) (int, error) {
	if len(slice) != 8 {
		return 0, errors.New("invalid slice length")
	}
	return int(binary.LittleEndian.Uint64(slice)), nil
}
