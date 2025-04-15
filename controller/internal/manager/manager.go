package manager

import (
	"context"
	"fmt"
	"log"
	"sync"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
	worker "github.com/moevm/grpc_server/pkg/proto/worker"
)

const (
	workerPortStart = 50051
	workerCount     = 5
)

type Task struct {
	ID     int
	Input  []byte
	Output []byte
}

func ProcessTasks(inputs [][]byte) {
	clients := make([]worker.WorkerServiceClient, workerCount)
	for i := 0; i < workerCount; i++ {
		conn, err := grpc.Dial(
			fmt.Sprintf("localhost:%d", workerPortStart+i),
			grpc.WithTransportCredentials(insecure.NewCredentials()),
		)
		if err != nil {
			log.Fatalf("Failed to connect to worker localhost:%d: %v", workerPortStart+i, err)
		}
		defer conn.Close()
		clients[i] = worker.NewWorkerServiceClient(conn)
	}

	workerPool := make(chan int, workerCount)
	for i := 0; i < workerCount; i++ {
		workerPool <- i
	}

	var wg sync.WaitGroup
	taskChan := make(chan Task)

	for i := 0; i < workerCount; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			for task := range taskChan {
				workerID := <-workerPool
				
				ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
				defer cancel()
				
				res, err := clients[workerID].ProcessTask(ctx, &worker.TaskData{
					Data: task.Input,
				})
				
				if err != nil {
					log.Printf("Worker %d failed task %d: %v", workerID, task.ID, err)
				} else {
					task.Output = res.Result
					fmt.Printf("Task %d processed by worker %d: %s => %s\n",
						task.ID, workerID, task.Input, task.Output)
				}
				
				workerPool <- workerID
			}
		}()
	}

	for id, input := range inputs {
		taskChan <- Task{ID: id, Input: input}
	}

	close(taskChan)
	wg.Wait()
}
