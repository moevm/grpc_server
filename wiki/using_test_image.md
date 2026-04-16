# RISC-V Virtual Machine with DPDK for Testing

This set of scripts creates and runs a RISC-V virtual machine (via QEMU) with pre-installed DPDK, a shared folder for file exchange, and port forwarding for network testing.

## Quick Start


### 1. Create image

```bash
chmod +x create_image.sh
./create_image.sh
```

## 2. Run VM

```bash
chmod +x run_vm.sh
./run_vm.sh <shared_folder_path> [port_forwarding_rules...]
```

Parameters:
- shared_folder_path — Directory on the host that will be accessible in the guest
- port_forwarding_rules — Optional, format: PROTOCOL::HOST_PORT-:GUEST_PORT


## Example

```bash
./run_vm.sh /home/bob/mycode tcp::2222-:22
```


