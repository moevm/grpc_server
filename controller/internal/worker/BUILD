load("@rules_go//go:def.bzl", "go_library")

go_library(
    name = "worker",
    srcs = ["worker.go"],
    importpath = "github.com/moevm/grpc_server/internal/worker",
    visibility = ["//visibility:public"],
    deps = [
        "//internal/conn",
        "//pkg/converter",
    ],
)
