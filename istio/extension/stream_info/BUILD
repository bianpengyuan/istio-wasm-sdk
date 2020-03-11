load("@rules_proto//proto:defs.bzl", "proto_library")
load("@io_bazel_rules_go//go:def.bzl", "go_library")
load("@io_bazel_rules_go//proto:def.bzl", "go_proto_library")

# Copyright 2020 Istio Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
################################################################################
#

cc_library(
    name = "stream_info",
    hdrs = [
        "stream_info.h",
    ],
    visibility = [
        "//istio/extension:__pkg__",
    ],
    deps = [
        ":stream_info_cc_proto",
        "//istio/extension/node_info:node_info_cc_proto",
        "@com_google_absl//absl/strings",
    ],
)

cc_proto_library(
    name = "stream_info_cc_proto",
    visibility = [
        "//istio/extension:__pkg__",
    ],
    deps = ["stream_info_proto"],
)

proto_library(
    name = "stream_info_proto",
    srcs = ["stream_info.proto"],
    deps = [
        "@com_google_protobuf//:wrappers_proto",
    ],
)

proto_library(
    name = "istio_extension_proto",
    srcs = ["stream_info.proto"],
    visibility = ["//visibility:public"],
    deps = ["@com_google_protobuf//:wrappers_proto"],
)
