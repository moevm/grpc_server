load("@rules_proto//proto:defs.bzl", "proto_library")
load("@rules_go//proto:def.bzl", "go_proto_library")

proto_library(
    name = "admin_service_proto",
    srcs = ["admin_service.proto"],
    visibility = ["//visibility:public"],
)


go_proto_library(
    name = "admin_service_go_proto",
    compilers = ["@rules_go//proto:go_grpc"],
    importpath = "github.com/moevm/grpc_server/pkg/proto/admin_service",
    proto = ":admin_service_proto",
    visibility = ["//visibility:public"],
)
