#!/bin/bash

set -e
wget https://github.com/jupp0r/prometheus-cpp/releases/download/v1.3.0/prometheus-cpp-with-submodules.tar.gz
tar -xvzf prometheus-cpp-with-submodules.tar.gz
cd prometheus-cpp-with-submodules

mkdir build 
cd build 

cmake .. -DBUILD_SHARED_LIBS=ON -DENABLE_COMPRESSION=OFF

cmake --build . --parallel 4

ctest -V

cmake --install .
