package converter

func ByteSliceToInt(slice []byte) int {
	var num int
	for i := 0; i < 8; i += 1 {
		num += int(slice[7-i])
		if i != 7 {
			num = num << 8
		}
	}
	return num
}

func IntToByteSlice(num int) []byte {
	slice := []byte{}
	for i := 0; i < 8; i += 1 {
		slice = append(slice, byte(num>>(i*8)))
	}
	return slice
}
