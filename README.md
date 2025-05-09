# grpc_server
GRPC server with workers

# Controller
A controller for distributed task processing via Unix sockets

## Requirements
- Go 1.20+
- Write access to `/run/controller/`
    - If there is no directory, then it needs to be created `sudo mkdir -p run/controller/` and `sudo chmod 777 run/controller/`

## Install Dependencies
- go mod download

## Build and Launch
- `sudo go run cmd/manager/main.go`