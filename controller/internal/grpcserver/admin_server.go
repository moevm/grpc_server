package grpcserver

import (
	"context"

	"github.com/moevm/grpc_server/internal/manager"
	pb "github.com/moevm/grpc_server/pkg/proto/admin_service"
)

type AdminServer struct {
	pb.UnimplementedAdminServiceServer
	configData []byte
	manager    *manager.Manager
}

func NewAdminServer() *AdminServer {
	return &AdminServer{}
}

func (s *AdminServer) SetManager(m *manager.Manager) {
	s.manager = m
}

func (s *AdminServer) LoadConfig(ctx context.Context, req *pb.LoadConfigRequest) (*pb.LoadConfigResponse, error) {
	s.configData = req.ConfigData

	if s.manager != nil {
		if err := s.manager.UpdateConfig(s.configData); err != nil {
			return &pb.LoadConfigResponse{
				Success:      false,
				ErrorMessage: "failed to update config " + err.Error(),
			}, nil
		}
	}

	return &pb.LoadConfigResponse{
		Success: true,
	}, nil
}

func (s *AdminServer) GetConfig() []byte {
	return s.configData
}
