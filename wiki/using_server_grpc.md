## Instructions for starting the grpc server

- installing dependencies from the folder controller: ```go get -u google.golang.org/protobuf``` and ```go get -u google.golang.org/grpc```. It is also worth installing protoc-gen-go and protoc-gen-go-grpc ```go install google.golang.org/protobuf/cmd/protoc-gen-go@latest``` and ```go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@latest```
- Make sure that $GOPATH/bin is added to the PATH ```export PATH="$PATH:$(go env GOPATH)/bin"```
- Check the protocol version ```protoc --version``` and check for plugins ```which protoc-gen-go``` ```which protoc-gen-go-grpc```
- Generate the code with explicit paths:
```bash
protoc \
  --plugin=protoc-gen-go=$(which protoc-gen-go) \
  --plugin=protoc-gen-go-grpc=$(which protoc-gen-go-grpc) \
  --go_out=. --go_opt=paths=source_relative \
  --go-grpc_out=. --go-grpc_opt=paths=source_relative \
  pkg/proto/file_service/file_service.proto
```
- run server ```go run cmd/grpc_server/main.go```