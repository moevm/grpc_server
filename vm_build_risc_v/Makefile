IMAGE_NAME = yocto-riscv-build
VOLUME_NAME = qemu
.DEFAULT_GOAL = help

build:
	docker buildx build -t $(IMAGE_NAME) .

yocto:
	docker run --rm -it \
		--memory=12g \
		--cpus=8 \
		-v $(PWD)/$(VOLUME_NAME)/:/home/builder/$(VOLUME_NAME)/:Z \
		$(IMAGE_NAME) /home/builder/scripts/build.sh

run:
	docker run --rm -it -v $(PWD)/$(VOLUME_NAME)/:/home/builder/$(VOLUME_NAME)/:Z $(IMAGE_NAME) /home/builder/scripts/run.sh

test:
	docker run --rm -it -v $(PWD)/$(VOLUME_NAME)/:/home/builder/$(VOLUME_NAME)/:Z $(IMAGE_NAME) /bin/bash

clean:
	docker rmi $(IMAGE_NAME)
	rm -rf $(VOLUME_NAME)

help:
	@echo "Makefile for building Yocto Project"
	@echo ""
	@echo "build	- build a container image"
	@echo "yocto	- build a RISC-V image using Yocto Project"
	@echo "run	- run QEMU"
	@echo "clean	- delete Docker image and build directory, please, run with \"sudo\""
	@echo "help	- print help information"
