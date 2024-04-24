# Copyright 2022, OpenTelemetry Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def io_opentelemetry_cpp_contrib_deps():
    maybe(
        http_archive,
        name = "io_opentelemetry_cpp",
        sha256 = "69b0fef380658e15be9d817bfcb32e3f5de96da652bcdce77b4e750ed8beddee",
        strip_prefix = "opentelemetry-cpp-1.15.0",
        urls = [
            "https://github.com/open-telemetry/opentelemetry-cpp/archive/v1.15.0.tar.gz",
        ],
    )

    # C++ Prometheus Client library.
    maybe(
        http_archive,
        name = "com_github_jupp0r_prometheus_cpp",
        sha256 = "b4eff62bcdba10efd6210b9fa8a5b2505ad8ea6c211968be79aeb2c4c2f97338",
        # strip_prefix = "prometheus-cpp-1.1.0",
        # 1.1.0 with bazel 6 support
        strip_prefix = "prometheus-cpp-81e208c250748657f1d5dab247e82c4429a931af",
        urls = [
            # "https://github.com/jupp0r/prometheus-cpp/archive/refs/tags/v1.1.0.tar.gz",
            "https://github.com/jupp0r/prometheus-cpp/archive/81e208c250748657f1d5dab247e82c4429a931af.tar.gz",
        ],
    )

    # bazel platforms
    maybe(
        http_archive,
        name = "platforms",
        sha256 = "5308fc1d8865406a49427ba24a9ab53087f17f5266a7aabbfc28823f3916e1ca",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/platforms/releases/download/0.0.6/platforms-0.0.6.tar.gz",
            "https://github.com/bazelbuild/platforms/releases/download/0.0.6/platforms-0.0.6.tar.gz",
        ],
    )

    # Abseil cpp
    # Please use 20200225.3 and enable patches below when using gcc 4.8-4.9
    maybe(
        http_archive,
        name = "com_google_absl",
        sha256 = "91ac87d30cc6d79f9ab974c51874a704de9c2647c40f6932597329a282217ba8",
        strip_prefix = "abseil-cpp-20220623.1",
        urls = [
            "https://github.com/abseil/abseil-cpp/archive/20220623.1.tar.gz",
            # "https://github.com/abseil/abseil-cpp/archive/20200225.3.tar.gz",
        ],
    )

    # GoogleTest framework.
    # Only needed for tests, not to build the OpenTelemetry library.
    # Please use 1.10.0 when using gcc 4.8-4.9
    maybe(
        http_archive,
        name = "com_google_googletest",
        sha256 = "81964fe578e9bd7c94dfdb09c8e4d6e6759e19967e397dbea48d1c10e45d0df2",
        strip_prefix = "googletest-release-1.12.1",
        # sha256 = "9dc9157a9a1551ec7a7e43daea9a694a0bb5fb8bec81235d8a1e6ef64c716dcb",
        # strip_prefix = "googletest-release-1.10.0",
        urls = [
            "https://github.com/google/googletest/archive/release-1.12.1.tar.gz",
            # "https://github.com/google/googletest/archive/release-1.10.0.tar.gz",
        ],
    )

    # Some versions of GoogleTest depend com_googlesource_code_re2.
    maybe(
        http_archive,
        name = "com_googlesource_code_re2",  # 2023-06-01
        sha256 = "1726508efc93a50854c92e3f7ac66eb28f0e57652e413f11d7c1e28f97d997ba",
        strip_prefix = "re2-03da4fc0857c285e3a26782f6bc8931c4c950df4",
        urls = ["https://github.com/google/re2/archive/03da4fc0857c285e3a26782f6bc8931c4c950df4.zip"],
    )
