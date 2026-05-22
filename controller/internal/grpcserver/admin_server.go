package grpcserver

import (
	"context"
	"fmt"
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

func (s *AdminServer) GetConfigAdmin(ctx context.Context, req *pb.GetConfigRequest) (*pb.GetConfigResponse, error) {
	return &pb.GetConfigResponse{
		ConfigData: s.configData,
	}, nil
}

func (s *AdminServer) ToggleFiltering(ctx context.Context, req *pb.ToggleFilteringRequest) (*pb.ToggleFilteringResponse, error) {
	if s.manager == nil {
		return &pb.ToggleFilteringResponse{Success: false, Message: "manager not initialized"}, nil
	}

	s.manager.SetFilteringEnabled(req.WorkerId, req.Enabled)

	return &pb.ToggleFilteringResponse{
		Success: true,
		Message: fmt.Sprintf("Filtering %s for worker %d",
			map[bool]string{true: "enabled", false: "disabled"}[req.Enabled],
			req.WorkerId),
	}, nil
}
