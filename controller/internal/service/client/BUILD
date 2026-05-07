load("@rules_go//go:def.bzl", "go_library")

go_library(
    name = "client",
    srcs = ["http_client.go"],
    importpath = "github.com/moevm/grpc_server/internal/service/client",
    visibility = ["//visibility:public"],
    deps = ["//internal/service/models"],
)
