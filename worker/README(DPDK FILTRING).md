# installing_dpdk.md
Setting up the environment for cross-compilation and installing dpdk is described in "installing_dpdk.md" on the wiki.



# Create a pair of veth and TAP device

```bash
sudo ./scripts/set_virt_dev_for_test_xdp.sh
```
The script creates a pair veth0 - veth1

```bash
sudo ./scripts/set_tap_dev.sh
```
The script creates a TAP device tap0


# Project assembly
For real ports (eth0/eth1 + tap0):
```bash
make -f Makefile.main_riscv all
```

For virtual ports (veth0/veth1 + tap0):
```bash
make -f Makefile.main_riscv virt
```
Defining the -DVIRT_PORTS macro switches the program to use virtual interfaces.

Before starting, it is recommended to run the virtual device configuration script:
```bash
sudo ./scripts/set_virt_dev_for_test_xdp.sh
```

For debugging add DEBUG=1, example:
```bash
make -f Makefile.main_x86 virt DEBUG=1
```

# Clean
```bash
make -f Makefile.main_riscv clean
```

# Launch
The program requires superuser rights (to work with DPDK and XDP):
```bash
sudo ./main-riscv-virt
```


# Notes
The DNS cache is automatically saved to cache.db (SQLite) and restored on restart.
Periodic saving of the cache occurs every hour using DPDK timers.
