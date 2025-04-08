load("@rules_go//go:def.bzl", "go_library")

go_library(
  name = "manager",
  srcs = ["manager.go"],
  deps = [
    "//pkg/converter",
  ],
  importpath = "github.com/moevm/grpc_server/internal/manager",
  visibility = ["//visibility:public"],
)
