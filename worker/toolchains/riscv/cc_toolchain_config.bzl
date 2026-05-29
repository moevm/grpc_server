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

_SDK_BASE = "/opt/riscv-sdk"
_SDK_HOST = _SDK_BASE + "/sysroots/x86_64-pokysdk-linux"
_SDK_TARGET = _SDK_BASE + "/sysroots/riscv64-poky-linux"
_TOOLCHAIN_BIN = _SDK_HOST + "/usr/bin/riscv64-poky-linux"

def _impl(ctx):
    tool_paths = [
        tool_path(
            name = "gcc",
            path = _TOOLCHAIN_BIN + "/riscv64-poky-linux-gcc",
        ),
        tool_path(
            name = "ld",
            path = _TOOLCHAIN_BIN + "/riscv64-poky-linux-ld",
        ),
        tool_path(
            name = "ar",
            path = _TOOLCHAIN_BIN + "/riscv64-poky-linux-ar",
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
            path = _TOOLCHAIN_BIN + "/riscv64-poky-linux-nm",
        ),
        tool_path(
            name = "objdump",
            path = _TOOLCHAIN_BIN + "/riscv64-poky-linux-objdump",
        ),
        tool_path(
            name = "strip",
            path = _TOOLCHAIN_BIN + "/riscv64-poky-linux-strip",
        ),
    ]

    features = [
        feature(
            name = "default_compile_flags",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = all_compile_actions,
                    flag_groups = [
                        flag_group(
                            flags = [
                                "--sysroot=" + _SDK_TARGET,
                                "-O2",
                                "-pipe",
                            ],
                        ),
                    ],
                ),
            ],
        ),
        feature(
            name = "default_linker_flags",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = all_link_actions,
                    flag_groups = [
                        flag_group(
                            flags = [
                                "--sysroot=" + _SDK_TARGET,
                                "-L" + _SDK_TARGET + "/usr/lib",
                                "-Wl,-rpath=" + _SDK_TARGET + "/usr/lib",
                                "-lstdc++",
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
            _SDK_HOST + "/usr/lib/riscv64-poky-linux/gcc/riscv64-poky-linux/13.4.0/include",
            _SDK_TARGET + "/usr/include/c++/13.4.0",
            _SDK_TARGET + "/usr/include/c++/13.4.0/riscv64-poky-linux",
            _SDK_TARGET + "/usr/include",
        ],
        toolchain_identifier = "riscv-yocto-toolchain",
        host_system_name = "local",
        target_system_name = "riscv64-poky-linux",
        target_cpu = "riscv64",
        target_libc = "glibc",
        compiler = "gcc",
        abi_version = "lp64d",
        abi_libc_version = "glibc_2.39",
        tool_paths = tool_paths,
    )

cc_toolchain_config = rule(
    implementation = _impl,
    attrs = {},
    provides = [CcToolchainConfigInfo],
)
