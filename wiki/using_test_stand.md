# Using Test Stand

The test stand runs QEMU RISC-V virtual machines (controller + filter workers) with traffic generators in Docker containers. It allows testing the full filtering pipeline: traffic generation, DNS interception, domain classification via Kaspersky API, and policy-based blocking.

## Prerequisites

- `qemu-system-riscv64`
- Docker and Docker Compose
- Bazel

## Network Layout

Bridges:
* br-mgmt (10.0.2.0/24) - SSH access, gRPC communication
* br-inet (10.0.3.0/24) - Internet access (controller)
* br-testnet1 (10.0.0.0/24) - Traffic to filter1
* br-testnet2 (10.0.1.0/24) - Traffic to filter2

VMs:
* filter1 (10.0.2.1) - DPDK packet filter and gRPC worker
* filter2 (10.0.2.2) - DPDK packet filter and gRPC worker
* controller (10.0.2.3) - gRPC server and Kaspersky API classifier

## 0. Yocto Image

The test stand requires a RISC-V image `cluster-image` and SDK from `vm_build_risc_v`

**Image**
```bash
cd vm_build_risc_v
make build
make qemu
```

`make build` creates the Docker container with the build environment.

`make qemu` runs the full Yocto build for `qemuriscv64` inside it.

**SDK**
```bash
make qemu-sdk
```

The SDK installer script will appear in `vm_build_risc_v/qemu/poky/build/tmp/deploy/sdk/`. Install it in `/opt/riscv-sdk`:

```bash
sudo vm_build_risc_v/qemu/poky/build/tmp/deploy/sdk/poky-glibc-x86_64-cluster-image-riscv64-qemuriscv64-toolchain-*.sh -d /opt/riscv-sdk
```

Edit `test/virtual_load_network/.env`:
```env
YOCTO_DEPLOY_DIR=/path/to/deploy/images/qemuriscv64
```


## 1. Build Controller

```bash
cd controller
make build-riscv
```

The binary is placed at `controller/bin/grpc_server`.

## 2. Build Worker

```bash
cd worker
bazel build --config=riscv64 //:worker
```

The binary is placed at `worker/bazel-bin/worker`.

## 3. Start Test Stand

```bash
cd test/virtual_load_network
make run
```

This will:
- Start traffic generator containers
- Create network bridges and TAP interfaces
- Launch 3 QEMU VMs: `filter1`, `filter2`, and `controller`
- Copy binaries and configs to `shared/` directory

