load("@rules_proto//proto:defs.bzl", "proto_library")
load("@rules_go//proto:def.bzl", "go_proto_library")

proto_library(
    name = "file_service_proto",
    srcs = ["file_service.proto"],
    visibility = ["//visibility:public"],
)

go_proto_library(
    name = "file_service_go_proto",
    compilers = ["@rules_go//proto:go_grpc"],
    importpath = "github.com/moevm/grpc_server/pkg/proto/file_service",
    proto = ":file_service_proto",
    visibility = ["//visibility:public"],
    deps = [
        "@org_golang_google_protobuf//types/known/anypb:go_default_library",
        "@org_golang_google_grpc//:go_default_library",
    ],
)