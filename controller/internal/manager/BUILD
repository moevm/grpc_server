load("@rules_go//go:def.bzl", "go_library", "go_test")

go_library(
    name = "manager",
    srcs = [
        "manager.go",
    ],
    importpath = "github.com/moevm/grpc_server/internal/manager",
    visibility = ["//visibility:public"],
    deps = [
        "//internal/conn",
        "//pkg/converter",
        "//pkg/proto/communication:communication_go_proto",
        "@org_golang_google_protobuf//proto",
        "@com_github_joho_godotenv//:go_default_library",
    ],
)

go_test(
    name = "manager_test",
    srcs = ["manager_test.go"],
    embed = [":manager"],
    deps = [
        "//internal/worker",
        "//pkg/converter",
    ],
    size = "large", 
    tags = ["integration"], 
)
