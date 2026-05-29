load("@grpc//bazel:generate_cc.bzl", "generate_cc")

proto_library(
    name = "communication_proto",
    srcs = ["communication.proto"],
    deps = [
        "@com_google_protobuf//:any_proto",
        "@com_google_protobuf//:struct_proto",
        "@com_google_protobuf//:empty_proto",
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
    well_known_protos = False,
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
        "include/metrics_collector.hpp",
        "include/dpdk_filter/net_port.h",
        "include/dpdk_filter/ip_cache.h",
        "include/dpdk_filter/domain_cache.h",
        "include/dpdk_filter/filtr_packets.h",
        "include/dpdk_filter/pars_packets.h",
        "include/dpdk_filter/proc_packets.h",
        "include/dpdk_filter/types.h",
        "include/dpdk_filter/constants.h",
    ],
    includes = ["include", "include/dpdk_filter"],
    srcs = [],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "worker",
    srcs = [
        "src/main.cpp",
        "src/worker.cpp",
        "src/dpdk_filter/ip_cache.c",
        "src/dpdk_filter/domain_cache.c",
        "src/metrics_collector.cpp",
        "src/dpdk_filter/net_port.c", 
        "src/dpdk_filter/filtr_packets.c",
        "src/dpdk_filter/pars_packets.c",
        "src/dpdk_filter/proc_packets.c",
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
    ] + select({
        "@platforms//cpu:x86_64": [
            "-mssse3",
            "-msse4.2",
            "-mpclmul",
            "-maes",
            "-I/usr/include",
        ],
        "@platforms//cpu:riscv64": [],
        "//conditions:default": ["-I/usr/include"],
    }),
    cxxopts = [
        "-std=c++17",
    ],
    linkopts = [
        "-lprometheus-cpp-push",
        "-lprometheus-cpp-core",
        "-lsqlite3",
        "-lssl",
        "-lcrypto",
        "-lnuma",
        "-ldl",
        "-lpthread",
    ] + select({
        "@platforms//cpu:x86_64": [
            "-L/usr/local/openssl/lib",
            "-L/usr/local/lib",
            "-L/usr/lib",
            "-lrte_eal",
            "-lrte_ethdev",
            "-lrte_mempool",
            "-lrte_mbuf",
            "-lrte_bus_vdev",
            "-lrte_ring",
            "-lrte_telemetry",
            "-lrte_kvargs",
            "-lrte_log",
            "-lrte_net",
            "-lrte_hash",
            "-lrte_timer",
        ],
        "@platforms//cpu:riscv64": [
            "-Wl,--whole-archive",
            "-lrte_eal",
            "-lrte_ethdev",
            "-lrte_mempool",
            "-lrte_mbuf",
            "-lrte_bus_vdev",
            "-lrte_ring",
            "-lrte_telemetry",
            "-lrte_kvargs",
            "-lrte_log",
            "-lrte_net",
            "-lrte_hash",
            "-lrte_timer",
            "-lrte_net_af_xdp",
            "-Wl,--no-whole-archive",
            "-lbpf",
            "-lxdp",
            "-lelf",
            "-lz",
        ],
        "//conditions:default": [
            "-lrte_eal",
            "-lrte_ethdev",
            "-lrte_mempool",
            "-lrte_mbuf",
            "-lrte_bus_vdev",
            "-lrte_ring",
            "-lrte_telemetry",
            "-lrte_kvargs",
            "-lrte_log",
            "-lrte_net",
            "-lrte_hash",
            "-lrte_timer",
        ],
    }),
)
