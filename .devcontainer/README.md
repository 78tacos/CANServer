# VS Code DevContainer Setup

This DevContainer provides a complete development environment for building the CANServer project, including:

- **Native x86_64 compilation** with g++ and C++20 support
- **ARM64 cross-compilation** with aarch64-linux-gnu-g++
- **CAN utilities** (can-utils, iproute2, vcan support)
- **Debugging tools** (gdb, strace)
- **Realtime capabilities** (CAP_NET_RAW, CAP_SYS_NICE, CAP_NET_ADMIN)

## Getting Started

1. Open this folder in VS Code
2. When prompted, click "Reopen in Container" (or run: `Dev Containers: Reopen in Container`)
3. Wait for the container to build (first time only)
4. Once inside, you're ready to build!

## Building

### Native x86_64 build:
```bash
make all
```

### ARM64 cross-compile:
```bash
make arm64
```

### Static binaries:
```bash
# x86_64 static
g++ -std=c++20 -Wall -pthread -static server.cpp -o output/server-static

# ARM64 static
aarch64-linux-gnu-g++ -std=c++20 -Wall -pthread -static server.cpp -o outputarm/server-arm64-static
```

## Testing with Virtual CAN

The container has the necessary capabilities to create virtual CAN interfaces:

```bash
# Load vcan kernel module (if not already loaded)
sudo modprobe vcan

# Create and bring up vcan0
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0

# Verify
ip link show vcan0
```

## Running the Server

```bash
# Start server
./output/server output/server.conf

# In another terminal, monitor CAN traffic
candump -tz vcan0

# In another terminal, use the client
./output/client output/client.conf
```

## Capabilities Included

The devcontainer runs with these Linux capabilities:
- `CAP_NET_RAW` - Required for PF_CAN raw socket access
- `CAP_SYS_NICE` - Required for SCHED_FIFO realtime scheduling
- `CAP_NET_ADMIN` - Required to create vcan interfaces

## Installed Tools

### Compilers & Build Tools
- g++ (native x86_64)
- aarch64-linux-gnu-g++ (ARM64 cross-compiler)
- make
- pkg-config

### CAN Tools
- can-utils (cansend, candump, cangen, etc.)
- iproute2 (ip command for network interfaces)

### Debugging & Analysis
- gdb (GNU debugger)
- strace (system call tracer)

### Utilities
- git
- vim/nano
- curl
- sudo

## Cross-Compilation Notes

When cross-compiling for ARM64:
- Compiler: `aarch64-linux-gnu-g++`
- Target: ARM64/AArch64 Linux
- Flags: `-std=c++20 -Wall -pthread`
- Output tested on: Raspberry Pi, AWS Graviton, other ARM64 Linux systems

## Troubleshooting

### Can't create vcan interface
```bash
# Make sure vcan module is loaded
sudo modprobe vcan
lsmod | grep vcan

# Check kernel support
zcat /proc/config.gz | grep CONFIG_CAN
```

### Permission denied on raw sockets
The container should have CAP_NET_RAW by default. If issues persist:
```bash
# Grant capability to binary
sudo setcap 'cap_net_raw,cap_sys_nice+ep' ./output/server
```

### ARM64 binary won't run
ARM64 binaries built here are for ARM64 Linux systems. To test:
- Copy to ARM64 device (Raspberry Pi, etc.)
- Use QEMU user-mode emulation
- Deploy to ARM64 cloud instance
