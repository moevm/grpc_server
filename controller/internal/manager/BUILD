load("@rules_go//go:def.bzl", "go_library", "go_test")

go_library(
  name = "manager",
  srcs = ["manager.go"],
  deps = [
    "//pkg/converter",
    "//internal/conn"
  ],
  importpath = "github.com/moevm/grpc_server/internal/manager",
  visibility = ["//visibility:public"],
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