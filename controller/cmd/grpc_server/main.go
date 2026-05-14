package main

import (
	"fmt"
	"log"
	"net"
	"net/http"

	"github.com/moevm/grpc_server/internal/config"
	"github.com/moevm/grpc_server/internal/grpcserver"
	"github.com/moevm/grpc_server/internal/manager"
	adminPb "github.com/moevm/grpc_server/pkg/proto/admin_service"
	commPb "github.com/moevm/grpc_server/pkg/proto/communication"
	"google.golang.org/grpc"
	"google.golang.org/grpc/reflection"
)

func main() {
	cfg := config.Load()
	adminServer := grpcserver.NewAdminServer()
	mgr, err := manager.NewManager()
	if err != nil {
		log.Fatalf("manager.NewManager(): %v", err)
	}

	adminServer.SetManager(mgr)

	dataServer, err := grpcserver.NewDataServer(mgr, "internal/service/config/categories.json", "internal/service/config/providers.json")

	if err != nil {
		log.Fatalf("Failed to create data server: %v", err)
	}

	go func() {
		http.HandleFunc("/metrics", func(w http.ResponseWriter, r *http.Request) {
			stats := mgr.GetAggregatedStats()

			w.Header().Set("Content-Type", "text/plain; version=0.0.4")

			fmt.Fprintf(w, "# HELP packets_passed_total Total number of packets passed\n")
			fmt.Fprintf(w, "# TYPE packets_passed_total counter\n")
			fmt.Fprintf(w, "packets_passed_total %d\n", stats.PacketsPassed)

			fmt.Fprintf(w, "# HELP packets_dropped_total Total number of packets dropped\n")
			fmt.Fprintf(w, "# TYPE packets_dropped_total counter\n")
			fmt.Fprintf(w, "packets_dropped_total %d\n", stats.PacketsDropped)

			fmt.Fprintf(w, "# HELP packets_received_total Total number of packets received\n")
			fmt.Fprintf(w, "# TYPE packets_received_total counter\n")
			fmt.Fprintf(w, "packets_received_total %d\n", stats.PacketsReceived)

			fmt.Fprintf(w, "# HELP worker_last_update Last update timestamp\n")
			fmt.Fprintf(w, "# TYPE worker_last_update gauge\n")
			fmt.Fprintf(w, "worker_last_update %d\n", stats.LastUpdate.Unix())

			fmt.Fprintf(w, "# HELP worker_up Worker is reporting metrics\n")
			fmt.Fprintf(w, "# TYPE worker_up gauge\n")
			fmt.Fprintf(w, "worker_up 1\n")
		})

		log.Println("Metrics HTTP server starting on :8081")
		if err := http.ListenAndServe("0.0.0.0:8081", nil); err != nil {
			log.Printf("Metrics HTTP server error: %v", err)
		}
	}()

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