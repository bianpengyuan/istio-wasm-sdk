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

licenses(["notice"])

cc_library(
    name = "context",
    srcs = [
        "context.cc",
        "util.cc",
    ],
    hdrs = [
        "context.h",
        "util.h",
    ],
    # additional_linker_inputs = ["@envoy_wasm_api//:jslib"],
    # linkopts = [
    #     "--js-library",
    #     "external/envoy_wasm_api/proxy_wasm_intrinsics.js",
    # ],
    deps = [
        ":node_info_cc_proto",
        "@com_google_protobuf//:protobuf",
        "@envoy_wasm_api//:proxy_wasm_intrinsics",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "node_info_cache",
    srcs = [
        "node_info_cache.cc",
    ],
    hdrs = [
        "node_info_cache.h",
    ],
    # additional_linker_inputs = ["@envoy_wasm_api//:jslib"],
    # linkopts = [
    #     "--js-library",
    #     "external/envoy_wasm_api/proxy_wasm_intrinsics.js",
    # ],
    deps = [
        ":context",
        "@envoy_wasm_api//:proxy_wasm_intrinsics",
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

# cc_test(
#     name = "context_test",
#     size = "small",
#     srcs = ["context_test.cc"],
#     repository = "@envoy",
#     deps = [
#         ":context",
#         "@envoy//source/extensions/common/wasm:wasm_lib",
#     ],
# )

# cc_test(
#     name = "util_test",
#     size = "small",
#     srcs = ["util_test.cc"],
#     repository = "@envoy",
#     deps = [
#         ":context",
#         "@envoy//source/extensions/common/wasm:wasm_lib",
#     ],
# )
