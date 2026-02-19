load("@rules_go//go:def.bzl", "go_library", "go_test")

go_library(
    name = "grpcserver",
    srcs = [
        "server.go",
        "admin_server.go",
    ],
    importpath = "github.com/moevm/grpc_server/internal/grpcserver",
    visibility = ["//visibility:public"],
    deps = [
        "//internal/manager",
        "//pkg/proto/file_service:file_service_go_proto",
        "//pkg/proto/admin_service:admin_service_go_proto",
        "@org_golang_google_grpc//codes",
        "@org_golang_google_grpc//status",
        "@org_golang_google_grpc//:go_default_library",
    ]
)

go_test(
    name = "grpcserver_test",
    srcs = ["server_test.go"],
    embed = [":grpcserver"],
    deps = [
        "@com_github_stretchr_testify//assert",
        "@com_github_stretchr_testify//require",
        "//internal/manager",
        "//pkg/proto/file_service:file_service_go_proto",
        "@org_golang_google_grpc//codes",
        "@org_golang_google_grpc//status",
    ],
)
