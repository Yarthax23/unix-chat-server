# Event‑Driven Chat Server in C

A learning‑focused systems programming project to explore **low‑level networking**, **I/O multiplexing**, and **explicit protocol design** in C.

The project incrementally builds a multi‑client chat server, prioritizing correctness, deterministic parsing, and clear architectural boundaries.



## Current Status

**Current implementation**

* UNIX domain stream sockets (`AF_UNIX`, `SOCK_STREAM`)
* Single‑process, single‑threaded server
* Event‑driven I/O using `select()`
* Fixed‑size client table (no dynamic allocation)
* Line‑based framing (`\n`-delimited)
* Explicit command grammmar
* Strict validation and disconnect-on-violation policy
* Architecture, standards, and design documentation
* Command grammar with intent-based execution model

> TCP support and Go migration are intentionally deferred to a later phase.

## Protocol Overview

The server uses a line-based, command-only text protocol.

* Messages are UTF‑8 text ending with `\n`
* All input is parsed as a command (no free-form messages)
* Grammar is deterministic and failure-intolerant
* Commands:
```markdown
  * NICK <name>     Set client nickname
  * JOIN <room_id>  Join a room
  * LEAVE           Leave current room
  * MSG <payload>   Send message to current room
  * QUIT            Disconnect
```
Grammar violations result in immediate disconnect.
See `docs/architecture.md` for the full protocol specification.

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
│   ├── server.c            # Server implementation (owns Client state)
│   ├── server.h            # Server public interface
│   ├── grammar.c           # Command parsing and validation
│   └── grammar.h           # Grammar public interface
├── test/
├── NOTES.md                # Personal notes and ideas
├── PROJECT_LOG.md          # Daily progress log
├── README.md               # Project overview (this file)
├── Makefile
└── .gitignore
```


## Architecture (High‑Level)

* Single process
* Event‑driven I/O using `select()`
* Clients managed via a fixed‑size table
* Clear separation between:
  * Framing
  * Grammar parsing
  * Server state mutation

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

## Why This Protocol Is Strict

* Every message is parsed deterministically.
* Grammar violations result in immediate disconnect.
* Separating framing from grammar ensures buffer safety and predictable state.
* All command validation occurs before any server-side state mutation.
* This approach minimizes unexpected side effects and simplifies debugging.


See:

* `docs/architecture.md`
* `docs/standards.md`
* `PROJECT_LOG.md`

