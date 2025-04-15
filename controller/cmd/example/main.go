package main

import (
	"context"
	"fmt"
	"log"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
	worker "github.com/moevm/grpc_server/pkg/proto/worker"
)

func main() {
	conn, err := grpc.Dial(
		"localhost:50051",
		grpc.WithTransportCredentials(insecure.NewCredentials()),
	)
	if err != nil {
		log.Fatalf("Connection failed: %v", err)
	}
	defer conn.Close()

	client := worker.NewWorkerServiceClient(conn)

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	res, err := client.ProcessTask(ctx, &worker.TaskData{
		Data: []byte("Hello, World!"),
	})
	if err != nil {
		log.Fatalf("RPC failed: %v", err)
	}

	fmt.Printf("Received: %s\n", res.Result)
}
