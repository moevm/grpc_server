#include <iostream>
#include "../include/file.hpp"

void print_usage(const std::string& program_name) {
    std::cerr << "Usage: " << program_name 
              << " PATH ALGORITHM" << std::endl
              << "Supported algorithms are: "
              << "md2, md5, sha, " 
              << "sha1, sha224, sha256, sha384, sha512, "
              << "mdc2 and ripemd160" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string(argv[1]) == "--status") {
        std::cout << "Worker status: OK" << std::endl;
        return 0;
    }

    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }
    
    try {
        std::string hash = File::calculate_hash(argv[1], argv[2]);
        std::cout << "Digest is: " << hash << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
