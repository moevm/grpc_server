load("@bazel_tools//tools/build_defs/cc:action_names.bzl", "ACTION_NAMES")
load(
    "@bazel_tools//tools/cpp:cc_toolchain_config_lib.bzl",
    "feature",
    "flag_group",
    "flag_set",
    "tool_path",
)

all_link_actions = [
    ACTION_NAMES.cpp_link_executable,
    ACTION_NAMES.cpp_link_dynamic_library,
    ACTION_NAMES.cpp_link_nodeps_dynamic_library,
]

all_compile_actions = [
    ACTION_NAMES.cpp_compile,
    ACTION_NAMES.c_compile,
]

def _impl(ctx):
    tool_paths = [
        tool_path(
            name = "gcc",
            path = "/usr/bin/riscv64-linux-gnu-g++",
        ),
        tool_path(
            name = "ld",
            path = "/usr/bin/riscv64-linux-gnu-ld",
        ),
        tool_path(
            name = "ar",
            path = "/usr/bin/riscv64-linux-gnu-ar",
        ),
        tool_path(
            name = "cpp",
            path = "/bin/false",
        ),
        tool_path(
            name = "gcov",
            path = "/bin/false",
        ),
        tool_path(
            name = "nm",
            path = "/usr/bin/riscv64-linux-gnu-nm",
        ),
        tool_path(
            name = "objdump",
            path = "/usr/bin/riscv64-linux-gnu-objdump",
        ),
        tool_path(
            name = "strip",
            path = "/usr/bin/riscv64-linux-gnu-strip",
        ),
    ]

    features = [
        feature(
            name = "default_linker_flags",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = all_link_actions,
                    flag_groups = [
                        flag_group(
                            flags = [
                                "-L/usr/riscv64-linux-gnu/lib",
                                "-Wl,-rpath=/usr/riscv64-linux-gnu/lib",
                                "-Wl,--start-group",
                                "-lprometheus-cpp-push",
                                "-lprometheus-cpp-core",
                                "-lcurl",
                                "-lssl",
                                "-lcrypto",
                                "-lz",
                                "-Wl,--end-group",
                            ],
                        ),
                    ],
                ),
            ],
        ),
        feature(
            name = "cpp_compiler_flags",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = [ACTION_NAMES.cpp_compile],
                    flag_groups = [
                        flag_group(
                            flags = [
                                "-std=c++17",
                                "-isystem/usr/riscv64-linux-gnu/include/c++/11",
                                "-isystem/usr/riscv64-linux-gnu/include/c++/11/riscv64-linux-gnu",
                                "-isystem/usr/lib/gcc-cross/riscv64-linux-gnu/11/include",
                            ],
                        ),
                    ],
                ),
            ],
        ),
    ]

    return cc_common.create_cc_toolchain_config_info(
        ctx = ctx,
        features = features,
        cxx_builtin_include_directories = [
            "/usr/riscv64-linux-gnu/include/c++/11",
            "/usr/riscv64-linux-gnu/include/c++/11/riscv64-linux-gnu",
            "/usr/lib/gcc-cross/riscv64-linux-gnu/11/include",
            "/usr/lib/gcc-cross/riscv64-linux-gnu/11/include-fixed",
            "/usr/riscv64-linux-gnu/include",
            "/usr/include",
            "/usr/local/include",
        ],
        toolchain_identifier = "riscv-toolchain",
        host_system_name = "local",
        target_system_name = "riscv64-linux-gnu",
        target_cpu = "riscv64",
        target_libc = "glibc",
        compiler = "gcc",
        abi_version = "lp64d",
        abi_libc_version = "glibc_2.36",
        tool_paths = tool_paths,
    )

cc_toolchain_config = rule(
    implementation = _impl,
    attrs = {},
    provides = [CcToolchainConfigInfo],
)