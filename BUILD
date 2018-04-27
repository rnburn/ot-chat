proto_library(
    name = "ot_chat_configuration_proto",
    srcs = ["configuration.proto"],
    deps = [
        "@com_google_protobuf//:struct_proto",
    ]
)

cc_proto_library(
    name = "ot_chat_configuration",
    deps = [":ot_chat_configuration_proto"],
)

proto_library(
    name = "chat_message_proto",
    srcs = ["chat_message.proto"],
)

cc_proto_library(
    name = "chat_message",
    deps = [":chat_message_proto"],
)

cc_binary(
    name = "ot-chat",
    srcs = glob([
        "src/**/*.h",
        "src/**/*.cpp",
    ]),
    deps = [
        ":ot_chat_configuration",
        ":chat_message",

        "@com_github_gflags_gflags//:gflags",
        "@com_github_gabime_spdlog//:spdlog",
        "@io_opentracing_cpp//:opentracing",
        "@boost//:beast",
        "@boost//:filesystem",
    ],
    copts = [
        "-std=c++14",
    ],
    linkopts = 
        select({
            "@bazel_tools//tools/osx:darwin": [
            ],
            "//conditions:default": [
                "-ldl",
                "-pthread",
                "-static-libstdc++",
                "-static-libgcc",
            ],
        }),
)
