# Setup and Running Instructions

## Prerequisites
- Bazel 8.2.1 (required version)

## Building Components

### Controller

The controller is written in Go and uses Bazel for building:

Create and set permissions for the controller socket directory:
```bash
sudo mkdir -p /run/controller
sudo chmod 777 /run/controller

cd controller
bazel run //cmd/grpc_server:grpc_server
```

### Worker

The worker is written in C++ and also uses Bazel:

```bash
cd worker
bazel run :worker
```

To run the worker manually, you need to set the following environment variables:

```bash
export METRICS_GATEWAY_ADDRESS=localhost
export METRICS_GATEWAY_PORT=9091
export METRICS_WORKER_NAME=worker1

cd worker
bazel run :worker
```
