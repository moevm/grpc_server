load("@rules_go//go:def.bzl", "go_test")
load("@rules_proto//proto:defs.bzl", "proto_library")
load("@rules_go//proto:def.bzl", "go_proto_library")

proto_library(
    name = "test_communication_proto",
    srcs = ["communication.proto"],
    visibility = ["//visibility:private"],
    deps = [
        "@protobuf//:struct_proto",
        "@protobuf//:empty_proto",
    ],
)

go_proto_library(
    name = "test_communication_go_proto",
    compilers = ["@rules_go//proto:go_grpc"],
    importpath = "github.com/moevm/grpc_server/controller/test",
    proto = ":test_communication_proto",
    visibility = ["//visibility:private"],
)

go_test(
    name = "integration_test",
    srcs = ["worker_test.go"],
    deps = [
        "//pkg/proto/communication:communication_go_proto", 
        "@org_golang_google_grpc//:grpc",
        "@org_golang_google_grpc//test/bufconn",
        "@org_golang_google_grpc//credentials/insecure",
        "@org_golang_google_protobuf//types/known/emptypb:go_default_library", 
        "@com_github_stretchr_testify//assert",
    ],
    args = ["-test.v"],
    tags = ["exclusive"],
)
