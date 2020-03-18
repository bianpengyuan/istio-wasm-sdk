package(default_visibility = ["//visibility:public"])

cc_library(
    name = "proxy_wasm_intrinsics",
    srcs = [
        "proxy_wasm_intrinsics.cc",
    ],
    hdrs = [
        "proxy_wasm_api.h",
        "proxy_wasm_common.h",
        "proxy_wasm_enums.h",
        "proxy_wasm_externs.h",
        "proxy_wasm_intrinsics.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":proxy_wasm_intrinsics_cc_proto",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_proto_library(
    name = "proxy_wasm_intrinsics_cc_proto",
    deps = [":proxy_wasm_intrinsics_proto"],
)

proto_library(
    name = "proxy_wasm_intrinsics_proto",
    srcs = ["proxy_wasm_intrinsics.proto"],
    deps = [
        "@com_google_protobuf//:any_proto",
        "@com_google_protobuf//:duration_proto",
        "@com_google_protobuf//:struct_proto",
        "@com_google_protobuf//:empty_proto",
    ],
)

filegroup(
    name = "jslib",
    srcs = [
        "proxy_wasm_intrinsics.js",
    ],
    visibility = ["//visibility:public"],
)
