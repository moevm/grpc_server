# grpc_server
GRPC server with workers

# Controller
A controller for distributed task processing via Unix sockets

## Requirements
- Go 1.20+

## Build and Launch with Bazel
### Controller
To build binaries for the x86_64 architecture:
`bazel build --platforms=@rules_go//go/toolchain:linux_amd64 //cmd/grpc_server:grpc_server`

To build binary files for the RISC-V architecture:
`bazel build --platforms=@rules_go//go/toolchain:linux_riscv64 //cmd/grpc_server:grpc_server`

To run locally for x86_64, need to use: 
`bazel run //cmd/grpc_server:grpc_server`

p.s. before the new build, you should use the commands:
`rm -rf ~/.cache/bazel` and `bazel clean --expunge`