# AI Coding Assistant Instructions for CAN Bus Scheduler

## Architecture Overview

This is a client-server system for scheduling CAN bus messages over TCP sockets. The server manages a thread pool with deadline-based priority queuing to execute recurring `cansend` commands.

**Key Components:**
- `server.cpp`: Multi-threaded TCP server handling client connections and CAN message scheduling
- `client.cpp`: Interactive TCP client for sending commands to the server
- `ThreadPool`: Custom priority queue implementation with deadline scheduling
- `ThreadRegistry`: Tracks active worker threads for monitoring

## Build & Run Workflow

```bash
```markdown
# AI assistant notes — CAN bus scheduler (server/client)

Purpose: help an AI coding agent get productive quickly in this repo by describing the architecture, concrete run/build commands, protocol examples, and where to change behavior.

## Big picture
- Single-process, multi-threaded TCP server (`server.cpp`) that accepts simple text commands from a client (`client.cpp`).
- The server schedules recurring CAN transmissions by enqueuing tasks into an in-process ThreadPool. Each task forks+execs the external `cansend` utility to perform the actual CAN transmit.

## Key files / where to look
- `server.cpp` — entire server: config parsing, listener, command dispatch, ThreadPool, ThreadRegistry, CAN discovery, task lifecycle and cansend invocation.
- `client.cpp` — minimal interactive client used to send text commands.
- `Makefile` — build targets: `make all`, `make client`, `make test` (produces executables under `output/`).
- `output/server.conf`, `output/client.conf` — example config files used by CI/manual runs.

## Build & run (concrete)
- Build: `make all` (server binary -> `output/server`).
- Build client: `make client` (client -> `output/client`).
- Run server (from repo root): `./output/server output/server.conf` (or cd into `output/` and run `./server server.conf`).
- Run client (in another terminal): `./output/client output/client.conf`.

## Configuration (discoverable in `server.cpp`)
- Format: key=value lines. Supported keys used by the server:
  - `PORT=<port>` (required)
  - `LOG_LEVEL=<DEBUG|INFO|WARNING|ERROR>`
  - `WORKER_THREADS=<n>` (optional, defaults to number of hardware threads, min 1)

## Client protocol — exact format implemented
- The server checks for messages that start with `CANSEND#` and splits on `#`.
- Required parts: `CANSEND#<id>#<payload>#<time_ms>#<bus>`; optional 5th part is `priority` (0-9). Example:
  - `CANSEND#123#deadbeef#1000#vcan0` (default priority 5)
  - `CANSEND#0x123#deadbeef#1000ms#vcan0#7` (hex id with `0x`, `ms` suffix allowed, explicit priority)
- Other implemented text commands (match prefix): `SHUTDOWN`, `KILL_ALL`, `LIST_THREADS`, `RESTART`, `KILL_THREAD <id>`, `SET_LOG_LEVEL <level>`, `PAUSE <task_id>`, `RESUME <task_id>`, `LIST_TASKS`, `KILL_TASK <task_id>`, `KILL_ALL_TASKS`, `LIST_CAN_INTERFACES`.

## Scheduling internals (ThreadPool / tasks)
- ThreadPool lives in `server.cpp`. Important behavior:
  - Deadlines use `std::chrono::steady_clock` (enqueue_deadline).
  - Priority: integer where higher = run earlier (code expects 0–9; default 5 used in client handler).
  - FIFO tie-break via an atomic `seq` counter.
  - Worker thread count is clamped between 1 and hardware concurrency; can be overridden with `WORKER_THREADS`.
- Tasks are created per-client, assigned IDs like `task_<n>` (n is an atomic counter per client). The recurring task reschedules itself by enqueueing another deadline task after each run.

## CAN discovery & requirements
- The server discovers CAN interfaces by scanning `/sys/class/net` and falling back to `ip link` parsing. `LIST_CAN_INTERFACES` refreshes discovery before listing.
- External dependency: `cansend` must be available in PATH (the server calls `execl("/bin/sh","sh","-c", cmd, NULL)` where cmd is `cansend <bus> <id>#<data>`).
- For local testing use `vcan` (virtual CAN):
  - `sudo modprobe vcan`
  - `sudo ip link add dev vcan0 type vcan`
  - `sudo ip link set up vcan0`

## Error modes & logging
- Logging: `logEvent()` appends to `server.log` in the current working directory. Log level numeric mapping is in `server.cpp` (DEBUG=5, INFO=10, WARNING=20, ERROR=30).
- Task errors: when a cansend child exits non‑zero or by signal, the task sets itself inactive and records an error string in a global map `globalTaskErrors` (used by `LIST_TASKS`).

## Where to make typical changes
- Add a new client command: modify the `commandMap` inside the client handler thread in `server.cpp` (search for `commandMap[...] =`).
- Change scheduling criteria: edit the `Task` struct and `Cmp` comparator in the ThreadPool implementation inside `server.cpp`.
- Change how CAN messages are invoked: update the `setupRecurringCansend` lambda where it builds and `execl`s the `cansend` command.

## Quick contract & edge-cases to be aware of
- Inputs: config file path (argv[1]); text commands via TCP.
- Outputs: `server.log`, `send()` replies to clients, `cansend` side-effects on the host CAN bus.
- Edge cases found in code: missing/invalid PORT in config exits; empty CAN interface list logs a WARNING; fork failures or cansend failures stop the individual task and record an error.

---
If anything here is unclear or you want additional examples (unit tests, a short integration script that schedules a message on vcan0, or a small refactor to move ThreadPool into its own file), tell me which area to expand and I'll update this file.
```