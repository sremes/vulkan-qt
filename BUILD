# BUILD

load("@bazel_rules_qt//:qt.bzl", "qt_cc_library")

qt_cc_library(
    name = "vulkan_application",
    srcs = ["src/vulkan_application.cpp"],
    hdrs = ["include/vulkan_application.h"],
    strip_include_prefix = "include",
    deps = [
        ":graphics",
        "@qt//:qt_gui",
    ],
)

# https://docs.bazel.build/versions/master/be/c-cpp.html#cc_library
cc_library(
    name = "graphics",
    srcs = ["src/graphics.cpp"],
    hdrs = ["include/graphics.h"],
    strip_include_prefix = "include",
    deps = ["@glm"],
)

cc_binary(
    name = "vulkan_qt",
    srcs = glob(["src/main.cpp"]),
    copts = ["-fpic"],
    deps = [
        ":vulkan_application",
    ],
    data = [
        "//shaders:fragment_shader",
        "//shaders:vertex_shader",
    ],
)
