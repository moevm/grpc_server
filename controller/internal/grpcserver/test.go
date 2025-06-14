package grpcserver

import (
	"context"
	"testing"

	"github.com/moevm/grpc_server/internal/grpcserver"
	"github.com/moevm/grpc_server/internal/manager"
	pb "github.com/moevm/grpc_server/pkg/proto/file_service"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

type mockTaskManager struct {
	counter  uint64
	receiver chan manager.TaskSolution
}

func (m *mockTaskManager) AddTask(content []byte, receiver *chan manager.TaskSolution) uint64 {
	m.counter += 1
	if receiver == nil {
		m.receiver <- manager.TaskSolution{Body: []byte{0, 1, 2, 3}}
		return m.counter
	}
	*receiver <- manager.TaskSolution{Body: []byte{0, 1, 2, 3}}
	return m.counter
}

func (m *mockTaskManager) GetTaskSolution() manager.TaskSolution {
	return <-m.receiver
}

func TestServer_UploadFile(t *testing.T) {
	allowedChars := "abc123"
	s := grpcserver.NewServer(allowedChars, &mockTaskManager{})

	testCases := []struct {
		name        string
		req         *pb.FileRequest
		expectedRes *pb.FileResponse
		expectedErr error
	}{
		{
			name: "valid text",
			req: &pb.FileRequest{
				FileType: "text",
				Content:  []byte("a1 b2 c3"),
			},
			expectedRes: &pb.FileResponse{
				Size:    8,
				IsValid: true,
				Message: "Validation successful. Hash is mocked_hash",
			},
		},
		{
			name: "invalid text - disallowed chars",
			req: &pb.FileRequest{
				FileType: "text",
				Content:  []byte("invalid! char@"),
			},
			expectedRes: &pb.FileResponse{
				Size:    14,
				IsValid: false,
				Message: "Invalid file content",
			},
		},
		{
			name: "invalid text - bad utf8",
			req: &pb.FileRequest{
				FileType: "text",
				Content:  []byte{0xff, 0xfe, 0xfd},
			},
			expectedRes: &pb.FileResponse{
				Size:    3,
				IsValid: false,
				Message: "Invalid file content",
			},
		},
		{
			name: "valid binary",
			req: &pb.FileRequest{
				FileType: "binary",
				Content:  []byte{0x00, 0x01, 0x02},
			},
			expectedRes: &pb.FileResponse{
				Size:    3,
				IsValid: true,
				Message: "Validation successful. Hash is mocked_hash",
			},
		},
		{
			name: "invalid file type",
			req: &pb.FileRequest{
				FileType: "invalid",
				Content:  []byte("any"),
			},
			expectedErr: status.Error(codes.InvalidArgument, "invalid file type: invalid"),
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			res, err := s.UploadFile(context.Background(), tc.req)

			if tc.expectedErr != nil {
				require.Error(t, err)
				assert.Contains(t, err.Error(), tc.expectedErr.Error())
				return
			}

			require.NoError(t, err)
			assert.Equal(t, tc.expectedRes.Size, res.Size)
			assert.Equal(t, tc.expectedRes.IsValid, res.IsValid)

			if tc.expectedRes.IsValid {
				assert.Contains(t, res.Message, "[0 1 2 3]")
			} else {
				assert.Equal(t, tc.expectedRes.Message, res.Message)
			}
		})
	}
}
