package main

import (
	"log"
	"net"

	"github.com/moevm/grpc_server/internal/config"
	"github.com/moevm/grpc_server/internal/grpcserver"
	"github.com/moevm/grpc_server/internal/manager"
	pb "github.com/moevm/grpc_server/pkg/proto/file_service"
	"google.golang.org/grpc"
	"google.golang.org/grpc/reflection"
)

func main() {
	cfg := config.Load()

	mgr, err := manager.NewManager()
	if err != nil {
		log.Fatalf("manager.NewManager(): %v", err)
	}

	lis, err := net.Listen("tcp", net.JoinHostPort(cfg.Host, cfg.Port))
	if err != nil {
		log.Fatalf("failed to listen: %v", err)
	}

	serverOpts := []grpc.ServerOption{
		grpc.MaxRecvMsgSize(cfg.MaxMessageSize),
		grpc.MaxSendMsgSize(cfg.MaxMessageSize),
	}

	service := grpc.NewServer(serverOpts...)
	pb.RegisterFileServiceServer(service, grpcserver.NewServer(cfg.AllowedChars, mgr))
	reflection.Register(service)

	log.Printf("Server starting on %s:%s", cfg.Host, cfg.Port)
	if err := service.Serve(lis); err != nil {
		log.Fatalf("failed to serve: %v", err)
	}
}
