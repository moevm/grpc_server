load("@rules_go//go:def.bzl", "go_binary", "go_library")

go_binary(
    name = "worker",
    embed = [":worker_lib"],
    visibility = ["//visibility:public"],
)

go_library(
    name = "worker_lib",
    srcs = ["main.go"],
    importpath = "github.com/moevm/grpc_server/cmd/worker",
    visibility = ["//visibility:private"],
    deps = ["//internal/worker"],
)
