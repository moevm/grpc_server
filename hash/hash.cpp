#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstring>

#include <openssl/evp.h> 

int calculate_hash(char* path, char* algorithm, char* result) {
  std::ifstream stream(path, std::ios::in | std::ios::binary);
  
  if (!stream.good()) {
    throw std::runtime_error(std::strerror(errno));
  }

  EVP_MD_CTX *ctx;
  const EVP_MD *md;
  unsigned char md_value[EVP_MAX_MD_SIZE];
  char buffer[4096];
  unsigned int md_len;

  OpenSSL_add_all_digests();

  md = EVP_get_digestbyname(algorithm);
  if(!md) {
       std::cerr << "Unknown message digest" << std::endl;
       return 1;
  }

  ctx = EVP_MD_CTX_create();
  EVP_DigestInit_ex(ctx, md, nullptr);
  while (stream.good()) {
    stream.read(buffer, 4096);
    EVP_DigestUpdate(ctx, buffer, stream.gcount());
  }
  EVP_DigestFinal_ex(ctx, md_value, &md_len);
  EVP_MD_CTX_destroy(ctx);
  stream.close();

  std::cout << "Digest is: ";
    for (int i = 0; i < md_len; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(md_value[i]);
    }
  std::cout << std::endl;

  EVP_cleanup();
  return 0;
}

int main(int argc, char* argv[]) {

  if (argc < 3) {
    std::cerr << "usage: " << argv[0] 
              << " PATH ALGORITHM" 
              << std::endl
              << "Supported algorithms are: "
              << "md2, md5, sha, " 
              << "sha1, sha224, sha256, sha384, sha512, "
              << "mdc2 and ripemd160" 
              << std::endl;
    return 1;
  }

  char* result;
  try {
    calculate_hash(argv[1], argv[2], result);
  } catch(const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }
  return 0;
}
