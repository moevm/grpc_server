load("@rules_go//go:def.bzl", "go_library", "go_test")

go_library(
    name = "manager",
    srcs = ["manager.go"],
    importpath = "github.com/moevm/grpc_server/internal/manager",
    visibility = ["//visibility:public"],
    deps = [
        "//internal/conn",
        "//pkg/converter",
    ],
)

go_test(
    name = "manager_test",
    size = "large",
    srcs = ["manager_test.go"],
    embed = [":manager"],
    tags = ["integration"],
    deps = ["//internal/worker"],
)
