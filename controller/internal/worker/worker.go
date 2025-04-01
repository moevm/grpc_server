package worker

import (
	"crypto/md5"
	"crypto/rand"
	"fmt"
	"github.com/moevm/grpc_server/pkg/converter"
	"log"
	"net"
	"os"
	"os/signal"
	"strings"
	"syscall"
)

var SocketPath strings.Builder

var Listener net.Listener

func DoTask(c net.Conn) {
	fmt.Println("worker started performing the task...")
	buf := make([]byte, 8)
	_, err := c.Read(buf)
	if err != nil {
		log.Fatalln(err)
	}
	taskSize := converter.ByteSliceToInt(buf)
	task := make([]byte, 0, taskSize)
	for flag := 1; flag != 0; {
		buf := make([]byte, 8)
		_, err := c.Read(buf)
		if err != nil {
			log.Fatalln(err)
		}
		frameLen := converter.ByteSliceToInt(buf)
		switch {
		case frameLen > 0:
			buf := make([]byte, frameLen)
			_, err := c.Read(buf)
			if err != nil {
				log.Fatalln(err)
			}
			task = append(task, buf...)
		case frameLen == 0:
			flag = 0
		}
	}
	md5sum := md5.Sum(task)
	_, err = c.Write(converter.IntToByteSlice(len(md5sum[:])))
	if err != nil {
		log.Fatalln(err)
	}
	_, err = c.Write(md5sum[:])
	if err != nil {
		log.Fatalln(err)
	}
	fmt.Println("task complete")
}

func WorkerStart() {
	idBuff := make([]byte, 8)
	respBuf := make([]byte, 8)
	var err error
	var conn net.Conn
	_, err = rand.Read(idBuff)
	if err != nil {
		log.Fatalln(err)
	}
	id := converter.ByteSliceToInt(idBuff)
	SocketPath.WriteString("/tmp/socket/")
	SocketPath.WriteString(fmt.Sprintf("%v.sock", id))
	if err = os.RemoveAll(SocketPath.String()); err != nil {
		log.Fatalln(err)
	}
	Listener, err = net.Listen("unix", SocketPath.String())
	if err != nil {
		log.Fatalln(err)
	}
	conn, err = net.Dial("unix", "/tmp/socket/init.sock")
	if err != nil {
		log.Fatalln(err)
	}
	_, err = conn.Write(idBuff)
	if err != nil {
		log.Fatalln(err)
	}
	_, err = conn.Read(respBuf)
	if err != nil {
		log.Fatalln(err)
	}
	conn.Close()
	if converter.ByteSliceToInt(respBuf) != 1 {
		log.Fatalln("worker init error")
	} else {
		fmt.Println("worker init soccessfully")
	}
	fmt.Println("worker start listen...")
	for {
		conn, err := Listener.Accept()
		if err != nil {
			log.Fatalln(err)
		}
		DoTask(conn)
		conn.Close()
	}
}

func init() {
	c := make(chan os.Signal, 10)
	signal.Notify(c, os.Interrupt, syscall.SIGTERM)
	go func() {
		<-c
		Listener.Close()
		os.RemoveAll(SocketPath.String())
		os.Exit(1)
	}()
}
