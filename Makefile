.PHONY: all clean

all: build/grpc_server

build:
	mkdir build

build/grpc_server: build src/hash.cpp src/metrics-collector.cpp src/metrics-collector.hpp 
	g++ -Wall -Werror -o build/grpc_server src/hash.cpp src/metrics-collector.cpp -lssl -lcrypto \
		$(shell pkg-config --libs prometheus-cpp-core prometheus-cpp-push)

clean:
	rm -rf build
