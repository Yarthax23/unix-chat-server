# Event‑Driven Chat Server in C

A learning‑focused systems programming project to explore **low‑level networking**, **I/O multiplexing**, and **protocol design** in C.

The project incrementally builds a multi‑client chat server, prioritizing correctness, explicit state management, and clear architectural decisions.



## Current Status

**Current implementation**

* UNIX domain stream sockets (`AF_UNIX`, `SOCK_STREAM`)
* Single‑process, single‑threaded server
* Event‑driven I/O using `select()`
* Fixed‑size client table
* Line‑based text protocol (planned, `\n`‑terminated)

**TCP support is planned for a later phase**, particularly during the Go migration.


## Features

### Implemented

* Project skeleton and build automation (`Makefile`)
* AF_UNIX server socket setup
* `select()`‑based event loop
* Client tracking via fixed‑size array
* Architecture and design documentation

### Planned (near term)

* Line‑based text protocol (`\n`‑terminated)
* Message broadcasting
* Command system (`/rooms`, `/join`, `/quit`, …)
* Graceful client disconnect handling
* Input buffering and protocol framing

### Planned (mid‑term)

* Chat rooms
* Small built‑in minigames (`/roll`, `/guess`, `/vote`)
* Migration to TCP during Go rewrite
* REST API
* Dockerization


## Protocol Overview (Planned)

Simple text protocol:

* Messages are UTF‑8 text ending with `\n`
* Commands start with `/`
* Example commands:

  * `/rooms` — list available rooms
  * `/join <room>` — join a room
  * `/quit` — disconnect


## Build Instructions

### Compile

```sh
make all
```

### Run the server

```sh
make run
```

### Clean build artifacts

```sh
make clean
```


## Project Structure

```text
├── app/
│   └── main.c              # Application entry point
├── docs/
│   ├── architecture.md     # Canonical architecture documentation
│   └── standards.md        # Internal coding and design rules
├── src/
│   ├── server.c            # Server implementation
│   └── server.h            # Public server interface
├── test/
├── NOTES.md                # Personal notes and ideas
├── PROJECT_LOG.md          # Daily progress and decisions
├── README.md               # Project overview (this file)
├── Makefile
└── .gitignore
```


## Architecture (High‑Level)

* Single process
* Event‑driven I/O using `select()`
* Clients managed via a fixed‑size table
* Explicit protocol framing (no reliance on `recv()` boundaries)

See `docs/architecture.md` for full details.


## Roadmap

### Phase A — Core in C

1. Multi‑client chat server (AF_UNIX)
2. Rooms
3. Commands and protocol framing
4. Minigames
5. Stability and refactor pass

### Phase B — Migration to Go

6. Port core logic
7. TCP networking
8. REST API
9. Dockerization

### Phase C — UI

10. Optional React interface
11. Tests and full documentation


## Project Philosophy

This is a **learning‑oriented project**.
Design decisions are documented explicitly, and simplicity is preferred over premature optimization.

See:

* `docs/architecture.md`
* `PROJECT_LOG.md`
* `docs/standards.md`
