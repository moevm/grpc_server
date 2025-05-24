#ifndef FILE_HPP
#define FILE_HPP

#include <string>

class File {
public:
  static std::string calculate_hash(const std::string &file_path,
                                    const std::string &algorithm);
};

#endif
