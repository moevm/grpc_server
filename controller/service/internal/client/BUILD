load("@rules_go//go:def.bzl", "go_library")

go_library(
    name = "client",
    srcs = ["http_client.go"],
    importpath = "service/internal/client",
    visibility = ["//visibility:public"],
    deps = ["//service/internal/models"],
)
