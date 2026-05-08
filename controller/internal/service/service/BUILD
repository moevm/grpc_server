load("@rules_go//go:def.bzl", "go_library")

go_library(
    name = "service",
    srcs = ["service.go"],
    importpath = "github.com/moevm/grpc_server/internal/service/service",
    visibility = ["//visibility:public"],
    deps = [
        "//internal/service/client",
        "//internal/service/models",
        "//internal/service/parser",
        "@com_github_joho_godotenv//:go_default_library", 
    ],
)
