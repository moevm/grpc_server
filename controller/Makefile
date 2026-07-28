BIN_DIR = bin
PROTO_DIR = pkg/proto
GRPC_SERVER = cmd/grpc_server
MANAGER = cmd/manager

all: generate build 

generate:
	protoc --go_out=. --go_opt=paths=source_relative \
	--go-grpc_out=. --go-grpc_opt=paths=source_relative \
	$(PROTO_DIR)/admin_service/admin_service.proto \
	$(PROTO_DIR)/communication/communication.proto

build: generate
	mkdir -p $(BIN_DIR)
	go build -o $(BIN_DIR)/grpc_server ./$(GRPC_SERVER)
	go build -o $(BIN_DIR)/manager ./$(MANAGER)

build-riscv:
	bazel build --platforms=@rules_go//go/toolchain:linux_riscv64 //cmd/grpc_server:grpc_server
	mkdir -p $(BIN_DIR)
	cp -f bazel-bin/cmd/grpc_server/grpc_server_/grpc_server $(BIN_DIR)/grpc_server 2>/dev/null || cp -f bazel-bin/cmd/grpc_server/grpc_server $(BIN_DIR)/grpc_server

clean:
	rm -rf $(BIN_DIR)
	rm -f $(PROTO_DIR)/communication/*.pb.go
	rm -f $(PROTO_DIR)/admin_service/*.pb.go

run-server: build
	./$(BIN_DIR)/grpc_server

run-manager: build
	./$(BIN_DIR)/manager
