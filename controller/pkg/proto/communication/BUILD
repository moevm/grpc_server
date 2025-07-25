load("@rules_proto//proto:defs.bzl", "proto_library")
load("@rules_go//proto:def.bzl", "go_proto_library")

proto_library(
    name = "communication_proto",
    srcs = ["communication.proto"],
    visibility = ["//visibility:public"],
)

go_proto_library(
    name = "communication_go_proto",
    compilers = ["@rules_go//proto:go_grpc"],
    importpath = "github.com/moevm/grpc_server/pkg/proto/communication",
    proto = ":communication_proto",
    visibility = ["//visibility:public"],
    deps = [
        "@org_golang_google_protobuf//types/known/anypb:go_default_library",
        "@org_golang_google_grpc//:go_default_library",
    ],
)
