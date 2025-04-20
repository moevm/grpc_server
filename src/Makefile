all: md_calculator.cpp md_calculator.hpp main.cpp file.cpp file.hpp metrics-collector.hpp metrics-collector.cpp
	g++ -o hash main.cpp md_calculator.cpp metrics-collector.cpp file.cpp -lssl -lcrypto \
		$(shell pkg-config --libs prometheus-cpp-core prometheus-cpp-push)
