load("@rules_go//go:def.bzl", "go_library", "go_test")

go_library(
    name = "grpcserver",
    srcs = [
        "admin_server.go",
        "data_server.go",
    ],
    importpath = "github.com/moevm/grpc_server/internal/grpcserver",
    visibility = ["//visibility:public"],
    deps = [
        "//internal/service/service",
        "//internal/manager",
        "//internal/service/storage",
        "//pkg/proto/admin_service:admin_service_go_proto",
        "//pkg/proto/communication:communication_go_proto",
        "@com_github_redis_go_redis_v9//:go_redis_v9",
        "@org_golang_google_grpc//codes",
        "@org_golang_google_grpc//status",
        "@org_golang_google_grpc//:go_default_library",
        "@org_golang_google_protobuf//types/known/emptypb",
        "@org_golang_google_protobuf//proto:go_default_library",
    ],
)