Expected output:
```
➜  virtual_load_network git:(test_stand) ✗ sudo scripts/start.sh 
[sudo] password for lespend: 
[+] Building 2.9s (13/13) FINISHED                                                                                                                           
 => [internal] load local bake definitions                                                                                                              0.0s
 => => reading from stdin 1.21kB                                                                                                                        0.0s
 => [traffic-gen-1 internal] load build definition from Dockerfile.traffic-gen                                                                          0.0s
 => => transferring dockerfile: 208B                                                                                                                    0.0s
 => [traffic-gen-2 internal] load metadata for docker.io/library/alpine:3.20                                                                            1.9s
 => [traffic-gen-1 internal] load .dockerignore                                                                                                         0.0s
 => => transferring context: 2B                                                                                                                         0.0s
 => [traffic-gen-1 internal] load build context                                                                                                         0.0s
 => => transferring context: 500B                                                                                                                       0.0s
 => [traffic-gen-2 1/4] FROM docker.io/library/alpine:3.20@sha256:d9e853e87e55526f6b2917df91a2115c36dd7c696a35be12163d44e6e2a4b6bc                      0.0s
 => CACHED [traffic-gen-2 2/4] RUN apk add --no-cache curl bash                                                                                         0.0s
 => CACHED [traffic-gen-2 3/4] COPY ./scripts/traffic-gen.sh /entrypoint.sh                                                                             0.0s
 => CACHED [traffic-gen-1 4/4] RUN chmod +x /entrypoint.sh                                                                                              0.0s
 => [traffic-gen-2] exporting to image                                                                                                                  0.0s
 => => exporting layers                                                                                                                                 0.0s
 => => writing image sha256:e99e2c8bdbe3ed141026d06d3ececcadbc769171b650ac5359ff79940eb87889                                                            0.0s
 => => naming to docker.io/library/virtual_load_network-traffic-gen-2                                                                                   0.0s
 => [traffic-gen-1] exporting to image                                                                                                                  0.0s
 => => exporting layers                                                                                                                                 0.0s
 => => writing image sha256:8528a890010cfd50b8c21c85de488da8e22070021e37709b7a7c9926c16629b2                                                            0.0s
 => => naming to docker.io/library/virtual_load_network-traffic-gen-1                                                                                   0.0s
 => [traffic-gen-2] resolving provenance for metadata file                                                                                              0.0s
 => [traffic-gen-1] resolving provenance for metadata file                                                                                              0.0s
[+] up 8/8
 ✔ Image virtual_load_network-traffic-gen-2       Built                                                                                                  3.0s
 ✔ Image virtual_load_network-traffic-gen-1       Built                                                                                                  3.0s
 ✔ Network virtual_load_network_testnet1          Created                                                                                                0.1s
 ✔ Network virtual_load_network_testnet2          Created                                                                                                0.1s
 ✔ Container virtual_load_network-traffic-gen-2-2 Started                                                                                                0.7s
 ✔ Container virtual_load_network-traffic-gen-1-2 Started                                                                                                0.7s
 ✔ Container virtual_load_network-traffic-gen-2-1 Started                                                                                                0.5s
 ✔ Container virtual_load_network-traffic-gen-1-1 Started                                                                                                0.5s
10.0.0.254 dev eth0 lladdr 1e:68:2a:65:96:99 ref 1 used 0/0/0 probes 4 REACHABLE

*** Round 1, deleting 1 entries ***
*** Flush is complete after 1 round(s) ***
10.0.0.254 dev eth0 lladdr 1e:68:2a:65:96:99 ref 1 used 0/0/0 probes 4 REACHABLE

*** Round 1, deleting 1 entries ***
*** Flush is complete after 1 round(s) ***
10.0.1.254 dev eth0 lladdr 36:e3:51:79:b3:3c ref 1 used 0/0/0 probes 4 REACHABLE

*** Round 1, deleting 1 entries ***
*** Flush is complete after 1 round(s) ***
10.0.1.254 dev eth0 lladdr 36:e3:51:79:b3:3c ref 1 used 0/0/0 probes 4 REACHABLE

*** Round 1, deleting 1 entries ***
*** Flush is complete after 1 round(s) ***
Test stand is running
  Filter VMs: filter1 (pid 37833), filter2 (pid 37857)
  Controller: pid 38215
  Traffic generators: 4 containers
```
```
```

### 1. Start Controller

Connect to the controller VM via SSH:

```bash
ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null root@10.0.2.3
```

Set up internet access (required for Kaspersky API):

```bash
sh /mnt/shared/setup-inet.sh
```

Start the controller:

```bash
cd /mnt/shared
KASPERSKY_API_KEY=<your_key> ./controller
KASPERSKY_API_KEY=wyo915OXTCe5stpLCtc5Ww== ./controller
```

### 2. Load Filtering Policy

On the host, install dependencies and load a policy:

```bash
cd admin
uv venv --python 3.12 .venv
source .venv/bin/activate
uv pip install -r requirements.txt
uv pip install "setuptools<75"
python -m grpc_tools.protoc -I. --python_out=. --grpc_python_out=. admin_service.proto
```

Edit `admin/.env` to point to the controller:

```
SERVER_HOST=10.0.2.3
SERVER_PORT=50051
```

Edit `config.toml` with desired rules, then load:

```bash
python admin.py --file config.toml
```

Expected output:
```
(.venv) ➜  admin git:(test_stand) ✗ python admin.py --file config.toml
Config loaded
```
```
```

### 3. Start Worker

Connect to the filter VM via SSH:

```bash
ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null root@10.0.2.1    # filter1
ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null root@10.0.2.2    # filter2
```

Start the worker:

```bash
cd /mnt/shared
WORKER_ID=1 METRICS_GATEWAY_ADDRESS=10.0.2.254 METRICS_GATEWAY_PORT=9091 CONTROLLER_GRPC_ADDR=10.0.2.3:50051 DPDK_PORT_IN=eth0 DPDK_PORT_OUT=eth1 ./worker
```

For `filter2`, use `WORKER_ID=2`.

