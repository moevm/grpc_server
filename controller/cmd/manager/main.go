package main

import (
	"crypto/rand"
	"github.com/moevm/grpc_server/internal/manager"
	"log"
)

func main() {
	tasks := make([][]byte, 5)
	tasks[0] = make([]byte, 888)
	tasks[1] = make([]byte, 1848588)
	tasks[2] = make([]byte, 50138788)
	tasks[3] = make([]byte, 170338664)
	tasks[4] = make([]byte, 558777333)
	for i := range tasks {
		_, err := rand.Read(tasks[i])
		if err != nil {
			log.Fatalln(err)
		}
	}
	manager.ClusterInit(tasks)
}
