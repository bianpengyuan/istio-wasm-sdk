load("@io_bazel_rules_go//go:def.bzl", "go_library")

go_library(
    name = "go_default_library",
    srcs = [
        "envoy.go",
        "envoy_conf.go",
        "framework.go",
        "http_client.go",
        "http_server.go",
        "ports.go",
        "setup.go",
        "tcp_envoy_conf.go",
        "tcp_server.go",
        "utils.go",
    ],
    importpath = "istio/test/framework/istio/test/framework",
    visibility = ["//visibility:public"],
    deps = [
        "@com_github_prometheus_client_model//go:go_default_library",
        "@com_github_prometheus_common//expfmt:go_default_library",
    ],
)
