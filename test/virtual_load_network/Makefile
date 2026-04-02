DOCKER_IMAGE=traffic-gen

SCRIPTS_DIR=scripts
HOSTS_CREATE_SCRIPT=$(SCRIPTS_DIR)/setup.sh
HOSTS_CLEAN_SCRIPT=$(SCRIPTS_DIR)/teardown.sh

FILTER_ELF_PATH=../../worker/main-x86-virt

N ?= 3

.PHONY: build
build:
	docker build -t $(DOCKER_IMAGE) .

.PHONY: run
run:
	-sudo $(HOSTS_CLEAN_SCRIPT) $(N)
	sudo $(HOSTS_CREATE_SCRIPT) $(N)
	sudo sh -c 'LD_LIBRARY_PATH=/usr/local/lib $(FILTER_ELF_PATH) --no-pci --' | grep "port = 53;"
	

.PHONY: clean
clean:
	sudo $(HOSTS_CLEAN_SCRIPT) $(N)

