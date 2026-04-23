package main

import (
	"fmt"
	"log"
	"net"
	"os"

	"github.com/moevm/grpc_server/internal/config"
	"github.com/moevm/grpc_server/internal/grpcserver"
	"github.com/moevm/grpc_server/internal/manager"
	"github.com/moevm/grpc_server/internal/service/storage"
	adminPb "github.com/moevm/grpc_server/pkg/proto/admin_service"
	commPb "github.com/moevm/grpc_server/pkg/proto/communication"
	"github.com/subosito/gotenv"
	"google.golang.org/grpc"
	"google.golang.org/grpc/reflection"
)

func init() {
	gotenv.Load()
}

func LoadConfigRedis() (storage.Config, error) {
	addr, exist := os.LookupEnv("REDIS_ADDR")

	if !exist {
		return storage.Config{}, fmt.Errorf("REDIS_ADDR does not exists")
	}

	return storage.Config{
		Addr: addr,
	}, nil
}

func main() {
	cfg := config.Load()
	adminServer := grpcserver.NewAdminServer()
	mgr, err := manager.NewManager()
	if err != nil {
		log.Fatalf("manager.NewManager(): %v", err)
	}

	adminServer.SetManager(mgr)

	configRedis, err := LoadConfigRedis()

	if err != nil {
		log.Fatalf("Error to load redis config: %v", err)
	}

	dataServer, err := grpcserver.NewDataServer(mgr, "internal/service/config/categories.json", "internal/service/config/providers.json", configRedis)

	if err != nil {
		log.Fatalf("Failed to create data server: %v", err)
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
	adminPb.RegisterAdminServiceServer(service, adminServer)
	commPb.RegisterDataServiceServer(service, dataServer)
	reflection.Register(service)

	log.Printf("Server starting on %s:%s", cfg.Host, cfg.Port)
	if err := service.Serve(lis); err != nil {
		log.Fatalf("failed to serve: %v", err)
	}
}
