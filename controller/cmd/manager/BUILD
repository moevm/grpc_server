load("@rules_go//go:def.bzl", "go_binary", "go_library")

go_binary(
    name = "manager",
    embed = [":manager_lib"],
    visibility = ["//visibility:public"],
)

go_library(
    name = "manager_lib",
    srcs = ["main.go"],
    importpath = "github.com/moevm/grpc_server/cmd/manager",
    visibility = ["//visibility:private"],
    deps = ["//internal/manager"],
)
