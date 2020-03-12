load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def sdk_dependencies():
    http_archive(
        name = "emscripten_toolchain",
        url = "https://github.com/emscripten-core/emsdk/archive/a5082b232617c762cb65832429f896c838df2483.tar.gz",
        build_file = "@istio_wasm_sdk//bazel/external:emscripten-toolchain.BUILD",
        strip_prefix = "emsdk-a5082b232617c762cb65832429f896c838df2483",
        patch_cmds = [
            "./emsdk install 1.39.0-upstream",
            "./emsdk activate --embedded 1.39.0-upstream",
        ],
    )

    http_archive(
        name = "bazel_skylib",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.0.2/bazel-skylib-1.0.2.tar.gz",
            "https://github.com/bazelbuild/bazel-skylib/releases/download/1.0.2/bazel-skylib-1.0.2.tar.gz",
        ],
        sha256 = "97e70364e9249702246c0e9444bccdc4b847bed1eb03c5a3ece4f83dfe6abc44",
    )

    # this must be named com_google_protobuf to match dependency pulled in by
    # rules_proto.
    git_repository(
        name = "com_google_protobuf",
        remote = "https://github.com/protocolbuffers/protobuf",
        commit = "655310ca192a6e3a050e0ca0b7084a2968072260",
    )

    new_git_repository(
        name = "proxy_wasm_cpp_sdk",
        remote = "https://github.com/istio/envoy",
        commit = "ec96b58244665969eb3a4d5ca34b07b022360749",
        workspace_file_content = 'workspace(name = "proxy_wasm_cpp_sdk")',
        strip_prefix = "api/wasm/cpp",
        patch_cmds = ["rm BUILD"],
        build_file = "@istio_wasm_sdk//bazel/external:proxy-wasm-cpp-sdk.BUILD",
    )

    http_archive(
        name = "rules_proto",
        sha256 = "602e7161d9195e50246177e7c55b2f39950a9cf7366f74ed5f22fd45750cd208",
        strip_prefix = "rules_proto-97d8af4dc474595af3900dd85cb3a29ad28cc313",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/rules_proto/archive/97d8af4dc474595af3900dd85cb3a29ad28cc313.tar.gz",
            "https://github.com/bazelbuild/rules_proto/archive/97d8af4dc474595af3900dd85cb3a29ad28cc313.tar.gz",
        ],
    )

    http_archive(
        name = "com_google_absl",
        sha256 = "3df5970908ed9a09ba51388d04661803a6af18c373866f442cede7f381e0b94a",
        strip_prefix = "abseil-cpp-14550beb3b7b97195e483fb74b5efb906395c31e",
        # 2019-07-31
        urls = ["https://github.com/abseil/abseil-cpp/archive/14550beb3b7b97195e483fb74b5efb906395c31e.tar.gz"],
        patches = [
            "@istio_wasm_sdk//:bazel/patches/absl.patch",
        ],
        patch_args = ["-p1"],
    )
