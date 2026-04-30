package test

import (
	"context"
	"net"
	"os"
	"os/exec"
	"path/filepath"
	"testing"
	"time"

	pb "github.com/moevm/grpc_server/pkg/proto/communication"
	"github.com/stretchr/testify/assert"
	"google.golang.org/grpc"
	"google.golang.org/protobuf/types/known/emptypb"
)

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
	m.t.Logf("Classify called: worker_id=%d, type=%s, target=%s", req.WorkerId, req.Type, req.Target)
	return &pb.ClassifyResponse{
		Categories: []string{"news", "technology"},
		TrustLevel: 3,
	}, nil
}

func (m *MockController) SendStats(ctx context.Context, req *pb.StatsReport) (*emptypb.Empty, error) {
	m.t.Logf("SendStats called: worker_id=%d", req.WorkerId)
	return &emptypb.Empty{}, nil
}

func StartMockController(t *testing.T, policy *pb.WorkerPolicy) (string, func()) {
	listenAddr := os.Getenv("TEST_CONTROLLER_ADDR")
	if listenAddr == "" {
		listenAddr = "localhost:0"
	}

	lis, err := net.Listen("tcp", listenAddr)
	if err != nil {
		t.Fatalf("Failed to listen on %s: %v", listenAddr, err)
	}
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
		ConfigVersion:   2,
		MinTrustLevel:   2,
		BlockCategories: []string{"CATEGORY_ONLINE_SHOPS", "CATEGORY_ANONYMIZERS", "CATEGORY_ALCOHOL"},
		BlockDomains:    []string{"1xbet.com"},
		AllowDomains:    []string{"github.com", "vk.com"},
		BlockByTrust: map[string]int32{
			"CATEGORY_MALWARE": 3,
			"CATEGORY_BETTING": 4,
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

	outputStr := string(output)

	assert.Contains(t, outputStr, "Policy received")
	assert.Contains(t, outputStr, "Min trust level: 2")
	assert.Contains(t, outputStr, "Config version: 2")

	assert.Contains(t, outputStr, "blocked_categories: CATEGORY_ONLINE_SHOPS")
	assert.Contains(t, outputStr, "blocked_categories: CATEGORY_ANONYMIZERS")
	assert.Contains(t, outputStr, "blocked_categories: CATEGORY_ALCOHOL")
	assert.Contains(t, outputStr, "block_domains: 1xbet.com")
	assert.Contains(t, outputStr, "allow_domains: github.com")
	assert.Contains(t, outputStr, "allow_domains: vk.com")
}

func TestWorkerClassify(t *testing.T) {
	root := findProjectRoot()
	workerBin := filepath.Join(root, "worker", "bazel-bin", "worker")

	if _, err := os.Stat(workerBin); err != nil {
		t.Skipf("Worker binary not found: %v", err)
	}

	addr, cleanup := StartMockController(t, nil)
	defer cleanup()

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	worker := exec.CommandContext(ctx, workerBin)
	worker.Env = []string{
		"WORKER_ID=1",
		"CONTROLLER_GRPC_ADDR=" + addr,
		"METRICS_GATEWAY_ADDRESS=localhost",
		"METRICS_GATEWAY_PORT=9091",
		"TEST_CLASSIFY_TARGET=example.com",
	}

	output, err := worker.CombinedOutput()
	outputStr := string(output)
	assert.NoError(t, err, "Worker failed: %s", string(output))
	assert.Contains(t, outputStr, "Target 'example.com' classified as categories [news, technology] with trust level 3")
}

func TestWorkerSendStats(t *testing.T) {
	root := findProjectRoot()
	workerBin := filepath.Join(root, "worker", "bazel-bin", "worker")

	if _, err := os.Stat(workerBin); err != nil {
		t.Skipf("Worker binary not found: %v", err)
	}

	addr, cleanup := StartMockController(t, nil)
	defer cleanup()

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	worker := exec.CommandContext(ctx, workerBin)
	worker.Env = []string{
		"WORKER_ID=1",
		"CONTROLLER_GRPC_ADDR=" + addr,
		"METRICS_GATEWAY_ADDRESS=localhost",
		"METRICS_GATEWAY_PORT=9091",
		"TEST_STATS=true",
	}

	output, err := worker.CombinedOutput()
	assert.NoError(t, err, "Worker failed: %s", string(output))

	outputStr := string(output)

	assert.Contains(t, outputStr, "Stats sent successfully")

}
