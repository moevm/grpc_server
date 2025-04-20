package grpcserver_test

import (
	"context"
	"testing"
	"github.com/moevm/grpc_server/internal/grpcserver"
	pb "github.com/moevm/grpc_server/pkg/proto/file_service"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

func TestUploadFile_TextValidation(t *testing.T) {
	testCases := []struct {
		name        string
		content     string
		allowedChars string
		isValid     bool
	}{
		{
			name:        "valid content",
			content:     "Hello 123!",
			allowedChars: "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789! ",
			isValid:     true,
		},
		{
			name:        "invalid character",
			content:     "Hello@123",
			allowedChars: "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789! ",
			isValid:     false,
		},
		{
			name:        "invalid utf8",
			content:     string([]byte{0xff, 0xfe, 0xfd}),
			allowedChars: "abc",
			isValid:     false,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			server := grpcserver.NewServer(tc.allowedChars)
			resp, err := server.UploadFile(context.Background(), &pb.FileRequest{
				Content:  []byte(tc.content),
				FileType: "text",
			})

			require.NoError(t, err)
			assert.Equal(t, tc.isValid, resp.IsValid)
		})
	}
}

func TestUploadFile_BinaryValidation(t *testing.T) {
	server := grpcserver.NewServer("")
	resp, err := server.UploadFile(context.Background(), &pb.FileRequest{
		Content:  []byte{0x00, 0x01, 0x02},
		FileType: "binary",
	})

	require.NoError(t, err)
	assert.True(t, resp.IsValid)
}

func TestUploadFile_InvalidFileType(t *testing.T) {
	server := grpcserver.NewServer("")
	_, err := server.UploadFile(context.Background(), &pb.FileRequest{
		Content:  []byte("test"),
		FileType: "invalid",
	})

	require.Error(t, err)
	st, ok := status.FromError(err)
	require.True(t, ok)
	assert.Equal(t, codes.InvalidArgument, st.Code())
}