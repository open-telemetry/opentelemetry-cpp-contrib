package(default_visibility = ["//visibility:public"])

cc_library(
    name = "prometheus_push_exporter",
    srcs = [
        "src/push_exporter.cc",
    ],
    hdrs = [
        "include/opentelemetry/exporters/prometheus/push_exporter.h",
    ],
    strip_include_prefix = "include",
    tags = ["prometheus"],
    deps = [
        "@com_github_jupp0r_prometheus_cpp//core",
        "@com_github_jupp0r_prometheus_cpp//push",
        "@io_opentelemetry_cpp//api",
        "@io_opentelemetry_cpp//exporters/prometheus:prometheus_collector",
        "@io_opentelemetry_cpp//exporters/prometheus:prometheus_exporter_utils",
        "@io_opentelemetry_cpp//sdk:headers",
    ],
)

cc_library(
    name = "prometheus_test_helper",
    hdrs = [
        "test/prometheus_test_helper.h",
    ],
    tags = ["prometheus"],
    deps = [
        "@io_opentelemetry_cpp//api",
        "@io_opentelemetry_cpp//sdk:headers",
        "@io_opentelemetry_cpp//sdk/src/trace",
    ],
)

cc_test(
    name = "prometheus_push_exporter_test",
    srcs = [
        "test/push_exporter_test.cc",
    ],
    tags = [
        "prometheus",
        "test",
    ],
    deps = [
        ":prometheus_push_exporter",
        ":prometheus_test_helper",
        "@com_google_googletest//:gtest_main",
        "@io_opentelemetry_cpp//api",
        "@io_opentelemetry_cpp//exporters/prometheus:prometheus_collector",
        "@io_opentelemetry_cpp//exporters/prometheus:prometheus_exporter_utils",
        "@io_opentelemetry_cpp//sdk:headers",
    ],
)
