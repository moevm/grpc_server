load("@rules_go//go:def.bzl", "go_library")

go_library(
    name = "grpcserver",
    srcs = ["server.go"],
    importpath = "github.com/moevm/grpc_server/internal/grpcserver",
    visibility = ["//visibility:public"],
    deps = [
        "//internal/manager",
        "//pkg/proto/file_service:file_service_go_proto",
        "@org_golang_google_grpc//:go_default_library",
        "@org_golang_google_grpc//codes:go_default_library",
        "@org_golang_google_grpc//status:go_default_library",
        "@org_golang_x_text//unicode:go_default_library",
    ],
)
