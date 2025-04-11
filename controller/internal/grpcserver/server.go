package grpcserver

import (
	"context"
	"unicode"
	"unicode/utf8"
	pb "github.com/moevm/grpc_server/pkg/proto/file_service"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

type Server struct {
	pb.UnimplementedFileServiceServer
	allowedChars string
	charMap      map[rune]bool
}

func NewServer(allowedChars string) *Server {
	s := &Server{
		allowedChars: allowedChars,
	}
	s.charMap = s.initCharMap()
	return s
}

func (s *Server) initCharMap() map[rune]bool {
	chars := make(map[rune]bool)
	for _, c := range s.allowedChars {
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
		isValid = s.validateText(content)
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

func (s *Server) validateText(content []byte) bool {
	if !utf8.Valid(content) {
		return false
	}

	str := string(content)
	for _, r := range str {
		if !s.charMap[r] && !unicode.IsSpace(r) {
			return false
		}
	}
	return true
}

func validateBinary(content []byte) bool {
	return true
}