BIN_DIR = bin
PROTO_DIR = pkg/proto
GRPC_SERVER = cmd/grpc_server
MANAGER = cmd/manager

all: generate build 

generate:
	protoc --go_out=. --go_opt=paths=source_relative \
	--go-grpc_out=. --go-grpc_opt=paths=source_relative \
	$(PROTO_DIR)/file_service/file_service.proto \
	$(PROTO_DIR)/communication/communication.proto

build: generate
	mkdir -p $(BIN_DIR)
	go build -o $(BIN_DIR)/grpc_server ./$(GRPC_SERVER)
	go build -o $(BIN_DIR)/manager ./$(MANAGER)

clean:
	rm -rf $(BIN_DIR)
	rm -f $(PROTO_DIR)/communication/*.pb.go
	rm -f $(PROTO_DIR)/file_service/*.pb.go

run-server: build
	./$(BIN_DIR)/grpc_server

run-manager: build
	./$(BIN_DIR)/manager

