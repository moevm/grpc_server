package grpcserver_test

import (
	"context"
	"testing"
	"time"

	"github.com/moevm/grpc_server/internal/grpcserver"
	pb "github.com/moevm/grpc_server/pkg/proto/file_service"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

func TestUploadFile_TextValidation(t *testing.T) {
	testCases := []struct {
		name         string
		content      string
		allowedChars string
		isValid      bool
	}{
		{
			name:         "valid content",
			content:      "Hello 123!",
			allowedChars: "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789! ",
			isValid:      true,
		},
		{
			name:         "invalid character",
			content:      "Hello@123",
			allowedChars: "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789! ",
			isValid:      false,
		},
		{
			name:         "invalid utf8",
			content:      string([]byte{0xff, 0xfe, 0xfd}),
			allowedChars: "abc",
			isValid:      false,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			taskChan := make(chan []byte, 1)
			server := grpcserver.NewServer(tc.allowedChars, taskChan)
			resp, err := server.UploadFile(context.Background(), &pb.FileRequest{
				Content:  []byte(tc.content),
				FileType: "text",
			})

			require.NoError(t, err)
			assert.Equal(t, tc.isValid, resp.IsValid)

			if tc.isValid {
				select {
				case <-taskChan:
				case <-time.After(100 * time.Millisecond):
					t.Error("Task was not sent to channel")
				}
			} else {
				select {
				case <-taskChan:
					t.Error("Task should not be sent to channel")
				default:
				}
			}
		})
	}
}

func TestUploadFile_BinaryValidation(t *testing.T) {
	taskChan := make(chan []byte, 1)
	server := grpcserver.NewServer("", taskChan)
	resp, err := server.UploadFile(context.Background(), &pb.FileRequest{
		Content:  []byte{0x00, 0x01, 0x02},
		FileType: "binary",
	})

	require.NoError(t, err)
	assert.True(t, resp.IsValid)

	select {
	case <-taskChan:
	case <-time.After(100 * time.Millisecond):
		t.Error("Binary task was not sent to channel")
	}
}

func TestUploadFile_InvalidFileType(t *testing.T) {
	taskChan := make(chan []byte, 1)
	server := grpcserver.NewServer("", taskChan)
	_, err := server.UploadFile(context.Background(), &pb.FileRequest{
		Content:  []byte("test"),
		FileType: "invalid",
	})

	require.Error(t, err)
	st, ok := status.FromError(err)
	require.True(t, ok)
	assert.Equal(t, codes.InvalidArgument, st.Code())

	select {
	case <-taskChan:
		t.Error("Task should not be sent for invalid file type")
	default:
	}
}
