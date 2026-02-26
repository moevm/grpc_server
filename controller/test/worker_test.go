package test

import (
	"context"
	"os"
	"os/exec"
	"path/filepath"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
)

func findProjectRoot() string {
	dir, _ := os.Getwd()
	for {
		if _, err := os.Stat(filepath.Join(dir, "controller")); err != nil {
			if _, err := os.Stat(filepath.Join(dir, "worker")); err != nil {
				dir = filepath.Dir(dir)
				continue
			}
		}
		return dir
	}
}

func TestWorkerPolicyRequest(t *testing.T) {
	root := findProjectRoot()

	ctrlBin := filepath.Join(root, "controller", "bazel-bin", "cmd", "grpc_server", "grpc_server_", "grpc_server")
	workerBin := filepath.Join(root, "worker", "bazel-bin", "worker")

	ctrl := exec.Command(ctrlBin)
	ctrl.Start()
	defer ctrl.Process.Kill()

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	worker := exec.CommandContext(ctx, workerBin)
	worker.Env = []string{
		"METRICS_GATEWAY_ADDRESS=localhost",
		"METRICS_GATEWAY_PORT=9091",
		"TEST_REQUEST_POLICY=true",
	}

	output, err := worker.CombinedOutput()
	assert.NoError(t, err, "Worker failed")
	assert.Contains(t, string(output), "Worker 1 requests policy")
	assert.Contains(t, string(output), "Policy received")
}

func TestWorkerClassifyRequest(t *testing.T) {
	root := findProjectRoot()

	ctrlBin := filepath.Join(root, "controller", "bazel-bin", "cmd", "grpc_server", "grpc_server_", "grpc_server")
	workerBin := filepath.Join(root, "worker", "bazel-bin", "worker")

	ctrl := exec.Command(ctrlBin)
	ctrl.Start()
	defer ctrl.Process.Kill()

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	worker := exec.CommandContext(ctx, workerBin)
	worker.Env = []string{
		"METRICS_GATEWAY_ADDRESS=localhost",
		"METRICS_GATEWAY_PORT=9091",
		"TEST_CLASSIFY_DOMAIN=facebook.com",
	}

	output, err := worker.CombinedOutput()
	assert.NoError(t, err, "Worker failed")
	assert.Contains(t, string(output), "Domain 'facebook.com' classified as category")
}