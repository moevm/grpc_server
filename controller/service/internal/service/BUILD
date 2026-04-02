load("@rules_go//go:def.bzl", "go_library")

go_library(
    name = "service",
    srcs = ["service.go"],
    importpath = "service/internal/service",
    visibility = ["//visibility:public"],
    deps = [
        "//service/internal/client",
        "//service/internal/models",
        "//service/internal/parser",
    ],
)
