# WORKSPACE

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")

git_repository(
    name = "bazel_rules_qt",
    branch = "master",
    remote = "https://github.com/justbuchanan/bazel_rules_qt.git",
)

new_local_repository(
    name = "qt",
    build_file = "@bazel_rules_qt//:qt.BUILD",
    # path = "/usr/include/qt", # May need configuring for your installation
    # For Qt5 on Ubuntu 16.04
    path = "/usr/include/x86_64-linux-gnu/qt5/",
)

new_git_repository(
    name = "glm",
    branch = "master",
    build_file = "@//:glm.BUILD",
    remote = "https://github.com/g-truc/glm.git",
)

git_repository(
    name = "gtest",
    remote = "https://github.com/google/googletest",
    tag = "release-1.10.0",
)

git_repository(
    name = "rules_vulkan",
    remote = "https://github.com/jadarve/rules_vulkan.git",
    tag = "v0.0.6",
)

load("@rules_vulkan//vulkan:repositories.bzl", "vulkan_repositories")

vulkan_repositories()
