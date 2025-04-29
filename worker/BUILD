cc_binary(
  name = "worker",
  srcs = [
    "src/main.cpp",
    "src/md_calculator.cpp",
    "src/file.cpp",
    "src/worker.cpp",
    "include/file.hpp",
    "include/md_calculator.hpp",
    "include/worker.hpp"
  ],
  linkopts = ["-lssl", "-lcrypto"],
)
