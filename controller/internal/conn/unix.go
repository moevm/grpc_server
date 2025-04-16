// Wrapper for [net.Conn] with Unix socket type
//
// [net.Conn]: https://pkg.go.dev/net#Conn
package conn

import (
	"errors"
	"fmt"
	"net"

	"github.com/moevm/grpc_server/pkg/converter"
)

const (
	intByteLen = 8 // default for x86_64
)

// TODO: add constructor for Unix

// conn.Unix embeds net.Conn
type Unix struct {
	net.Conn
}

// TODO: add a detailed Doc comments for Unix.Read and Unix.Write

// Read reads all the contents from channel into the buffer
// if the buffer size is too small it will return an error
// (that's why you need to pass the message size first,
// and only then the message itself).
func (c *Unix) Read(b []byte) (n int, err error) {
	messageLenBuf := make([]byte, intByteLen)

	// Read message len.
	_, err = c.Conn.Read(messageLenBuf)
	if err != nil {
		return 0, fmt.Errorf("Unix.Read - c.Conn.Read: %v", err)
	}

	messageLen, err := converter.ByteSliceToInt(messageLenBuf)
	if err != nil {
		return 0, fmt.Errorf("Unix.Read - converter.ByteSliceToInt: %v", err)
	}
	// Check buffer len.
	if messageLen > len(b) {
		return 0, errors.New("invalid buffer size: buffer too small")
	}
	// Allocate slice for full message.
	message := make([]byte, 0, messageLen)

	// Read frame by frame the message.
	for {
		frameLenBuf := make([]byte, intByteLen)
		frameLen := 0

		// Read frame len.
		_, err = c.Conn.Read(frameLenBuf)
		if err != nil {
			return 0, fmt.Errorf("Unix.Read - c.Conn.Read: %v", err)
		}

		frameLen, err = converter.ByteSliceToInt(frameLenBuf)
		if err != nil {
			return 0, fmt.Errorf("Unix.Read - converter.ByteSliceToInt: %v", err)
		}

		switch {
		case frameLen > 0:
			frame := make([]byte, frameLen)

			_, err = c.Conn.Read(frame)
			if err != nil {
				return 0, fmt.Errorf("Unix.Read - c.Conn.Read: %v", err)
			}

			message = append(message, frame...)

		// frameLen == 0 means that the message has been transmitted in full.
		case frameLen == 0:
			// Write message into buffer
			// (without re-allocate).
			b = b[:0]
			b = append(b, message...)

			return len(message), nil

		case frameLen < 0:
			return 0, errors.New("invalid frame len")
		}
	}
}

func (c *Unix) Write(b []byte) (n int, err error) {
	messageLen := len(b)
	messageLenBuf := converter.IntToByteSlice(messageLen)

	_, err = c.Conn.Write(messageLenBuf)
	if err != nil {
		return 0, fmt.Errorf("Unix.Write - c.Conn.Write: %v", err)
	}

	const fullFrameLen = 1024
	fullFrameLenBuf := converter.IntToByteSlice(fullFrameLen)
	offset := 0

	for {
		switch {
		case messageLen-offset >= fullFrameLen:
			_, err = c.Conn.Write(fullFrameLenBuf)
			if err != nil {
				return 0, fmt.Errorf("Unix.Write - c.Conn.Write: %v", err)
			}

			_, err = c.Conn.Write(b[offset : offset+fullFrameLen])
			if err != nil {
				return 0, fmt.Errorf("Unix.Write - c.Conn.Write: %v", err)
			}

			offset += fullFrameLen

		case (messageLen-offset < fullFrameLen) && (messageLen > offset):
			frameLen := messageLen - offset
			frameLenBuf := converter.IntToByteSlice(frameLen)

			_, err = c.Conn.Write(frameLenBuf)
			if err != nil {
				return 0, fmt.Errorf("Unix.Write - c.Conn.Write: %v", err)
			}

			_, err = c.Conn.Write(b[offset : offset+frameLen])
			if err != nil {
				return 0, fmt.Errorf("Unix.Write - c.Conn.Write: %v", err)
			}

			offset += frameLen

		case messageLen == offset:
			_, err := c.Conn.Write(converter.IntToByteSlice(0))
			if err != nil {
				return 0, fmt.Errorf("Unix.Write - c.Conn.Write: %v", err)
			}

			return offset, nil
		}
	}
}
