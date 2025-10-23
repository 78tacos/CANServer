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
# VS Code DevContainer — quick build & run

This DevContainer provides a ready environment for developing the CANServer project (native x86_64 build, ARM64 cross-compiles, CAN utilities, and debug tools).

Below are concise, copy-pasteable build/run and test instructions so you can get the project compiled and the integration test running locally.

## Quick build (recommended)

Build native artifacts (server, client, unit tests, integration test):

```bash
make all
```

Notes:
- The Makefile auto-detects ARM cross-compilers (prefers `g++-13-aarch64-linux-gnu` then `aarch64-linux-gnu-g++`). You can override with `CXX_ARM=` if needed.

Build ARM artifacts (cross-compile):

```bash
make arm        # dynamic ARM binaries
make arm-static # static ARM binaries (requires cross static libs)
make arm-all    # both
```

## Create a virtual CAN (vcan) device for local testing

```bash
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
ip link show vcan0
```

## Start the server and run the integration test

1. Ensure the server config uses the same port the test expects (default test uses port 50123). Create or edit `output/server.conf` (example below):

```properties
PORT=50123
LOG_LEVEL=DEBUG
WORKER_THREADS=2
LOG_PATH=server.log
```

2. Start the server (background):

```bash
# start server and detach
nohup ./output/server output/server.conf > server.stdout 2>&1 &
```

3. Run the integration test (requires server to be running on the configured port):

```bash
make integration
```

If the integration test fails to connect, check `server.stdout` and `server.log` for errors and confirm the port in `output/server.conf`.

## Run unit tests

```bash
make test
```

## Useful commands

- Build a single target:

```bash
make server        # build native server
make client        # build native client
make test_build    # build native unit test
```

- Run ARM tests under qemu-user (if you cross-compile the ARM tests):

```bash
make test_arm
make integration_arm
```

## Troubleshooting pointers

- If the integration test cannot connect (assertion at connection/send):
	- Confirm the server is running and bound to the port in `output/server.conf`.
	- Ensure `vcan0` exists when tests send CAN frames.
	- Check `server.log` and `server.stdout` for startup errors.

- If cross-compiler is not found: either install the distribution package (e.g., `g++-13-aarch64-linux-gnu`) in the devcontainer or set `CXX_ARM=/path/to/compiler` when running `make`.

## Devcontainer notes

- The devcontainer includes CAN utilities (`cansend`, `candump`) and capabilities (`CAP_NET_RAW`, `CAP_NET_ADMIN`) useful for local integration testing.
- If you run into permission issues with raw sockets, running `sudo setcap 'cap_net_raw,cap_sys_nice+ep' ./output/server` can help for manual runs.

---

If you'd like, I can also add a small shell script `scripts/run-integration.sh` that performs the vcan setup, starts the server, waits for it to become reachable, runs the integration test, and then tears down vcan0 — would you like that? 
```bash
