load("@rules_go//go:def.bzl", "go_library")

go_library(
    name = "conn",
    srcs = ["unix.go"],
    importpath = "github.com/moevm/grpc_server/internal/conn",
    visibility = ["//visibility:public"],
    deps = [
        "//pkg/converter",
    ],
)
