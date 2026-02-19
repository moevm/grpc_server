package grpcserver

import (
	"context"
	"github.com/moevm/grpc_server/internal/manager"
	pb "github.com/moevm/grpc_server/pkg/proto/admin_service"
)

type AdminServer struct {
	pb.UnimplementedAdminServiceServer
	configData []byte
	version    uint64
	manager    *manager.Manager 
}

func NewAdminServer() *AdminServer {
	return &AdminServer{
		version: 0,
	}
}

func (s *AdminServer) SetManager(m *manager.Manager) {
	s.manager = m
}

func (s *AdminServer) LoadConfig(ctx context.Context, req *pb.LoadConfigRequest) (*pb.LoadConfigResponse, error) {
	s.configData = req.ConfigData
	s.version++
	
	if s.manager != nil {
		s.manager.UpdateConfig(s.configData, s.version)
	}

	return &pb.LoadConfigResponse{
		Success: true,
	}, nil
}


func (s *AdminServer) GetConfig() ([]byte, uint64) {
	return s.configData, s.version
}
