SCRIPTS_DIR = scripts
WORKER_DIR = ../../worker
CONTROLLER_DIR = ../../controller

.PHONY: run stop logs build build-worker build-controller clean-vms

run: build
	sudo $(SCRIPTS_DIR)/start.sh

stop:
	sudo $(SCRIPTS_DIR)/stop.sh

logs:
	@sudo bash -c 'while true; do clear; cat shared/filter1.log 2>/dev/null | tail -10; echo "---"; cat shared/filter2.log 2>/dev/null | tail -10; echo "---"; cat shared/controller.log 2>/dev/null | tail -5; sleep 2; done'

build: build-worker build-controller

build-worker:
	cd $(WORKER_DIR) && bazel build --config=riscv64 //:worker

build-controller:
	$(MAKE) -C $(CONTROLLER_DIR) build-riscv

clean-vms:
	rm -f filter1.qcow2 filter2.qcow2 controller.qcow2
