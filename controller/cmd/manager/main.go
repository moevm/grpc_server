package main

import (
	"github.com/moevm/grpc_server/internal/manager"
	"log"
	"math/rand"
)

func main() {
	const (
		minSize = 5 * 1024 * 1024
		maxSize = 100 * 1024 * 1024
	)

	tasks := make([][]byte, 20)

	for i := range tasks {
		size := minSize + rand.Intn(maxSize-minSize+1)
		tasks[i] = make([]byte, size)
		if _, err := rand.Read(tasks[i]); err != nil {
			log.Fatalf("Failed to create task: %v", err)
		}
	}
	
	manager.ProcessTasks(tasks)
}
