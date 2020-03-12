load("@rules_proto//proto:defs.bzl", "proto_library")

# Copyright 2019 Istio Authors. All Rights Reserved.
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
    name = "node_info",
    srcs = [
        "node_info.cc",
        "node_info_cache.cc",
    ],
    hdrs = [
        "node_info.h",
        "node_info_cache.h",
    ],
    visibility = [
        "//istio/extension:__pkg__",
    ],
    deps = [
        ":node_info_cc_proto",
        "//istio/extension/util",
        "@com_google_absl//absl/strings",
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
)

cc_proto_library(
    name = "node_info_cc_proto",
    visibility = ["//visibility:public"],
    deps = ["node_info_proto"],
)

proto_library(
    name = "node_info_proto",
    srcs = ["node_info.proto"],
)

proto_library(
    name = "istio_extension_proto",
    srcs = ["node_info.proto"],
    visibility = ["//visibility:public"],
)
