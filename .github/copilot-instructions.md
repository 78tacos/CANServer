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
# Build server (only target in Makefile)
make all

# Build client separately
make client

# Run server with config
./server output/server.conf

# Run client with config (in another terminal)
./client output/client.conf
```

**Configuration Files:**
- `server.conf`: `PORT=50123`, `LOG_LEVEL=DEBUG`
- `client.conf`: `SERVER_IP=127.0.0.1`, `SERVER_PORT=50123`

## Command Protocol

Client sends text commands to server. Key commands:

- `CANSEND <interface> <id#data> <time_ms> [priority]`: Schedule recurring CAN message
  - Example: `CANSEND vcan0 123#deadbeef 1000 5`
- `LIST_THREADS`: Show active server threads
- `LIST_TASKS`: Show scheduled CAN message tasks
- `PAUSE <task_id>` / `RESUME <task_id>`: Control task execution
- `KILL_TASK <task_id>` / `KILL_ALL_TASKS`: Stop tasks
- `SHUTDOWN`: Close client connection

## ThreadPool Implementation

Custom ThreadPool uses priority queue with:
- **Deadlines**: `std::chrono::high_resolution_clock::time_point`
- **Priorities**: Higher numbers = higher priority (0-9)
- **FIFO ordering**: Within same deadline/priority
- **Drop-if-missed**: Tasks can be discarded if deadline passed

Example usage:
```cpp
ThreadPool pool;
pool.enqueue_deadline(deadline, priority, drop_if_missed, []() {
    system("cansend vcan0 123#deadbeef");
});
```

## Logging & Debugging

- Logs to `server.log` with timestamps and severity levels
- Log levels: DEBUG(5), INFO(10), WARNING(20), ERROR(30)
- Set via `LOG_LEVEL=` in server.conf
- Thread safety: All logging operations are thread-safe

## CAN Integration

- Uses external `cansend` utility for CAN message transmission
- Requires CAN interface (e.g., `vcan0`) to be configured
- Tasks spawn child processes for `cansend` execution
- Error handling for `cansend` failures with exit code/signal logging

## Development Patterns

- **Thread Safety**: Mutex protection for shared data structures
- **RAII**: ThreadRegistry automatically tracks thread lifecycle
- **Configuration Parsing**: Simple key=value format parsing
- **Error Handling**: System error codes with `std::error_code`
- **Child Process Management**: SIGCHLD handling for cleanup

## Common Tasks

**Adding new commands:**
1. Add command parsing in server's client handler loop
2. Implement command logic using ThreadPool/task management
3. Add to command documentation in file header

**Modifying ThreadPool:**
- Update `Task` struct for new scheduling criteria
- Modify `Cmp` comparator for priority ordering
- Ensure thread-safe enqueue/dequeue operations

**CAN interface changes:**
- Update `cansend` command construction
- Add interface validation
- Handle different CAN message formats