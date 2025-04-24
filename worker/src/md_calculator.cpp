#include <stdexcept>
#include <sstream>
#include <iomanip>

#include "../include/md_calculator.hpp"

MDCalculator::MDCalculator(const std::string& algorithm) 
    : md_(nullptr), ctx_(nullptr, EVP_MD_CTX_free) {
    
    OpenSSL_add_all_digests();
    md_ = EVP_get_digestbyname(algorithm.c_str());
    if (!md_) {
        throw std::runtime_error("Unknown message digest: " + algorithm);
    }

    ctx_.reset(EVP_MD_CTX_new());
    if (!ctx_) {
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    }

    if (EVP_DigestInit_ex(ctx_.get(), md_, nullptr) != 1) {
        throw std::runtime_error("Failed to initialize digest");
    }
}

void MDCalculator::update(const unsigned char* data, size_t length) {
    if (EVP_DigestUpdate(ctx_.get(), data, length) != 1) {
        throw std::runtime_error("Failed to update digest");
    }
}

std::string MDCalculator::finalize() {
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len;

    if (EVP_DigestFinal_ex(ctx_.get(), md_value, &md_len) != 1) {
        throw std::runtime_error("Failed to finalize digest");
    }

    std::ostringstream oss;
    for (int i = 0; i < md_len; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') 
            << static_cast<int>(md_value[i]);
    }

    return oss.str();
}
