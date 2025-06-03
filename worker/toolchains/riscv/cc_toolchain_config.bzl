def _impl(ctx):
    tool_paths = [
        tool_path(
            name = "gcc",
            path = "/usr/bin/riscv64-linux-gnu-gcc",
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
                                "-lstdc++",
                                "-L/usr/local/lib",
                                "-lssl",
                                "-lcrypto",
                                "-lprometheus-cpp-push",
                                "-lprometheus-cpp-core",
                                "-lcurl",
                            ],
                        ),
                    ],
                ),
            ],
        ),
        feature(
            name = "builtin_sysroot",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = all_link_actions + [
                        ACTION_NAMES.cpp_compile,
                        ACTION_NAMES.c_compile,
                    ],
                    flag_groups = [
                        flag_group(
                            flags = [
                                "--sysroot=/usr/riscv64-linux-gnu",
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
            "/usr/lib/gcc-cross/riscv64-linux-gnu/11/include",
            "/usr/lib/gcc-cross/riscv64-linux-gnu/11/include-fixed",
            "/usr/riscv64-linux-gnu/include",
            "/usr/include",
            "/usr/local/include",
        ],
        toolchain_identifier = "riscv-toolchain",
        host_system_name = "linux",
        target_system_name = "riscv64-linux-gnu",
        target_cpu = "riscv64",
        target_libc = "glibc",
        compiler = "riscv-gcc",
        abi_version = "lp64d",
        abi_libc_version = "glibc_2.36",
        tool_paths = tool_paths,
    )