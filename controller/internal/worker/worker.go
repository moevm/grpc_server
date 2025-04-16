package worker

import (
	"crypto/md5"
	"fmt"
	"log"
	"net"
	"os"
	"os/signal"
	"strings"
	"syscall"

	"github.com/moevm/grpc_server/internal/conn"
	"github.com/moevm/grpc_server/pkg/converter"
)

const (
	workerInitSocketPath = "/run/controller/init.sock"
	workerSocketPath     = "/run/controller/"

	intByteLen = 8

	successfulResp = 1
)

var (
	socketPath strings.Builder

	listener net.Listener

	id int
)

func doTask(connection conn.Unix) {
	fmt.Println("Worker started performing the task... ")

	taskLenBuf := make([]byte, intByteLen)

	_, err := connection.Read(taskLenBuf)
	if err != nil {
		log.Panic(err)
	}

	taskLen, err := converter.ByteSliceToInt(taskLenBuf)
	if err != nil {
		log.Panic(err)
	}

	task := make([]byte, taskLen)

	_, err = connection.Read(task)
	if err != nil {
		log.Panic()
	}

	md5sum := md5.Sum(task)

	responseLen := len(md5sum)

	_, err = connection.Write(converter.IntToByteSlice(responseLen))
	if err != nil {
		log.Panic(err)
	}

	_, err = connection.Write(md5sum[:])
	if err != nil {
		log.Panic(err)
	}

	fmt.Println("Task complete")
}

func init() {
	channel := make(chan os.Signal, 10)
	signal.Notify(channel, os.Interrupt, syscall.SIGINT, syscall.SIGTERM)

	go func() {
		<-channel

		listener.Close()
		os.RemoveAll(workerInitSocketPath)
		os.RemoveAll(socketPath.String())

		os.Exit(1)
	}()
}

func Start() {
	idBuff := make([]byte, intByteLen)

	netConn, err := net.Dial("unix", workerInitSocketPath)
	if err != nil {
		log.Panic(err)
	}

	connection := conn.Unix{Conn: netConn}
	// Get worker id.
	_, err = connection.Read(idBuff)
	if err != nil {
		log.Panic(err)
	}

	id, err := converter.ByteSliceToInt(idBuff)
	if err != nil {
		log.Panic(err)
	}

	socketPath.WriteString(workerSocketPath)
	socketPath.WriteString(fmt.Sprintf("%v.sock", id))

	err = os.RemoveAll(socketPath.String())
	if err != nil {
		log.Panic(err)
	}

	listener, err = net.Listen("unix", socketPath.String())
	if err != nil {
		log.Panic(err)
	}
	// Send succ response.
	response := converter.IntToByteSlice(successfulResp)

	_, err = connection.Write(response)
	if err != nil {
		log.Panic(err)
	}

	connection.Close()

	fmt.Printf("Worker %v init successfully\n", id)

	// Waiting conn accept.
	for {
		netConn, err := listener.Accept()
		if err != nil {
			log.Panic(err)
		}

		connection := conn.Unix{Conn: netConn}

		doTask(connection)

		connection.Close()
	}
}
