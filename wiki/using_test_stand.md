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

standard assembly:
```bash
cd worker
bazel build --config=riscv64 //:worker
```

build with debug output:
```bash
cd worker
bazel build --config=riscv64 //:worker --copt="-DDEBUG=1"
```

If you experience a hang during assembly, limit the memory with a flag, for example:
```bash
cd worker
bazel build --config=riscv64 //:worker --copt="-DDEBUG=1" --local_ram_resources=4096
```

The binary is placed at `worker/bazel-bin/worker`.

## 3. Start Test Stand

```bash
cd test/virtual_load_network/scripts
sudo ./start.sh
```

This will:
- Start traffic generator containers
- Create network bridges and TAP interfaces
- Launch 3 QEMU VMs: `filter1`, `filter2`, and `controller`
- Copy binaries and configs to `shared/` directory

Expected output:
```
 => [traffic-gen-1 internal] load build definition from Dockerfile.traffic-gen                                                                               0.0s
 => => transferring dockerfile: 217B                                                                                                                         0.0s
 => [traffic-gen-2 internal] load build definition from Dockerfile.traffic-gen                                                                               0.0s
 => => transferring dockerfile: 217B                                                                                                                         0.0s
 => [traffic-gen-1 internal] load metadata for docker.io/library/alpine:3.20                                                                                 0.7s
 => [traffic-gen-2 internal] load .dockerignore                                                                                                              0.0s
 => => transferring context: 2B                                                                                                                              0.0s
 => [traffic-gen-1 internal] load .dockerignore                                                                                                              0.0s
 => => transferring context: 2B                                                                                                                              0.0s
 => [traffic-gen-2 1/4] FROM docker.io/library/alpine:3.20@sha256:d9e853e87e55526f6b2917df91a2115c36dd7c696a35be12163d44e6e2a4b6bc                           0.0s
 => => resolve docker.io/library/alpine:3.20@sha256:d9e853e87e55526f6b2917df91a2115c36dd7c696a35be12163d44e6e2a4b6bc                                         0.0s
 => [traffic-gen-2 internal] load build context                                                                                                              0.0s
 => => transferring context: 71B                                                                                                                             0.0s
 => [traffic-gen-1 internal] load build context                                                                                                              0.0s
 => => transferring context: 71B                                                                                                                             0.0s
 => CACHED [traffic-gen-1 2/4] RUN apk add --no-cache curl bash iproute2                                                                                     0.0s
 => CACHED [traffic-gen-1 3/4] COPY ./scripts/traffic-gen.sh /entrypoint.sh                                                                                  0.0s
 => CACHED [traffic-gen-1 4/4] RUN chmod +x /entrypoint.sh                                                                                                   0.0s
 => [traffic-gen-2] exporting to image                                                                                                                       0.0s
 => => exporting layers                                                                                                                                      0.0s
 => => exporting manifest sha256:2a5b7514893477d98546e00304bab0c5d3df5e691685413fdcd4c5d7f8a44b8e                                                            0.0s
 => => exporting config sha256:0820b0d32a4e01e75aa6f7fb86776dcb475af2f7513593360acbecaf13110a4c                                                              0.0s
 => => exporting attestation manifest sha256:47fe7dece22aee82abbc0e0fbad91b19cf38bea269f9c2a480c8d063a4b4391d                                                0.0s
 => => exporting manifest list sha256:26a7229340f15481c403acf5125032cb86a70fcd48358256764d944d2a97a3c9                                                       0.0s
 => => naming to docker.io/library/virtual_load_network-traffic-gen-2:latest                                                                                 0.0s
 => => unpacking to docker.io/library/virtual_load_network-traffic-gen-2:latest                                                                              0.0s
 => [traffic-gen-1] exporting to image                                                                                                                       0.0s
 => => exporting layers                                                                                                                                      0.0s
 => => exporting manifest sha256:ba6645395e7cabb7c7babca4f5ae9e76586e524ee00b385e28b34ef6741d9964                                                            0.0s
 => => exporting config sha256:ab641cf5526440f2ce75212d98a7ce2c485b1a51d4215adfe0fc720d836c5774                                                              0.0s
 => => exporting attestation manifest sha256:4fd13be17b47ec34c6b497e30cbf496c9ebd542db2d7d1d03bee83e9dc8a4148                                                0.0s
 => => exporting manifest list sha256:3f4c8d5955f564bcbf2b33634da0edab10ca65e99e21042d2ed671601e2078e1                                                       0.0s
 => => naming to docker.io/library/virtual_load_network-traffic-gen-1:latest                                                                                 0.0s
 => => unpacking to docker.io/library/virtual_load_network-traffic-gen-1:latest                                                                              0.0s
 => [traffic-gen-1] resolving provenance for metadata file                                                                                                   0.0s
 => [traffic-gen-2] resolving provenance for metadata file                                                                                                   0.0s
[+] Running 12/12
 ✔ traffic-gen-1                                   Built                                                                                                     0.0s 
 ✔ traffic-gen-2                                   Built                                                                                                     0.0s 
 ✔ Network virtual_load_network_testnet2           Created                                                                                                   0.0s 
 ✔ Network virtual_load_network_default            Created                                                                                                   0.0s 
 ✔ Network virtual_load_network_testnet1           Created                                                                                                   0.0s 
 ✔ Container virtual_load_network-traffic-gen-2-2  Started                                                                                                   0.3s 
 ✔ Container virtual_load_network-pushgateway-1    Started                                                                                                   0.3s 
 ✔ Container virtual_load_network-traffic-gen-2-1  Started                                                                                                   0.4s 
 ✔ Container virtual_load_network-traffic-gen-1-2  Started                                                                                                   0.3s 
 ✔ Container virtual_load_network-traffic-gen-1-1  Started                                                                                                   0.4s 
 ✔ Container virtual_load_network-prometheus-1     Started                                                                                                   0.3s 
 ✔ Container virtual_load_network-grafana-1        Started                                                                                                   0.4s 
Nothing to flush
Nothing to flush
Nothing to flush
Nothing to flush
Nothing to flush
Nothing to flush
Test stand is running
  Filter VMs: filter1 (pid 176929), filter2 (pid 177022)
  Controller: pid 178189
  Traffic generators: 7 containers
```

