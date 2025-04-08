package main

import (
	"log"
	"net"
	"github.com/moevm/grpc_server/internal/grpcserver"
	pb "github.com/moevm/grpc_server/pkg/proto/file_service"
	"google.golang.org/grpc"
	"google.golang.org/grpc/reflection"
)

const (
	ServerHost        = "localhost"
	ServerPort        = "50051"
	MaxMessageSize    = 4 * 1024 * 1024 // 4MB
)

func main() {
	lis, err := net.Listen("tcp", net.JoinHostPort(ServerHost, ServerPort))
	if err != nil {
		log.Fatalf("failed to listen: %v", err)
	}

	serverOpts := []grpc.ServerOption{
		grpc.MaxRecvMsgSize(MaxMessageSize),
		grpc.MaxSendMsgSize(MaxMessageSize),
	}

	service := grpc.NewServer(serverOpts...)
	pb.RegisterFileServiceServer(service, &grpcserver.Server{})
	reflection.Register(service)

	log.Printf("Server starting on %s:%s", ServerHost, ServerPort)
	if err := service.Serve(lis); err != nil {
		log.Fatalf("failed to serve: %v", err)
	}
}