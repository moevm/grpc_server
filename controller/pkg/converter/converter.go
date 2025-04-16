// TODO: add Doc comments
package converter

import (
	"errors"
)

const (
	intByteLen = 8
)

// TODO: use generic for return/parameter type
func ByteSliceToInt(slice []byte) (num int, err error) {
	if len(slice) != intByteLen {
		return 0, errors.New("invalid slice len")
	}

	for i := range intByteLen {
		num += int(slice[intByteLen-i-1])

		if i != intByteLen-1 {
			num = num << intByteLen
		}
	}

	return num, nil
}

func IntToByteSlice(num int) []byte {
	slice := []byte{}

	for i := range intByteLen {
		slice = append(slice, byte(num>>(i*intByteLen)))
	}

	return slice
}
