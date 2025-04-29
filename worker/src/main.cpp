#include "../include/file.hpp"
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstring>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <sched.h>

std::vector<float> getCoreLoads() {
    std::vector<float> loads;
    std::ifstream stat("/proc/stat");

    if (!stat.is_open()) {
        std::cerr << "Failed to open /proc/stat" << std::endl;
        return loads;
    }

    // Pass 1st string (with total cpu data)
    std::string line;
    std::getline(stat, line);

    // Read each cpu data
    std::vector<std::vector<unsigned long long>> prevCpuData;
    while (std::getline(stat, line)) {
        if (line.find("cpu") == 0) {
            std::istringstream stream(line);
            std::string cpuString;
            stream >> cpuString;
            if (cpuString.substr(0, 3) != "cpu") break;

            std::vector<unsigned long long> coreData;
            unsigned long long value;
            while (stream >> value) {
                coreData.push_back(value);
            }
            prevCpuData.push_back(coreData);
        }
    }
    stat.close();

    // Wait for some time
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    stat.open("/proc/stat");
    std::getline(stat, line); // Pass 1st string

    std::vector<std::vector<unsigned long long>> currCpuData;
    for (size_t i = 0; i < prevCpuData.size(); ++i) {
        std::getline(stat, line);
        std::istringstream stream(line);
        std::string cpuString;
        stream >> cpuString;

        std::vector<unsigned long long> coreData;
        unsigned long long value;
        while (stream >> value) {
            coreData.push_back(value);
        }
        currCpuData.push_back(coreData);
    }
    stat.close();

    // Calculate load for all cpus
    for (size_t i = 0; i < prevCpuData.size(); ++i) {
        unsigned long long prevIdle = prevCpuData[i][3] + prevCpuData[i][4];
        unsigned long long currIdle = currCpuData[i][3] + currCpuData[i][4];

        unsigned long long prevTotal = 0;

        for(int j = 0; j < prevCpuData[i].size(); j++) {
            prevTotal += prevCpuData[i][j];
        }

        unsigned long long currTotal = 0;

        for(int j = 0; j < currCpuData[i].size(); j++) {
            currTotal += currCpuData[i][j];
        }

        unsigned long long totalDiff = currTotal - prevTotal;
        unsigned long long idleDiff = currIdle - prevIdle;

        float usage = 100.0f * (totalDiff - idleDiff) / totalDiff;
        loads.push_back(usage);
    }

    return loads;
}

bool bindProcessToCore(pid_t pid, int core) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core, &cpuset);

    if (sched_setaffinity(pid, sizeof(cpu_set_t), &cpuset) != 0) {
        std::cerr << "Failed to bind process to cpu: " << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

void print_usage(const std::string &program_name) {
  std::cerr << "Usage: " << program_name << " PATH ALGORITHM" << std::endl
            << "Supported algorithms are: " << "md2, md5, sha, "
            << "sha1, sha224, sha256, sha384, sha512, " << "mdc2 and ripemd160"
            << std::endl;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    print_usage(argv[0]);
    return 1;
  }

  std::vector<float> loads = getCoreLoads();
  if (loads.empty()) {
      std::cerr << "Failed to get cpu load." << std::endl;
      return 2;
  }

  int leastLoadedCPU = 0;
  for (int i = 1; i < loads.size(); i++) {
      if (loads[i] < loads[leastLoadedCPU]) leastLoadedCPU = i;
  }

  // Bind process to cpu
  if (!bindProcessToCore(0, leastLoadedCPU)) {
      return 3;
  }

  try {
    std::string hash = File::calculate_hash(argv[1], argv[2]);
    std::cout << "Digest is: " << hash << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 4;
  }

  return 0;
}