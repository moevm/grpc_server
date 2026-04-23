load("@rules_go//go:def.bzl", "go_binary", "go_library")

go_library(
    name = "grpc_server_lib",
    srcs = ["main.go"],
    importpath = "github.com/moevm/grpc_server/cmd/grpc_server",
    visibility = ["//visibility:private"],
    deps = [
        "//internal/config",
        "//internal/grpcserver",
        "//internal/manager",
        "//internal/service/service",
        "//internal/service/storage",
        "//pkg/proto/admin_service:admin_service_go_proto",
        "//pkg/proto/communication:communication_go_proto",
        "@org_golang_google_grpc//:go_default_library",
        "@org_golang_google_grpc//reflection",
        "@com_github_subosito_gotenv//:go_default_library",
    ],
)

go_binary(
    name = "grpc_server",
    embed = [":grpc_server_lib"],
    visibility = ["//visibility:public"],
)