# Dev container

The container has a native g++ with C++20, an ARM64 cross compiler, can-utils, iproute2, gdb, and strace. It runs with `CAP_NET_RAW`.

Open this folder in VS Code and choose **Reopen in Container** (`Dev Containers: Reopen in Container`). The first build takes a while.

## Build

```bash
make all          # server, client, unit test, integration test
make arm          # dynamic ARM64 binaries
make arm-static   # static ARM64 binaries (needs the cross static libs)
make arm-all
```

The Makefile looks for `g++-13-aarch64-linux-gnu`, then `aarch64-linux-gnu-g++`. Override that with `CXX_ARM=` if you have the compiler somewhere else.

Single targets: `make server`, `make client`, `make test_build`.

## vCAN and the integration test

```bash
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
```

The integration test connects to port 50123. Point `output/server.conf` (or `example_server.conf`) at that port, start the server, then run the test:

```bash
nohup ./output/server example_server.conf > server.stdout 2>&1 &
make integration
make test
```

If the test cannot connect, check `server.stdout` and `server.log`, and confirm the port. Cross-compiled tests can be run with `make test_arm` and `make integration_arm` under qemu-user.

Raw sockets need `CAP_NET_RAW`. If a manual run outside the container is denied, `sudo setcap 'cap_net_raw,cap_sys_nice+ep' ./output/server` is the usual fix.
