package grpcserver

import (
	"context"
	"unicode"
	"unicode/utf8"
	pb "github.com/moevm/grpc_server/pkg/proto/file_service"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

const (
	ValidChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 !?.,\n"
)

type Server struct {
	pb.UnimplementedFileServiceServer
}

var availableCharacters = initAvailableCharacters()

func initAvailableCharacters() map[rune]bool {
	chars := make(map[rune]bool)
	for _, c := range ValidChars {
		chars[c] = true
	}
	return chars
}

func (s *Server) UploadFile(ctx context.Context, req *pb.FileRequest) (*pb.FileResponse, error) {
	content := req.GetContent()
	fileType := req.GetFileType()

	var isValid bool
	size := int64(len(content))

	switch fileType {
	case "text":
		isValid = validateText(content)
	case "binary":
		isValid = validateBinary(content)
	default:
		return nil, status.Errorf(codes.InvalidArgument, "invalid file type: %s", fileType)
	}

	msg := "Validation successful"
	if !isValid {
		msg = "Invalid file content"
	}

	return &pb.FileResponse{
		Size:    size,
		IsValid: isValid,
		Message: msg,
	}, nil
}

func validateText(content []byte) bool {
	if !utf8.Valid(content) {
		return false
	}

	str := string(content)
	for _, r := range str {
		if !availableCharacters[r] && !unicode.IsSpace(r) {
			return false
		}
	}
	return true
}

func validateBinary(content []byte) bool {
	return true
}