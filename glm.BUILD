package(default_visibility = ["//visibility:public"])

cc_library(
    name = "glm",
    hdrs = glob([
        "glm/*.hpp",
        "glm/**/*.hpp",
        "glm/*.h",
        "glm/**/*.h"
    ]),
    includes = ["."],
    textual_hdrs = glob(["glm/**/*.inl"]),
    visibility = ["//visibility:public"],
)
