#pragma once

#include <memory>
#include <openssl/evp.h>
#include <string>

class MDCalculator {
public:
  MDCalculator(const std::string &algorithm);

  void update(const unsigned char *data, size_t length);
  std::string finalize();

  MDCalculator(const MDCalculator &) = delete;
  MDCalculator &operator=(const MDCalculator &) = delete;

private:
  const EVP_MD *md_;
  std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx_;
};