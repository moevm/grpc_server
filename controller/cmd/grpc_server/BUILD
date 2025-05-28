load("@rules_go//go:def.bzl", "go_binary")

go_binary(
    name = "grpc_server",
    srcs = ["main.go"],
    deps = [
        "//internal/config",
        "//internal/grpcserver",
        "//internal/manager",  # Добавлено
        "//pkg/proto/file_service:file_service_go_proto",
        "@org_golang_google_grpc//:go_default_library",
        "@org_golang_google_grpc//reflection:go_default_library",  # Добавлено
        "@com_github_joho_godotenv//:go_default_library",
    ],
)