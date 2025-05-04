#pragma once

#include <string>

class File {
public:
  static std::string calculate_hash(const std::string &file_path,
                                    const std::string &algorithm);
};