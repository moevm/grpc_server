load("@rules_go//go:def.bzl", "go_library")

go_library(
    name = "storage",
    srcs = [
        "redis.go"
    ],
    importpath = "github.com/moevm/grpc_server/internal/service/storage",
    visibility = ["//visibility:public"],
    deps = [
        "@com_github_redis_go_redis_v9//:go-redis",
    ],
)