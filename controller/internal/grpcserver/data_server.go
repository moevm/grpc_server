package grpcserver

import (
	"context"
	"log"

	"github.com/moevm/grpc_server/internal/manager"
	pb "github.com/moevm/grpc_server/pkg/proto/communication"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/types/known/emptypb"
)

type DataServer struct {
	pb.UnimplementedDataServiceServer
	manager *manager.Manager
}

func NewDataServer(mgr *manager.Manager) *DataServer {
	return &DataServer{manager: mgr}
}

func (s *DataServer) GetPolicy(ctx context.Context, req *pb.GetPolicyRequest) (*pb.GetPolicyResponse, error) {
	log.Printf("gRPC GetPolicy from worker %d ( version: %d)",
		req.WorkerId, req.ConfigVersion)

	policyBytes, changed, err := s.manager.HandleGetPolicy(
		req.WorkerId,
		req.ConfigVersion,
	)

	if err != nil {
		log.Printf("Error getting policy for worker %d: %v", req.WorkerId, err)
		return nil, status.Errorf(codes.Internal, "failed to get policy: %v", err)
	}

	if !changed {
		log.Printf("Policy unchanged for worker %d", req.WorkerId)
		return &pb.GetPolicyResponse{
			Result: pb.GetPolicyResponse_POLICY_UNCHANGED,
		}, nil
	}

	log.Printf("Policy changed for worker %d, sending full policy", req.WorkerId)
	var fullPolicy pb.WorkerPolicy
	if err := proto.Unmarshal(policyBytes, &fullPolicy); err != nil {
		log.Printf("Failed to unmarshal policy for worker %d: %v", req.WorkerId, err)
		return nil, status.Errorf(codes.Internal, "failed to unmarshal policy: %v", err)
	}

	return &pb.GetPolicyResponse{
		Result: pb.GetPolicyResponse_POLICY_PROVIDED,
		Policy: &fullPolicy,
	}, nil
}

func (s *DataServer) Classify(ctx context.Context, req *pb.ClassifyRequest) (*pb.ClassifyResponse, error) {
	log.Printf("gRPC Classify from worker %d for domain: %s", req.WorkerId, req.Domain)
	return &pb.ClassifyResponse{
		Categories: []string{"unknown"},
		TrustLevel: 50,
	}, nil
}

func (s *DataServer) SendStats(ctx context.Context, report *pb.StatsReport) (*emptypb.Empty, error) {
	log.Printf("gRPC Stats from worker %d: blocked=%d allowed=%d",
		report.WorkerId, report.TotalBlocked, report.TotalAllowed)
	return &emptypb.Empty{}, nil
}
