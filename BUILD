# BUILD

load("@bazel_rules_qt//:qt.bzl", "qt_cc_library")

qt_cc_library(
    name = "vulkan_application",
    src = "src/vulkan_application.cpp",
    hdr = "include/vulkan_application.h",
    deps = ["@qt//:qt_gui"],
    strip_include_prefix = "include",
)

cc_binary(
    name = "vulkan_qt",
    srcs = glob(["src/main.cpp"]),
    copts = ["-fpic"],
    deps = [
        ":vulkan_application"
    ],
)