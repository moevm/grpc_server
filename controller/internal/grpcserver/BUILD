load("@rules_go//go:def.bzl", "go_library", "go_test")

go_library(
    name = "grpcserver",
    srcs = ["server.go"],
    importpath = "github.com/moevm/grpc_server/internal/grpcserver",
    visibility = ["//visibility:public"],
    deps = [
        "//pkg/proto/file_service:file_service_go_proto",
        "@org_golang_google_grpc//codes",
        "@org_golang_google_grpc//status",
    ],
)

go_test(
    name = "server_test",
    embed = [ ":grpcserver" ],
    srcs = [ 
        "server_unit_test.go",
        "server_integration_test.go"
    ],
    deps = [
        "//internal/config",
        "//pkg/proto/file_service:file_service_go_proto",
        "@com_github_stretchr_testify//assert",
        "@com_github_stretchr_testify//require",
        "@org_golang_google_grpc//:go_default_library",
        "@org_golang_google_grpc//codes",
        "@org_golang_google_grpc//credentials/insecure",
        "@org_golang_google_grpc//status",
    ],
)
