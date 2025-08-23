# grpc_server
GRPC server with workers

# Controller
A controller for distributed task processing via Unix sockets

## Requirements
- Go 1.20+

## Build and Launch with Bazel
### Controller (bazel)
To build binaries for the x86_64 architecture:
`bazel build --platforms=@rules_go//go/toolchain:linux_amd64 //cmd/grpc_server:grpc_server`

To build binary files for the RISC-V architecture:
`bazel build --platforms=@rules_go//go/toolchain:linux_riscv64 //cmd/grpc_server:grpc_server`

To run locally for x86_64, need to use: 
`bazel run //cmd/grpc_server:grpc_server`

p.s. before the new build, you should use the commands:
`rm -rf ~/.cache/bazel` and `bazel clean --expunge`

### Worker (bazel)

To build binaries for the x86_64 architecture (cross compile x86_64 to x86_64):
`bazel build //:worker --extra_toolchains=//toolchains/x86_64:cc_toolchain_for_linux_x86_64 --platforms=//platforms:x86_64_linux`

To build binary files for the RISC-V architecture (cross compile x86_64 to riscv):
`bazel build //:worker --extra_toolchains=//toolchains/x86_64:cc_toolchain_for_linux_x86_64 --platforms=//platforms:x86_64_linux`

Native build:
`bazel build //:worker`

To run locally
`bazel run //:worker` or `./bazel-bin/worker`

### Controller (go build)

To build binaries:
`make`

To run locally:
`make run-server`

p.s. Go build instruction is located in controller/Makefile, also generate proto files is needed (this instruction is also in Makefile).
