load("@rules_go//go:def.bzl", "go_test")

go_test(
    name = "integration_test",
    srcs = ["worker_test.go"],
    deps = [
        "@com_github_stretchr_testify//assert",
    ],
    args = ["--test.v"],
    tags = ["exclusive"],
)