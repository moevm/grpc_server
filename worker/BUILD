cc_binary(
  name = "worker",
  srcs = [
    "src/main.cpp",
    "src/md_calculator.cpp",
    "src/file.cpp",
    "include/file.hpp",
    "include/md_calculator.hpp"
  ],
  linkopts = ["-lssl", "-lcrypto"],
)
