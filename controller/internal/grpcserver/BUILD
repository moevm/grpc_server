load("@rules_go//go:def.bzl", "go_library", "go_test")

go_library(
    name = "grpcserver",
    srcs = ["server.go"],
    importpath = "github.com/moevm/grpc_server/internal/grpcserver",
    visibility = ["//visibility:public"],
    deps = [
        "//internal/manager",
        "//pkg/proto/file_service:file_service_go_proto",
        "@org_golang_google_grpc//codes",
        "@org_golang_google_grpc//status",
    ],
)

go_test(
    name = "grpcserver_test",
    srcs = ["test.go"],
    deps = [
        ":grpcserver",
        "@com_github_stretchr_testify//assert",
        "@com_github_stretchr_testify//require",
        "//internal/manager",
        "//pkg/proto/file_service:file_service_go_proto",
        "@org_golang_google_grpc//codes",
        "@org_golang_google_grpc//status",
    ],
)
