package grpcserver_test

import (
	"context"
	"github.com/moevm/grpc_server/internal/config"
	"github.com/moevm/grpc_server/internal/grpcserver"
	pb "github.com/moevm/grpc_server/pkg/proto/file_service"
	"github.com/stretchr/testify/require"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
	"net"
	"testing"
	"time"
)

func startTestServer(t *testing.T) (string, func(), <-chan []byte, chan<- []byte) {
	cfg := config.Load()
	cfg.Host = "localhost"
	cfg.Port = "0"

	serverChan := make(chan []byte, 10)
	testWriteChan := make(chan []byte, 10)

	lis, err := net.Listen("tcp", net.JoinHostPort(cfg.Host, cfg.Port))
	require.NoError(t, err)

	server := grpc.NewServer(
		grpc.MaxRecvMsgSize(cfg.MaxMessageSize),
		grpc.MaxSendMsgSize(cfg.MaxMessageSize),
	)

	go func() {
		for msg := range testWriteChan {
			serverChan <- msg
		}
		close(serverChan)
	}()

	pb.RegisterFileServiceServer(server, grpcserver.NewServer(cfg.AllowedChars, serverChan))

	go func() {
		_ = server.Serve(lis)
	}()

	return lis.Addr().String(), func() {
		server.Stop()
		lis.Close()
		close(testWriteChan)
	}, serverChan, testWriteChan
}

func TestIntegration_ServerClientCommunication(t *testing.T) {
	addr, stop, taskChan, testWriteChan := startTestServer(t)
	defer stop()

	ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
	defer cancel()

	conn, err := grpc.NewClient(
		addr,
		grpc.WithTransportCredentials(insecure.NewCredentials()),
		grpc.WithContextDialer(func(ctx context.Context, addr string) (net.Conn, error) {
			d := net.Dialer{Timeout: 2 * time.Second}
			return d.DialContext(ctx, "tcp", addr)
		}),
	)
	require.NoError(t, err)
	defer conn.Close()

	client := pb.NewFileServiceClient(conn)

	t.Run("valid text file", func(t *testing.T) {
		content := []byte("Valid content")
		resp, err := client.UploadFile(ctx, &pb.FileRequest{
			Content:  content,
			FileType: "text",
		})
		require.NoError(t, err)
		require.True(t, resp.IsValid)

		select {
		case task := <-taskChan:
			require.Equal(t, content, task)
		case <-time.After(500 * time.Millisecond):
			t.Fatal("Task was not received in channel")
		}
	})

	t.Run("invalid binary file", func(t *testing.T) {
		content := make([]byte, 1024)
		resp, err := client.UploadFile(ctx, &pb.FileRequest{
			Content:  content,
			FileType: "binary",
		})
		require.NoError(t, err)
		require.True(t, resp.IsValid)

		select {
		case task := <-taskChan:
			require.Equal(t, content, task)
		case <-time.After(500 * time.Millisecond):
			t.Fatal("Binary task was not received in channel")
		}
	})

	t.Run("queue full handling", func(t *testing.T) {
		for i := 0; i < 10; i++ {
			testWriteChan <- []byte("test")
		}

		resp, err := client.UploadFile(ctx, &pb.FileRequest{
			Content:  []byte("Valid content"),
			FileType: "text",
		})
		require.NoError(t, err)
		require.False(t, resp.IsValid)
		require.Contains(t, resp.Message, "Task queue is full")
	})
}
