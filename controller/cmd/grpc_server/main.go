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
	adminPb "github.com/moevm/grpc_server/pkg/proto/admin_service"
	commPb "github.com/moevm/grpc_server/pkg/proto/communication"
)

func main() {
	cfg := config.Load()
	adminServer := grpcserver.NewAdminServer()
	mgr, err := manager.NewManager()
	if err != nil {
		log.Fatalf("manager.NewManager(): %v", err)
	}

	adminServer.SetManager(mgr)
	
	dataServer := grpcserver.NewDataServer(mgr)

	lis, err := net.Listen("tcp", net.JoinHostPort(cfg.Host, cfg.Port))
	if err != nil {
		log.Fatalf("failed to listen: %v", err)
	}

	serverOpts := []grpc.ServerOption{
		grpc.MaxRecvMsgSize(cfg.MaxMessageSize),
		grpc.MaxSendMsgSize(cfg.MaxMessageSize),
	}

	service := grpc.NewServer(serverOpts...)
	adminPb.RegisterAdminServiceServer(service, adminServer)
	pb.RegisterFileServiceServer(service, grpcserver.NewServer(cfg.AllowedChars, mgr))
	commPb.RegisterDataServiceServer(service, dataServer)  
	reflection.Register(service)

	log.Printf("Server starting on %s:%s", cfg.Host, cfg.Port)
	if err := service.Serve(lis); err != nil {
		log.Fatalf("failed to serve: %v", err)
	}
}
