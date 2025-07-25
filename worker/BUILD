cc_library(
  name = "protobuf_x86",
  visibility = ["//visibility:public"],
  linkopts = [
    "-lprotobuf", "-labsl_log_internal_check_op", "-labsl_leak_check", "-labsl_die_if_null", "-labsl_log_internal_conditions",
    "-labsl_log_internal_message", "-labsl_examine_stack", "-labsl_log_internal_format", "-labsl_log_internal_proto",
    "-labsl_log_internal_nullguard", "-labsl_log_internal_log_sink_set", "-labsl_log_sink",
    "-labsl_flags_internal", "-labsl_flags_marshalling", "-labsl_flags_reflection", "-labsl_flags_private_handle_accessor",
    "-labsl_flags_commandlineflag", "-labsl_flags_commandlineflag_internal", "-labsl_flags_config", "-labsl_flags_program_name",
    "-labsl_log_initialize", "-labsl_log_globals", "-labsl_vlog_config_internal", "-labsl_log_internal_fnmatch", "-labsl_log_internal_globals",
    "-labsl_raw_hash_set", "-labsl_hash", "-labsl_city", "-labsl_hashtablez_sampler", "-labsl_statusor",
    "-labsl_status", "-labsl_cord", "-labsl_cordz_info", "-labsl_cord_internal", "-labsl_cordz_functions", "-labsl_exponential_biased",
    "-labsl_cordz_handle", "-labsl_crc_cord_state", "-labsl_crc32c", "-labsl_crc_internal", "-labsl_crc_cpu_detect",
    "-labsl_strerror", "-labsl_str_format_internal", "-labsl_synchronization", "-labsl_graphcycles_internal",
    "-labsl_kernel_timeout_internal", "-labsl_stacktrace", "-labsl_symbolize", "-labsl_debugging_internal", "-labsl_demangle_internal",
    "-labsl_demangle_rust", "-labsl_decode_rust_punycode", "-labsl_utf8_for_code_point", "-labsl_malloc_internal", "-labsl_time",
    "-labsl_civil_time", "-labsl_strings", "-labsl_strings_internal", "-labsl_string_view", "-labsl_base", "-lrt", "-labsl_spinlock_wait",
    "-labsl_int128", "-labsl_throw_delegate", "-labsl_time_zone", "-labsl_raw_logging_internal", "-labsl_log_severity",
  ],
  target_compatible_with = ["@platforms//cpu:x86_64"]
)

cc_library(
  name = "protobuf_riscv",
  visibility = ["//visibility:public"],
  linkopts = [
    "-L/usr/riscv64-linux-gnu/lib",
    "-lprotobuf", "-labsl_log_internal_check_op", "-labsl_leak_check", "-labsl_die_if_null", "-labsl_log_internal_conditions",
    "-labsl_log_internal_message", "-labsl_examine_stack", "-labsl_log_internal_format", "-labsl_log_internal_proto",
    "-labsl_log_internal_nullguard", "-labsl_log_internal_log_sink_set", "-labsl_log_sink",
    "-labsl_flags_internal", "-labsl_flags_marshalling", "-labsl_flags_reflection", "-labsl_flags_private_handle_accessor",
    "-labsl_flags_commandlineflag", "-labsl_flags_commandlineflag_internal", "-labsl_flags_config", "-labsl_flags_program_name",
    "-labsl_log_initialize", "-labsl_log_globals", "-labsl_vlog_config_internal", "-labsl_log_internal_fnmatch", "-labsl_log_internal_globals",
    "-labsl_raw_hash_set", "-labsl_hash", "-labsl_city", "-labsl_hashtablez_sampler", "-labsl_statusor",
    "-labsl_status", "-labsl_cord", "-labsl_cordz_info", "-labsl_cord_internal", "-labsl_cordz_functions", "-labsl_exponential_biased",
    "-labsl_cordz_handle", "-labsl_crc_cord_state", "-labsl_crc32c", "-labsl_crc_internal", "-labsl_crc_cpu_detect",
    "-labsl_strerror", "-labsl_str_format_internal", "-labsl_synchronization", "-labsl_graphcycles_internal",
    "-labsl_kernel_timeout_internal", "-labsl_stacktrace", "-labsl_symbolize", "-labsl_debugging_internal", "-labsl_demangle_internal",
    "-labsl_demangle_rust", "-labsl_decode_rust_punycode", "-labsl_utf8_for_code_point", "-labsl_malloc_internal", "-labsl_time",
    "-labsl_civil_time", "-labsl_strings", "-labsl_strings_internal", "-labsl_string_view", "-labsl_base", "-lrt", "-labsl_spinlock_wait",
    "-labsl_int128", "-labsl_throw_delegate", "-labsl_time_zone", "-labsl_raw_logging_internal", "-labsl_log_severity",
  ],
  includes = ["/usr/riscv64-linux-gnu/include"],
  target_compatible_with = ["@platforms//cpu:riscv64"]
)

alias(
    name = "protobuf",
    actual = select({
        "@platforms//cpu:x86_64": ":protobuf_x86",
        "@platforms//cpu:riscv64": ":protobuf_riscv",
    })
)

cc_binary(
  name = "worker",
  srcs = [
    "src/main.cpp",
    "src/md_calculator.cpp",
    "src/file.cpp",
    "src/worker.cpp",
    "src/metrics_collector.cpp",
    "include/file.hpp",
    "include/md_calculator.hpp",
    "include/worker.hpp",
    "include/metrics_collector.hpp",
  ],
  deps = [
      ":protobuf",
      "@spdlog//:spdlog",
      ":communication_cc_proto"
      ],

)

cc_proto_library(
    name = "communication_cc_proto",
    deps = [":communication_proto"],
)

proto_library(
    name = "communication_proto",
    srcs = [ "communication.proto" ]
)