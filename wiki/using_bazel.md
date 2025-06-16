# Setup and Running Instructions

## Prerequisites
- Bazel 8.2.1 (required version)

### Controller

```bash
cd controller
bazel run //cmd/grpc_server:grpc_server
```
### Worker

```bash
cd worker
bazel run :worker
```
