package main

import (
	"fmt"
	"log"
	"net"
	"strconv"
	"os"
	"time"

	"github.com/moevm/grpc_server/internal/config"
	"github.com/moevm/grpc_server/internal/grpcserver"
	"github.com/moevm/grpc_server/internal/manager"
	"github.com/moevm/grpc_server/internal/service/storage"
	adminPb "github.com/moevm/grpc_server/pkg/proto/admin_service"
	commPb "github.com/moevm/grpc_server/pkg/proto/communication"
	"google.golang.org/grpc"
	"google.golang.org/grpc/reflection"
)

func LoadConfigRedis() (storage.Config, error) {
    cfg := storage.Config{}

    addr, exist := os.LookupEnv("REDIS_ADDR")
    if !exist {
        return storage.Config{}, fmt.Errorf("REDIS_ADDR does not exist")
    }
    cfg.Addr = addr

    password, exist := os.LookupEnv("REDIS_PASSWORD")
    if !exist {
        return storage.Config{}, fmt.Errorf("REDIS_PASSWORD does not exist")
    }
    cfg.Password = password

    user, exist := os.LookupEnv("REDIS_USER")
    if !exist {
        return storage.Config{}, fmt.Errorf("REDIS_USER does not exist")
    }
    cfg.User = user


    dbStr, exist := os.LookupEnv("REDIS_DB")
    if exist {
        db, err := strconv.Atoi(dbStr)
        if err != nil {
            return storage.Config{}, fmt.Errorf("REDIS_DB must be integer: %w", err)
        }
        cfg.DB = db
    } else {
        cfg.DB = 0
    }

    retriesStr, exist := os.LookupEnv("REDIS_MAX_RETRIES")
    if exist {
        retries, err := strconv.Atoi(retriesStr)
        if err != nil {
            return storage.Config{}, fmt.Errorf("REDIS_MAX_RETRIES must be integer: %w", err)
        }
        cfg.MaxRetries = retries
    } else {
        cfg.MaxRetries = 3
    }

    dialTimeoutStr, exist := os.LookupEnv("REDIS_DIAL_TIMEOUT")
    if exist {
        d, err := time.ParseDuration(dialTimeoutStr)
        if err != nil {
            return storage.Config{}, fmt.Errorf("REDIS_DIAL_TIMEOUT invalid duration: %w", err)
        }
        cfg.DialTimeout = d
    } else {
        cfg.DialTimeout = 5 * time.Second
    }

    timeoutStr, exist := os.LookupEnv("REDIS_TIMEOUT")
    if exist {
        d, err := time.ParseDuration(timeoutStr)
        if err != nil {
            return storage.Config{}, fmt.Errorf("REDIS_TIMEOUT invalid duration: %w", err)
        }
        cfg.Timeout = d
    } else {
        cfg.Timeout = 3 * time.Second
    }


    ttlSuccessStr, exist := os.LookupEnv("REDIS_TTL_SUCCESS")
    if exist {
        d, err := time.ParseDuration(ttlSuccessStr)
        if err != nil {
            return storage.Config{}, fmt.Errorf("REDIS_TTL_SUCCESS invalid duration: %w", err)
        }
        cfg.TTLSuccess = d
    } else {
        cfg.TTLSuccess = 24 * time.Hour
    }

    ttlUnknownStr, exist := os.LookupEnv("REDIS_TTL_UNKNOWN")
    if exist {
        d, err := time.ParseDuration(ttlUnknownStr)
        if err != nil {
            return storage.Config{}, fmt.Errorf("REDIS_TTL_UNKNOWN invalid duration: %w", err)
        }
        cfg.TTLUnknown = d
    } else {
        cfg.TTLUnknown = 1 * time.Hour
    }

    return cfg, nil
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
