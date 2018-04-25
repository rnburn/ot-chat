git_repository(
    name = "com_github_nelhage_rules_boost",
    commit = "5e746bc69de4142ce467b372339ab110a8d67781",
    remote = "https://github.com/nelhage/rules_boost",
)

load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")
boost_deps()

new_git_repository(
    name = "com_github_gabime_spdlog",
    commit = "93d41b2c0ecd0db7075e2386596ce39cb20546c9",
    remote = "https://github.com/gabime/spdlog",
    build_file = "bazel/BUILD.spdlog",
)

http_archive(
    name = "com_google_protobuf",
    sha256 = "cef7f1b5a7c5fba672bec2a319246e8feba471f04dcebfe362d55930ee7c1c30",
    strip_prefix = "protobuf-3.5.0",
    urls = ["https://github.com/google/protobuf/archive/v3.5.0.zip"],
)

git_repository(
    name   = "com_github_gflags_gflags",
    #tag    = "v2.2.1",
    commit = "46f73f88b18aee341538c0dfc22b1710a6abedef",
    remote = "https://github.com/gflags/gflags.git"
)

git_repository(
    name = "io_opentracing_cpp",
    commit = "630495ca7d45c21a4b9ca2f7c03f80c1d9c96f70",
    remote = "https://github.com/opentracing/opentracing-cpp.git",
)
