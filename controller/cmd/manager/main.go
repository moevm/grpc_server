package main

import (
	"crypto/rand"
	"log"

	"github.com/moevm/grpc_server/internal/manager"
)

func main() {
	tasks := make([][]byte, 5)
	tasks[0] = make([]byte, 30)
	tasks[1] = make([]byte, 188588)
	tasks[2] = make([]byte, 5138788)
	tasks[3] = make([]byte, 17338664)
	tasks[4] = make([]byte, 55777333)
	for i := range tasks {
		_, err := rand.Read(tasks[i])
		if err != nil {
			log.Panic(err)
		}
	}

	manager.ClusterInit(tasks)
	select {}
}
