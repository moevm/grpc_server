load("@grpc//bazel:generate_cc.bzl", "generate_cc")

proto_library(
    name = "communication_proto",
    srcs = ["communication.proto"],
    deps = [
        "@com_google_protobuf//:empty_proto",
        "@com_google_protobuf//:struct_proto",
    ],
)

cc_proto_library(
    name = "communication_cc_proto",
    deps = [":communication_proto"],
)

generate_cc(
    name = "communication_cc_grpc_gen",
    srcs = [":communication_proto"],
    plugin = "@grpc//src/compiler:grpc_cpp_plugin",
    well_known_protos = True,
    generate_mocks = True,
)

cc_library(
    name = "communication_cc_grpc",
    srcs = [":communication_cc_grpc_gen"],
    hdrs = [":communication_cc_grpc_gen"],
    deps = [
        ":communication_cc_proto",
        "@grpc//:grpc++",
    ],
)

cc_library(
    name = "worker_headers",
    hdrs = [
        "include/worker.hpp",
        "include/md_calculator.hpp",
        "include/file.hpp",
        "include/metrics_collector.hpp",
    ],
    srcs = [],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "worker",
    srcs = [
        "src/main.cpp",
        "src/md_calculator.cpp",
        "src/file.cpp",
        "src/worker.cpp",
        "src/metrics_collector.cpp",
    ],
    deps = [
        ":worker_headers",
        ":communication_cc_proto",
        ":communication_cc_grpc",
        "@grpc//:grpc++",
        "@spdlog//:spdlog",
        "@curl//:curl",
    ],
    copts = [
        "-I$(GENDIR)/..",
    ],
    linkopts = [
        "-L/usr/local/openssl/lib",
        "-lssl",
        "-lcrypto",
        "-L/usr/local/lib",
        "-lprometheus-cpp-push",
        "-lprometheus-cpp-core",
    ],
)
