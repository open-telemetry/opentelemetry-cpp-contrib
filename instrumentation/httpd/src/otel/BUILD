load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "otelmodlib",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.h"]),
    copts = [
        "-I/usr/include/apache2",
        "-I/usr/include/apr-1.0",
    ],
    deps = [
        "@io_opentelemetry_cpp//api",
        "@io_opentelemetry_cpp//exporters/ostream:ostream_span_exporter",
        "@io_opentelemetry_cpp//exporters/otlp:otlp_exporter",
        "@io_opentelemetry_cpp//sdk/src/trace",
    ],
    alwayslink = 1,
)
