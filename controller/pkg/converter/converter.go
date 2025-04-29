// TODO: add Doc comments
package converter

import (
	"errors";
	"encoding/binary";
)

const (
	intByteLen = 8
)

func IntToByteSlice(num int) []byte {
    buf := make([]byte, 8)
    binary.LittleEndian.PutUint64(buf, uint64(num))
    return buf
}

func ByteSliceToInt(slice []byte) (int, error) {
    if len(slice) != 8 {
        return 0, errors.New("invalid slice length")
    }
    return int(binary.LittleEndian.Uint64(slice)), nil
}
