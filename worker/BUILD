# i don't know how to use pkg-config in Bazel so here it is typed manually
cc_library(
  name = "protobuf",
  visibility = ["//visibility:public"],
  linkopts = [
    "-lprotobuf", "-labsl_log_internal_check_op", "-labsl_leak_check", "-labsl_die_if_null", "-labsl_log_internal_conditions",
    "-labsl_log_internal_message", "-labsl_examine_stack", "-labsl_log_internal_format", "-labsl_log_internal_proto",
    "-labsl_log_internal_nullguard", "-labsl_log_internal_log_sink_set", "-labsl_log_sink", "-labsl_log_entry",
    "-labsl_flags_internal", "-labsl_flags_marshalling", "-labsl_flags_reflection", "-labsl_flags_private_handle_accessor",
    "-labsl_flags_commandlineflag", "-labsl_flags_commandlineflag_internal", "-labsl_flags_config", "-labsl_flags_program_name",
    "-labsl_log_initialize", "-labsl_log_globals", "-labsl_vlog_config_internal", "-labsl_log_internal_fnmatch", "-labsl_log_internal_globals",
    "-labsl_raw_hash_set", "-labsl_hash", "-labsl_city", "-labsl_low_level_hash", "-labsl_hashtablez_sampler", "-labsl_statusor",
    "-labsl_status", "-labsl_cord", "-labsl_cordz_info", "-labsl_cord_internal", "-labsl_cordz_functions", "-labsl_exponential_biased",
    "-labsl_cordz_handle", "-labsl_crc_cord_state", "-labsl_crc32c", "-labsl_crc_internal", "-labsl_crc_cpu_detect",
    "-labsl_bad_optional_access", "-labsl_strerror", "-labsl_str_format_internal", "-labsl_synchronization", "-labsl_graphcycles_internal",
    "-labsl_kernel_timeout_internal", "-labsl_stacktrace", "-labsl_symbolize", "-labsl_debugging_internal", "-labsl_demangle_internal",
    "-labsl_demangle_rust", "-labsl_decode_rust_punycode", "-labsl_utf8_for_code_point", "-labsl_malloc_internal", "-labsl_time",
    "-labsl_civil_time", "-labsl_strings", "-labsl_strings_internal", "-labsl_string_view", "-labsl_base", "-lrt", "-labsl_spinlock_wait",
    "-labsl_int128", "-labsl_throw_delegate", "-labsl_time_zone", "-labsl_bad_variant_access", "-labsl_raw_logging_internal", "-labsl_log_severity",
  ],
)

cc_binary(
  name = "worker",
  srcs = [
    "src/main.cpp",
    "src/md_calculator.cpp",
    "src/file.cpp",
    "src/worker.cpp",
    "src/metrics_collector.cpp",
    "src/communication.pb.cc",
    "include/file.hpp",
    "include/md_calculator.hpp",
    "include/worker.hpp",
    "include/metrics_collector.hpp",
    "include/communication.pb.hh",
  ],
  deps = ["protobuf", "@spdlog//:spdlog"],
  linkopts = ["-lssl", "-lcrypto", "-lprometheus-cpp-push", "-lprometheus-cpp-core", "-lcurl"],
)
