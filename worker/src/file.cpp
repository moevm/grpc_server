#include <fstream>
#include <vector>
#include <stdexcept>

#include "../include/file.hpp"
#include "../include/md_calculator.hpp"

std::string File::calculate_hash(const std::string& file_path, const std::string& algorithm) {
    std::ifstream stream(file_path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("Failed to open file: " + file_path);
    }

    MDCalculator calculator(algorithm);
    std::vector<char> buffer(4096);

    while (stream) {
        stream.read(buffer.data(), buffer.size());
        calculator.update(
            reinterpret_cast<const unsigned char*>(buffer.data()),
            stream.gcount()
        );
    }

    stream.close();

    return calculator.finalize();
}
