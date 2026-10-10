![CANtrol Center](https://raw.githubusercontent.com/78tacos/CANServer/main/.assets/CANtrol_Center.png)

The matching UI lives in [DBCFileViewer](https://github.com/dnasso/DBCFileViewer). This repository is the scheduler that UI talks to.

# CAN Bus Scheduler

A multi-threaded TCP server and a small client for scheduling recurring or one-shot `cansend` frames on Linux CAN and vCAN interfaces.

A timer thread holds delayed work and hands it to a priority pool (higher priority first, FIFO when priorities match). `RESTART` shuts the process down and `execv`s the same binary. Logs go to `SERVER_LOG_PATH` if that is set, otherwise `LOG_PATH` from the config, otherwise `server.log` in the working directory. The server creates parent directories for `LOG_PATH` and falls back to stderr if it cannot write the file.

## Features

- Priority scheduling for recurring (`CANSEND`) and one-shot (`SEND_TASK`) jobs
- Pause, resume, kill, and list tasks per client connection
- Discovery of CAN and vCAN interfaces
- Logging with a runtime log level

## Layout

- `server.cpp` — listener, commands, scheduler, and CAN discovery
- `client.cpp` — interactive client
- `tests/test_server.cpp` — parser unit tests
- `tests/test_integration.cpp` — talks to a server already listening on `127.0.0.1:50123`
- `example_server.conf`, `example_client.conf` — config templates
- `Makefile` — server, client, and tests

## Prerequisites

Linux with POSIX sockets and pthreads, a C++20 `g++`, and `iproute2` plus can-utils (`cansend`, `candump`). A vCAN interface is enough for local testing:

```bash
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
```

## Build

```bash
make all      # server, client, and both test binaries under output/
make client   # output/client
make test     # build and run the parser tests
make clean
```

## Configuration

Copy the examples and keep the client port equal to the server `PORT`. `example_client.conf` is checked in with `SERVER_PORT=50126`; the integration test expects `50123`.

### Server

```
PORT=50123
LOG_LEVEL=WARNING          # DEBUG|INFO|WARNING|ERROR|NOLOG
WORKER_THREADS=2           # optional, at least 1
LOG_PATH=server.log        # optional; SERVER_LOG_PATH overrides this
USE_RAW_CAN=false          # true tries a raw CAN socket first
USE_PERSISTENT_SHELL=false # true keeps one shell for cansend
```

`USE_RAW_CAN` stays off unless the config turns it on. When it is on, the sender tries methods in this order:

1. Raw CAN socket
2. Persistent `cansend` shell, if `USE_PERSISTENT_SHELL=true`
3. `vfork`/`exec` of `cansend` for each frame

Rough local rates, for choosing a mode: raw sockets around 850 frames/s, `vfork`/`exec` around 550, persistent shell around 425. The persistent shell is the awkward one to debug. Leave both flags false unless you need the speed.

### Client

```
SERVER_IP=127.0.0.1
SERVER_PORT=50123
```

## Running

```bash
./output/server example_server.conf
./output/client example_client.conf
```

On a flaky SSH session, `tmux new -s canserver` keeps the server alive after you detach with Ctrl-b then d. `tmux attach -t canserver` brings it back.

## Commands

- `CANSEND#<id>#<payload>#<interval_ms>#<bus>[#priority]` — repeat
- `SEND_TASK#<id>#<payload>#<delay_ms>#<bus>[#priority]` — once
- `LIST_TASKS`, `PAUSE <task_id>`, `RESUME <task_id>`, `KILL_TASK <task_id>`, `KILL_ALL_TASKS`
- `LIST_CAN_INTERFACES` — rediscovers interfaces, then lists them
- `SET_LOG_LEVEL <level>`, `LIST_THREADS`, `KILL_THREAD <id>`, `SHUTDOWN`, `RESTART`, `DISCONNECT`, `HELP`
- `notice me senpai` — easter egg

Priority defaults to 5 and accepts `0`–`9` (higher runs first when deadlines tie). Times accept an optional `ms` suffix, and ids accept an optional `0x` prefix. `LIST_TASKS` includes the error string when a task has failed.

## Tests

`make test` runs the parser tests. They do not need gtest installed as a separate project; they do need `libgtest`. `make integration` expects a server on `127.0.0.1:50123`. `candump -tz vcan0` shows frames that actually went out.

## Troubleshooting

- No interfaces: bring up `vcan0` or the physical CAN device.
- `cansend` missing: install `can-utils` and check `PATH`.
- Fork failures: `ulimit -u`.
- Permission denied on a raw socket: run as root, or `sudo setcap cap_net_raw,cap_sys_nice+ep ./output/server`.

## License

MIT. See [LICENSE](./LICENSE).