### 1. If you need to connect via SSH

Connect to the controller VM via SSH:
```bash
ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null root@10.0.2.3
```

Connect to the filter1 VM via SSH:
```bash
ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null root@10.0.2.1
```

Connect to the filter2 VM via SSH:
```bash
ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null root@10.0.2.2
```


### 2. Load Filtering Policy

Detailed instructions for administering the policy are provided in grpc_server/admin/README.md

## Installing dependencies
```bash
pip install -r requirements.txt
```

## Generating proto files
```bash
python -m grpc_tools.protoc \
    --python_out=. \
    --grpc_python_out=. \
    -I . \
    admin_service.proto
```

## Example of a toml file with policy
```toml
[global.rules]                                   # Глобальные правила для всех воркеров
block_categories = ["Gambling", "Weapons"]       # Категории для блокировки
block_domains = ["youtube.com", "tiktok.com"]    # Домены для блокировки
allow_domains = ["github.com", "stackoverflow.com"] # Разрешённые домены (приоритет над блокировкой)
block_ips = ["192.168.0.1", "2001:0db8:85a3:0000:0000:8a2e:0370:7334"]      # IP для блокировки (IPv4 и IPv6)
allow_ips = ["8.8.8.8"]                           # Разрешённые IP 
ttl_ip = 604800                                   # TTL кэша IP 
ttl_domain = 604800                               # TTL кэша доменов 
min_trust_level = 5                               # Мин. уровень доверия 

[global.rules.block_by_trust]                     # Блокировка по уровню доверия
ENTERTAINMENT = 6                                 
NEWS = 4                                          

[filters.filter_1]                                # Правила для воркера #1
block_categories = ["Weapons", "Malware"]         # Доп. категории к глобальным
block_domains = ["instagram.com"]                 # Доп. домены к глобальным
allow_domains = ["vk.com"]                        # Доп. разрешённые домены
min_trust_level = 0                               # Переопределяет глобальный 

[filters.filter_1.block_by_trust]                 # Уровни доверия для воркера #1
SOCIAL = 8                                       
ENTERTAINMENT = 7                                

[filters.filter_2]                                # Правила для воркера #2
block_categories = ["Malware"]                    # Доп. категории к глобальным
allow_domains = ["github.com", "gitlab.com"]      # Доп. разрешённые домены

```

### Send a new policy to the controller

```bash
python admin.py load --file <your_policy>.toml
```

### Get the current policy from the controller

```bash
python admin.py get --file <your_policy>.toml
```

### Enable/disable filtering on the worker

```bash
python admin.py toogle --id 1 --on  #id - worker id
python admin.py toogle --id 1 --off #id - worker id
```


## 5. Monitoring

### Conroller

Log monitoring
```bash
cd test/virtual_load_network/shared/logs
sudo tail -f controller.log
```

### Worker 1

Log monitoring
```bash
cd test/virtual_load_network/shared/logs
sudo tail -f worker1.log
```

Monitoring the port on which packets are received
```bash
sudo tcpdump -i tap-f1-in port 53
```

Monitoring the port from which packets are emitted
```bash
sudo tcpdump -i tap-f1-out port 53
```

### Worker 2

Log monitoring
```bash
cd test/virtual_load_network/shared/logs
sudo tail -f worker2.log
```

Monitoring the port on which packets are received
```bash
sudo tcpdump -i tap-f2-in port 53
```

Monitoring the port from which packets are emitted
```bash
sudo tcpdump -i tap-f2-out port 53
```


## 4. Stop Test Stand

```bash
cd test/virtual_load_network/scripts
sudo ./stop.sh
```

