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
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"net"
)

// TODO: add constructor for Unix

// conn.Unix embeds [net.Conn]
type Unix struct {
	// You can see all interface methods here:
	//
	// [net.Conn]: https://pkg.go.dev/net#Conn
	net.Conn
}

// ReadMessage reads a complete length-prefixed message from the connection,
// allocating and returning the buffer automatically.
// See "ABI" section in wiki/worker_communication_protocol.md
func (c *Unix) ReadMessage() ([]byte, error) {
	var lenBuf [8]byte

	if err := c.readExact(lenBuf[:]); err != nil {
		if errors.Is(err, io.EOF) {
			return nil, io.ErrUnexpectedEOF
		}
		return nil, fmt.Errorf("read message length: %w", err)
	}
	messageLen := binary.LittleEndian.Uint64(lenBuf[:])

	buf := make([]byte, messageLen)

	if err := c.readExact(buf); err != nil {
		return nil, fmt.Errorf("read message content: %w", err)
	}

	return buf, nil
}

// WriteMessage writes a complete length-prefixed message to the connection.
// See "ABI" section in wiki/worker_communication_protocol.md
func (c *Unix) WriteMessage(b []byte) error {
	var lenBuf [8]byte
	messageLen := uint64(len(b))

	binary.LittleEndian.PutUint64(lenBuf[:], messageLen)
	if err := c.writeExact(lenBuf[:]); err != nil {
		return fmt.Errorf("write message length: %w", err)
	}

	if err := c.writeExact(b); err != nil {
		return fmt.Errorf("write message content: %w", err)
	}

	return nil
}

// readExact reads exactly len(buf) bytes from the connection.
// If data is not fully transmitted yet, it reads what is availible and waits for the next chunk.
func (c *Unix) readExact(buf []byte) error {
	totalRead := 0
	for totalRead < len(buf) {
		n, err := c.Conn.Read(buf[totalRead:])
		if err != nil {
			return err
		}
		totalRead += n
	}
	return nil
}

// writeExact writes exactly len(buf) bytes to the connection
// If data cannot be written in one chunk, it writes it in several chunks.
func (c *Unix) writeExact(buf []byte) error {
	totalWritten := 0
	for totalWritten < len(buf) {
		n, err := c.Conn.Write(buf[totalWritten:])
		if err != nil {
			return err
		}
		totalWritten += n
	}
	return nil
}
