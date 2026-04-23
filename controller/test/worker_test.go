package test

import (
	"context"
	"net"
	"os"
	"os/exec"
	"path/filepath"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"google.golang.org/grpc"
	"google.golang.org/protobuf/types/known/emptypb"
	pb "github.com/moevm/grpc_server/controller/test"
)

const bufSize = 1024 * 1024

type MockController struct {
	pb.UnimplementedDataServiceServer
	Policy *pb.WorkerPolicy
	t      *testing.T
}

func (m *MockController) GetPolicy(ctx context.Context, req *pb.GetPolicyRequest) (*pb.GetPolicyResponse, error) {
	m.t.Logf("GetPolicy called: worker_id=%d, version=%d", req.WorkerId, req.ConfigVersion)
	return &pb.GetPolicyResponse{
		Result: pb.GetPolicyResponse_POLICY_PROVIDED,
		Policy: m.Policy,
	}, nil
}

func (m *MockController) Classify(ctx context.Context, req *pb.ClassifyRequest) (*pb.ClassifyResponse, error) {
	m.t.Logf("Classify called: worker_id=%d, domain=%s", req.WorkerId, req.Domain)
	return &pb.ClassifyResponse{
		Categories: []string{"news", "technology"},
		TrustLevel: 85,
	}, nil
}

func (m *MockController) SendStats(ctx context.Context, req *pb.StatsReport) (*emptypb.Empty, error) {
	m.t.Logf("SendStats called: worker_id=%d", req.WorkerId)
	return &emptypb.Empty{}, nil
}

func StartMockController(t *testing.T, policy *pb.WorkerPolicy) (string, func()) {
    lis, err := net.Listen("tcp", "127.0.0.1:0")
    if err != nil {
        t.Fatalf("Failed to listen: %v", err)
    }

    s := grpc.NewServer()
    mock := &MockController{
        Policy: policy,
        t:      t,
    }
    pb.RegisterDataServiceServer(s, mock)

    go func() {
        if err := s.Serve(lis); err != nil {
            t.Logf("Server error: %v", err)
        }
    }()

    addr := lis.Addr().String()
    cleanup := func() {
        s.Stop()
        lis.Close()
    }
    return addr, cleanup
}

func findProjectRoot() string {
	dir, _ := os.Getwd()
	for {
		if _, err := os.Stat(filepath.Join(dir, "controller")); err == nil {
			return dir
		}
		if _, err := os.Stat(filepath.Join(dir, "worker")); err == nil {
			return dir
		}
		parent := filepath.Dir(dir)
		if parent == dir {
			return dir
		}
		dir = parent
	}
}

func TestWorkerPolicyContent(t *testing.T) {
	root := findProjectRoot()
	workerBin := filepath.Join(root, "worker", "bazel-bin", "worker")

	if _, err := os.Stat(workerBin); err != nil {
		t.Skipf("Worker binary not found: %v", err)
	}

	testPolicy := &pb.WorkerPolicy{
		ConfigVersion:   42,
		MinTrustLevel:   75,
		BlockCategories: []string{"gambling", "adult", "violence"},
		BlockDomains:    []string{"bad-site.com", "blocked.org"},
		AllowDomains:    []string{"safe-site.com", "trusted.net"},
		BlockByTrust: map[string]int32{
			"social_media": 60,
			"games":        40,
		},
	}

	addr, cleanup := StartMockController(t, testPolicy) 
	defer cleanup()

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	worker := exec.CommandContext(ctx, workerBin)
	worker.Env = []string{
		"WORKER_ID=1",
		"CONTROLLER_GRPC_ADDR=" + addr,
		"METRICS_GATEWAY_ADDRESS=localhost",
		"METRICS_GATEWAY_PORT=9091",
		"TEST_REQUEST_POLICY=true",
	}

	output, err := worker.CombinedOutput()
	assert.NoError(t, err, "Worker failed: %s", string(output))

	assert.Contains(t, string(output), "Policy received")
	assert.Contains(t, string(output), "Min trust level: 75")
	assert.Contains(t, string(output), "gambling")
	assert.Contains(t, string(output), "adult")
	assert.Contains(t, string(output), "violence")
	assert.Contains(t, string(output), "bad-site.com")
	assert.Contains(t, string(output), "blocked.org")
	assert.Contains(t, string(output), "safe-site.com")
	assert.Contains(t, string(output), "trusted.net")
	assert.Contains(t, string(output), "Config version: 42")
}

func TestWorkerClassifyWithMock(t *testing.T) {
	root := findProjectRoot()
	workerBin := filepath.Join(root, "worker", "bazel-bin", "worker")

	if _, err := os.Stat(workerBin); err != nil {
		t.Skipf("Worker binary not found: %v", err)
	}

	testPolicy := &pb.WorkerPolicy{
		ConfigVersion: 1,
		MinTrustLevel: 50,
	}

	addr, cleanup := StartMockController(t, testPolicy)
	defer cleanup()

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	worker := exec.CommandContext(ctx, workerBin)
	worker.Env = []string{
		"WORKER_ID=1",
		"CONTROLLER_GRPC_ADDR=" + addr,
		"METRICS_GATEWAY_ADDRESS=localhost",
		"METRICS_GATEWAY_PORT=9091",
		"TEST_CLASSIFY_DOMAIN=example.com",
	}

	output, err := worker.CombinedOutput()
	assert.NoError(t, err, "Worker failed: %s", string(output))
	assert.Contains(t, string(output), "Domain 'example.com' classified as category")
	assert.Contains(t, string(output), "trust level 85")
}
