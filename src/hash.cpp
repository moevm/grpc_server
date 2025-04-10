#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstring>

#include <openssl/evp.h> 

#include <thread>
#include "metrics-collector.hpp"

int calculate_hash(char* path, char* algorithm) {
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
    for (int i = 0; i < (int)md_len; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(md_value[i]);
    }
  std::cout << std::endl;

  EVP_cleanup();
  return 0;
}

int main_metrics(int argc, char* argv[]) {
  if (argc != 5) {
    std::cerr << "usage: " << argv[0] << ' ' << argv[1] 
              << " ADDRESS PORT WORKER_NAME" << std::endl;
    return 1;
  }
  
  MetricsCollector metrics_collector(argv[2], argv[3], argv[4]);

  srand(time(NULL));
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(rand() % 5 + 1));
    metrics_collector.startTask();
    
    // some "work" that requires memory
    void *data = malloc(rand() % (1024 * 1024 * 50)); // 0 - 50 megabytes
    std::this_thread::sleep_for(std::chrono::seconds(rand() % 5 + 1));
    free(data);
    
    metrics_collector.stopTask();
  }

  return 0;
}

int main_hash(int argc, char* argv[]) {
  if (argc != 4) {
    std::cerr << "usage: " << argv[0] << ' ' << argv[1] 
              << " PATH ALGORITHM" 
              << std::endl
              << "Supported algorithms are: "
              << "md2, md5, sha, " 
              << "sha1, sha224, sha256, sha384, sha512, "
              << "mdc2 and ripemd160" 
              << std::endl;
    return 1;
  }

  try {
    calculate_hash(argv[2], argv[3]);
  } catch(const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }
  return 0;
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "usage: " << argv[0] << " hash/metrics" << std::endl;
    return 1;
  }

  if (strcmp(argv[1], "hash") == 0)
    return main_hash(argc, argv);
  if (strcmp(argv[1], "metrics") == 0)
    return main_metrics(argc, argv);
  
  std::cerr << "specify hash/metrics" << std::endl;
  return 1;
}
