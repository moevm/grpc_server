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
  includes = ["/usr/local/include"],
  linkopts = ["-L/usr/local/lib","-lssl", "-lcrypto", "-lprometheus-cpp-push", "-lprometheus-cpp-core", "-lcurl"],
)
