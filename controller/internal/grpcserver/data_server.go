package grpcserver

import (
	"context"
	"log"
	"time"

	"github.com/moevm/grpc_server/internal/manager"
	"github.com/moevm/grpc_server/internal/service/storage"
	pb "github.com/moevm/grpc_server/pkg/proto/communication"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/types/known/emptypb"

	"github.com/moevm/grpc_server/internal/service/service"
	"github.com/redis/go-redis/v9"
)

type DataServer struct {
	pb.UnimplementedDataServiceServer
	manager    *manager.Manager
	classifier *service.Service
	storage    *storage.RedisClient
}

func NewDataServer(mgr *manager.Manager, categoryFile, providerFile string, config storage.Config) (*DataServer, error) {
	classifier, err := service.NewService(categoryFile, providerFile)

	if err != nil {
		return nil, err
	}

	ctx, cancel := context.WithTimeout(context.Background(), time.Second*5)

	defer cancel()

	redis, err := storage.NewRedisClient(ctx, config)

	if err != nil {
		return nil, err
	}

	return &DataServer{
		manager:    mgr,
		classifier: classifier,
		storage:    redis,
	}, nil
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
			Result:           pb.GetPolicyResponse_POLICY_UNCHANGED,
			FilteringEnabled: s.manager.IsFilteringEnabled(req.WorkerId),
		}, nil
	}

	log.Printf("Policy changed for worker %d, sending full policy", req.WorkerId)
	var fullPolicy pb.WorkerPolicy
	if err := proto.Unmarshal(policyBytes, &fullPolicy); err != nil {
		log.Printf("Failed to unmarshal policy for worker %d: %v", req.WorkerId, err)
		return nil, status.Errorf(codes.Internal, "failed to unmarshal policy: %v", err)
	}

	return &pb.GetPolicyResponse{
		Result:           pb.GetPolicyResponse_POLICY_PROVIDED,
		Policy:           &fullPolicy,
		FilteringEnabled: s.manager.IsFilteringEnabled(req.WorkerId),
	}, nil
}

func makeClassifyCacheKey(target, reqType string) string {
	return target + ":" + reqType
}

func (s *DataServer) Classify(ctx context.Context, req *pb.ClassifyRequest) (*pb.ClassifyResponse, error) {
    log.Printf("gRPC Classify from worker %d: type=%s, target=%s",
        req.WorkerId, req.Type, req.Target)

    cacheKey := makeClassifyCacheKey(req.Target, req.Type)
    cached, err := s.storage.GetRequestCache(ctx, cacheKey)

    var categoryIDs []int
    var cacheHit bool

    if err != nil {
        if err == redis.Nil {
            log.Printf("Cache miss for key: %s", cacheKey)
            cacheHit = false
        } else {
            log.Printf("Redis error: %v", err)
            cacheHit = false
        }
    } else {
        cacheHit = true
        categoryIDs = cached.CategoriesIds
        log.Printf("Cache hit for key: %s", cacheKey)
    }

    if !cacheHit {
        var err error
        categoryIDs, err = s.classifier.Check(req.Target, req.Type)
        if err != nil {
            log.Printf("Classification error: %v", err)
            return &pb.ClassifyResponse{
                Categories: []string{"unknown"},
                TrustLevel: 0,
            }, nil
        }

        if err := s.storage.SaveRequestCache(ctx, storage.RequestCache{
            Endpoint:      cacheKey,
            CategoriesIds: categoryIDs,
        }, len(categoryIDs) == 0); err != nil {
            log.Printf("Failed to save cache: %v", err)
        }
    }

    if len(categoryIDs) > 0 {
        categories := make([]string, 0, len(categoryIDs))
        minTrustLevel := 0

        for _, id := range categoryIDs {
            name, trustLevel := s.classifier.GetCategory(id)
            if name != "" {
                categories = append(categories, name)
            }
            if trustLevel < minTrustLevel {
                minTrustLevel = trustLevel
            }
        }

        return &pb.ClassifyResponse{
            Categories: categories,
            TrustLevel: int32(minTrustLevel),
        }, nil
    }

    return &pb.ClassifyResponse{
        Categories: []string{"unknown"},
        TrustLevel: 0,
    }, nil
}

func (s *DataServer) SendStats(ctx context.Context, report *pb.StatsReport) (*emptypb.Empty, error) {
	log.Printf("gRPC Stats from worker %d: blocked=%d allowed=%d",
		report.WorkerId, report.TotalBlocked, report.TotalAllowed)
	return &emptypb.Empty{}, nil
}
