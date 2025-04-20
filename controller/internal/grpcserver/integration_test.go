package grpcserver_test

import (
	"context"
	"net"
	"testing"
	"time"
	"github.com/moevm/grpc_server/internal/config"
	"github.com/moevm/grpc_server/internal/grpcserver"
	pb "github.com/moevm/grpc_server/pkg/proto/file_service"
	"github.com/stretchr/testify/require"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
)

func startTestServer(t *testing.T) (string, func()) {
	cfg := config.Load()
	cfg.Host = "localhost"
	cfg.Port = "0" // random port

	lis, err := net.Listen("tcp", net.JoinHostPort(cfg.Host, cfg.Port))
	require.NoError(t, err)

	server := grpc.NewServer(
		grpc.MaxRecvMsgSize(cfg.MaxMessageSize),
		grpc.MaxSendMsgSize(cfg.MaxMessageSize),
	)
	pb.RegisterFileServiceServer(server, grpcserver.NewServer(cfg.AllowedChars))

	go func() {
		_ = server.Serve(lis)
	}()

	return lis.Addr().String(), func() {
		server.Stop()
		lis.Close()
	}
}

func TestIntegration_ServerClientCommunication(t *testing.T) {
    addr, stop := startTestServer(t)
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
        resp, err := client.UploadFile(ctx, &pb.FileRequest{
            Content:  []byte("Valid content"),
            FileType: "text",
        })
        require.NoError(t, err)
        require.True(t, resp.IsValid)
    })

    t.Run("invalid binary file", func(t *testing.T) {
        resp, err := client.UploadFile(ctx, &pb.FileRequest{
            Content:  make([]byte, 1024),
            FileType: "binary",
        })
        require.NoError(t, err)
        require.True(t, resp.IsValid)
    })
}