// The package implements an interface for working with Unix sockets.
//
// The conn package is wrapper for [net.Conn] interface
// it should only be used for reading and writing from socket.
// The package fully preserves all net.Conn functions,
// so you should control writing to and reading from the socket manually.
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

// conn.Unix embeds [net.Conn]
type Unix struct {
	// You can see all interface methods here:
	//
	// [net.Conn]: https://pkg.go.dev/net#Conn
	net.Conn
}

// Read reads all the contents from channel into the buffer
// if the buffer size is too small it will return an error
// (that's why you need to manually pass the message size first,
// and only then the message itself).
//
// Initially it reads the full length of the message (8 bytes).
// Then it sequentially reads the frame length (8 bytes).
// and the frame itself (<= 1024 bytes).
// Reading ends when frame length 0 is transmitted.
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
	// Check buffer size.
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
			// (without re-allocate!).
			if len(b) < len(message) {
				return 0, errors.New("invalid buffer size: buffer too small")
			}
			n = copy(b, message)
			return n, nil

		case frameLen < 0:
			return 0, errors.New("invalid frame len")
		}
	}
}

// Write writes all contents from the buffer into the channel
// (you need to manually pass the message size first,
// and only then the message itself).
//
// First, the full message length (8 bytes) is written to the channel.
// Then, the frame length (8 bytes) is written sequentially
// and the frame itself (<= 1024 bytes).
// The writing ends when the offset points to the end of the buffer.
// The end of a message is indicated by the transmission of a zero buffer length.
func (c *Unix) Write(b []byte) (n int, err error) {
	messageLen := len(b)
	messageLenBuf := converter.IntToByteSlice(messageLen)

	// Send full message len.
	_, err = c.Conn.Write(messageLenBuf)
	if err != nil {
		return 0, fmt.Errorf("Unix.Write - c.Conn.Write: %v", err)
	}
	// Maximum frame length is 1024 bytes.
	const fullFrameLen = 1024
	fullFrameLenBuf := converter.IntToByteSlice(fullFrameLen)
	// Offset from the top of the message buffer.
	offset := 0

	for {
		switch {
		case messageLen-offset >= fullFrameLen:
			// Send full frame len.
			_, err = c.Conn.Write(fullFrameLenBuf)
			if err != nil {
				return 0, fmt.Errorf("Unix.Write - c.Conn.Write: %v", err)
			}
			// Send frame.
			_, err = c.Conn.Write(b[offset : offset+fullFrameLen])
			if err != nil {
				return 0, fmt.Errorf("Unix.Write - c.Conn.Write: %v", err)
			}

			offset += fullFrameLen

		case (messageLen-offset < fullFrameLen) && (messageLen > offset):
			frameLen := messageLen - offset
			frameLenBuf := converter.IntToByteSlice(frameLen)

			// Send frame len.
			_, err = c.Conn.Write(frameLenBuf)
			if err != nil {
				return 0, fmt.Errorf("Unix.Write - c.Conn.Write: %v", err)
			}
			// Send frame.
			_, err = c.Conn.Write(b[offset : offset+frameLen])
			if err != nil {
				return 0, fmt.Errorf("Unix.Write - c.Conn.Write: %v", err)
			}

			offset += frameLen

		case messageLen == offset:
			// Zero frame len means the message was successfully transmitted.
			_, err := c.Conn.Write(converter.IntToByteSlice(0))
			if err != nil {
				return 0, fmt.Errorf("Unix.Write - c.Conn.Write: %v", err)
			}

			return offset, nil
		}
	}
}
