cc_binary(
  name = "worker",
  srcs = [
    "src/main.cpp",
    "src/md_calculator.cpp",
    "src/file.cpp",
    "src/worker.cpp",
    "src/metrics_collector.cpp",
    "include/file.hpp",
    "include/md_calculator.hpp",
    "include/worker.hpp",
    "include/metrics_collector.hpp",
  ],
  deps = ["@spdlog//:spdlog", 
          "@curl//:curl", 
          ":communication_cc_proto"],
  includes = ["/usr/local/include"],
  linkopts = [
    "-L/usr/local/openssl/lib",
    "-lssl", 
    "-lcrypto",
    "-L/usr/local/lib", 
    "-lprometheus-cpp-push", 
    "-lprometheus-cpp-core",
  ],
)

cc_proto_library(
    name = "communication_cc_proto",
    deps = [":communication_proto"],
)

proto_library(
    name = "communication_proto",
    srcs = [ "communication.proto" ],
    deps = [
        "@protobuf//:struct_proto",
    ],
)
