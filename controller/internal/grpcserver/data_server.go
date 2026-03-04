package grpcserver

import (
    "context"
    "log"

    "github.com/moevm/grpc_server/internal/manager"
    pb "github.com/moevm/grpc_server/pkg/proto/communication"
    "google.golang.org/grpc/codes"
    "google.golang.org/grpc/status"
    "google.golang.org/protobuf/types/known/emptypb"
)

type DataServer struct {
    pb.UnimplementedDataServiceServer
    manager *manager.Manager
}

func NewDataServer(mgr *manager.Manager) *DataServer {
    return &DataServer{manager: mgr}
}

func (s *DataServer) GetPolicy(ctx context.Context, req *pb.GetPolicyRequest) (*pb.WorkerPolicy, error) {
    log.Printf("gRPC GetPolicy from worker %d (hash: %d)", req.WorkerId, req.PolicyHash)
    policy := s.manager.GetWorkerPolicy(req.WorkerId)
    if policy == nil {
        return nil, status.Errorf(codes.NotFound, "no policy for worker %d", req.WorkerId)
    }
    return policy, nil
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