To redirect logs to a shared file (viewable from host):

```bash
WORKER_ID=1 METRICS_GATEWAY_ADDRESS=10.0.2.254 METRICS_GATEWAY_PORT=9091 CONTROLLER_GRPC_ADDR=10.0.2.3:50051 DPDK_PORT_IN=eth0 DPDK_PORT_OUT=eth1 ./worker > /mnt/shared/filter1.log 2>&1 &
```

Expected output:
```
root@qemuriscv64:~# cd /mnt/shared
root@qemuriscv64:/mnt/shared# WORKER_ID=1 METRICS_GATEWAY_ADDRESS=10.0.2.254 METRICS_GATEWAY_PORT=9091 CONTROLLER_GRPC_ADDR=10.0.2.3:50051 DPDK_PORT_IN=eth0 
DPDK_PORT_OUT=eth1 ./worker
[2026-04-23 16:46:28.534] [info] Initialize MetricsCollector with 10.0.2.254:9091
[2026-04-23 16:46:28.823] [info] gRPC channel created to 10.0.2.3:50051
[2026-04-23 16:46:28.826] [info] Signal handlers registered
[2026-04-23 16:46:28.828] [info] Worker 1 requests policy
[2026-04-23 16:46:29.321] [info] Policy received
EAL: Detected CPU lcores: 2
EAL: Detected NUMA nodes: 1
EAL: Detected shared linkage of DPDK
EAL: Multi-process socket /var/run/dpdk/rte/mp_socket
EAL: Selected IOVA mode 'PA'
EAL: TSC using RISC-V rdtime.
TELEMETRY: No legacy callbacks, legacy socket not created
libbpf: Attribute of type 0x2a found multiple times in message, previous attribute is being ignored.
libbpf: Attribute of type 0x2a found multiple times in message, previous attribute is being ignored.
libbpf: elf: skipping unrecognized data section(8) .xdp_run_config
libbpf: elf: skipping unrecognized data section(9) xdp_metadata
libbpf: Attribute of type 0x2a found multiple times in message, previous attribute is being ignored.
libbpf: Attribute of type 0x2a found multiple times in message, previous attribute is being ignored.
libbpf: elf: skipping unrecognized data section(7) xdp_metadata
libbpf: elf: skipping unrecognized data section(7) xdp_metadata
libxdp: No bpffs found at /sys/fs/bpf
libxdp: Can't use dispatcher without a working bpffs
libxdp: Falling back to loading single prog without dispatcher
libbpf: elf: skipping unrecognized data section(7) xdp_metadata
Port 0 initialized
libbpf: Attribute of type 0x2a found multiple times in message, previous attribute is being ignored.
libbpf: Attribute of type 0x2a found multiple times in message, previous attribute is being ignored.
libbpf: elf: skipping unrecognized data section(8) .xdp_run_config
libbpf: elf: skipping unrecognized data section(9) xdp_metadata
libbpf: Attribute of type 0x2a found multiple times in message, previous attribute is being ignored.
libbpf: Attribute of type 0x2a found multiple times in message, previous attribute is being ignored.
libbpf: elf: skipping unrecognized data section(7) xdp_metadata
libbpf: elf: skipping unrecognized data section(7) xdp_metadata
libxdp: No bpffs found at /sys/fs/bpf
libxdp: Can't use dispatcher without a working bpffs
libxdp: Falling back to loading single prog without dispatcher
libbpf: elf: skipping unrecognized data section(7) xdp_metadata
Port 1 initialized
Port 2 initialized
Port 0 started
Port 1 started
Port 2 started
[INFO] Loaded 0 records from SQLite, 0 expired skipped
[2026-04-23 16:46:30.684] [info] DPDK initialized: in_port=0, out_port=1
[2026-04-23 16:46:31.204] [info] Worker 1 classifying domain 'betboom.ru'
[2026-04-23 16:46:33.307] [info] Domain 'betboom.ru' classified as category 'Gambling' with trust level 0
This site has a locked category[INFO] Packet without dns request
```
```
```


## 5. Monitoring
```sh
sudo tcpdump -i tap-f1-out port 53 # output packets
sudo tcpdump -i tap-f1-in port 53 # input packets
```

## 4. Stop Test Stand

```bash
cd test/virtual_load_network
make stop
```

