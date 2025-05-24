package main

import (
	"bytes"
	"crypto/md5"
	"crypto/rand"
	"fmt"
	"log"
	"os"
	"time"

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

	expected := make([][16]byte, 5)
	for i, task := range tasks {
		expected[i] = md5.Sum(task)
	}

	go manager.Init()

	time.Sleep(5 * time.Second)

	ids := make(map[uint64]int, 5)
	for i, task := range tasks {
		id := manager.AddTask(task)
		ids[id] = i
	}

	for range 5 {
		solution := manager.GetTaskSolution()
		task_id := ids[solution.Id]
		if !bytes.Equal(solution.Body, expected[task_id][:]) {
			fmt.Printf("%v != %v", solution.Body, expected[task_id][:])
			os.Exit(1)
		}
	}
}
