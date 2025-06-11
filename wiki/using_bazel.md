# Setup and Running Instructions

## Prerequisites
- Bazel 8.2.1 (required version)

## Building Components

### Controller

The controller is written in Go and uses Bazel for building:

1. Create the runtime directory and assign ownership to the user under which the controller will run (typically, this is the user currently logged in):

```bash
sudo mkdir -p /run/controller
sudo chown <user>:<group> /run/controller
```

> **Note:**
> Replace `<user>:<group>` with the appropriate username and group that will run the controller.
> For example, if you are logged in as `smile`, use `smile:smile`.

2. Launch the controller from the `controller` directory:

```bash
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
