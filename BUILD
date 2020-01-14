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
        # "request_info.cc",
        "util.cc",
        "node_info_cache.cc",
        "node_info.pb.cc",
        # "request_info.pb.cc",
    ],
    hdrs = [
        "context.h",
        # "request_info.h",
        "util.h",
        "node_info_cache.h",
        "node_info.pb.h",
        # "request_info.pb.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@com_google_protobuf//:protobuf",
        "@envoy_wasm_api//:proxy_wasm_intrinsics",
    ],
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
