SCRIPTS_DIR = scripts
WORKER_DIR = ../../worker
CONTROLLER_DIR = ../../controller
DPDK_RISCV_INSTALL = $(WORKER_DIR)/dpdk-riscv-install

.PHONY: run stop logs build build-filter build-controller clean-vms

run: build
	sudo $(SCRIPTS_DIR)/start.sh

stop:
	sudo $(SCRIPTS_DIR)/stop.sh

logs:
	@sudo bash -c 'while true; do clear; cat shared/filter1.log 2>/dev/null | tail -10; echo "---"; cat shared/filter2.log 2>/dev/null | tail -10; echo "---"; cat shared/controller.log 2>/dev/null | tail -5; sleep 2; done'

build: build-filter build-controller

build-filter:
	@if [ ! -d "$(DPDK_RISCV_INSTALL)" ]; then \
		echo "DPDK sysroot not found. Run ./create_image.sh first."; \
		exit 1; \
	fi
	$(MAKE) -C $(WORKER_DIR) -f Makefile.main_riscv

build-controller:
	$(MAKE) -C $(CONTROLLER_DIR) build-riscv

clean-vms:
	rm -f filter1.qcow2 filter2.qcow2 controller.qcow2 cloud-init-*.iso
