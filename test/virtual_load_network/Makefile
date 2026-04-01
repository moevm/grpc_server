DOCKER_IMAGE=traffic-gen
DOCKER_NAME=test-gen

.PHONY: build
build:
	docker build -t $(DOCKER_IMAGE) .

.PHONY: run
run:
	sudo ./teardown.sh && sudo ./setup.sh

.PHONY: clean
clean:
	sudo ./teardown.sh

