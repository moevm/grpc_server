package binary

import (
	"io"
	"log"
	"os"
)

func Read(filepath string) []byte {
	f, err := os.Open(filepath)
	if err != nil {
		log.Fatalln(err)
	}
	defer f.Close()
	dump := make([]byte, 0)
	for {
		buff := make([]byte, 1024)
		_, err := f.Read(buff)
		dump = append(dump, buff...)
		if err == io.EOF {
			break
		}
	}
	return dump
}
