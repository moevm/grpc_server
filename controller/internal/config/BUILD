load("@rules_go//go:def.bzl", "go_library")

go_library(
    name = "config",
    srcs = ["config.go"],
    importpath = "github.com/moevm/grpc_server/internal/config",
    visibility = ["//visibility:public"],
    deps = ["@com_github_joho_godotenv//:go_default_library"],
)