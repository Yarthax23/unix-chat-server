# Event-Driven Chat Server in C

A learning-focused systems programming project exploring **low-level networking**, **event-driven I/O**, and **explicit protocol design** in C.

The server is built incrementally with a strong emphasis on **correctness**, **deterministic parsing**, and **clear architectural boundaries** between framing, grammar, execution, and delivery.

This project prioritizes clarity and auditability over premature optimization.

---

## Current Status

**Implemented**

* UNIX domain stream sockets (`AF_UNIX`, `SOCK_STREAM`)
* Single-process, single-threaded server
* Event-driven I/O using `select()`
* Fixed-size client table (no dynamic allocation in steady-state operation)
* Line-based framing (`\n`-delimited, CRLF tolerated)
* Deterministic command grammar with intent-based execution
* Strict validation with disconnect-on-violation semantics
* Room-scoped messaging and server-generated events
* Architecture, standards, and daily project log documentation

> TCP support and any language migration are intentionally deferred.

---

## Protocol Overview

The server uses a **line-based, command-only text protocol**.

* Messages are byte sequences terminated by `\n` (commonly UTF-8 text)
* All input is parsed as a command (no free-form mode)
* Grammar is deterministic and failure-intolerant
* Grammar violations result in immediate disconnect

**Supported commands**

```text
NICK <name>     Set client nickname
JOIN <room_id>  Join a room
LEAVE           Leave current room
MSG <payload>   Send message to current room
QUIT            Disconnect
```

The grammar layer validates input and emits intent only.
All state mutation, ordering, and broadcasting are owned by the execution layer.

See `docs/architecture.md` for the full protocol and execution specification.

---

## Architecture (High-Level)

* Single process
* Event-driven I/O via `select()`
* Clients managed via a fixed-size table
* Strict separation between:

  * **Framing** (byte stream → lines)
  * **Grammar parsing** (lines → intent)
  * **Execution** (state mutation and ordering)
  * **Delivery** (room messages and server events)

A minimal end-to-end flow:

```
recv → byte stream → framing → grammar → intent → execution → delivery → send
```

---

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

---

## Project Structure

```text
├── app/
│   └── main.c                # Application entry point
├── src/
│   ├── server.c              # Server implementation (authoritative state & execution)
│   ├── server.h              # Server public interface
│   ├── grammar.c             # Command parsing and validation
│   └── grammar.h             # Grammar public interface
├── docs/
│   ├── architecture.md       # Canonical architecture, invariants, and data flow
│   ├── protocol_evolution.md # Protocol lifecycle, versioning, and forward-compat rules
│   ├── execution_audit.md    # Execution ordering and broadcast audit
│   ├── standards.md          # Coding, style, and design rules
│   └── manual_test.md        # Manual multi-client testing notes
├── assets/
│   ├── diagrams/             # Architecture and protocol diagrams
│   └── screenshots/          # Manual testing evidence
├── NOTES.md                  # Personal notes, ideas, and deferred work
├── PROJECT_LOG.md            # Chronological design and implementation log
├── README.md                 # Project overview (this file)
├── Makefile
└── .gitignore
```

---

## Roadmap

### Phase A — Core in C

1. [x] Multi‑client chat server (AF_UNIX)
2. [x] Rooms
3. [x] Commands and protocol framing
4. Minigames
5. Stability and refactor pass

### Phase B — Networking & Migration

6. TCP support
7. Optional Go port
8. REST API
9. Dockerization

### Phase C — UI & Tooling

10. Optional web interface
11. Expanded tests and documentation

---

## Project Philosophy

This is a **learning-oriented project**.

Design decisions are documented explicitly.
Simplicity, correctness, and explicit invariants are preferred over flexibility or performance claims.

Key principles:

* Deterministic parsing over permissive input
* Early validation and fail-fast behavior
* No hidden side effects across layers
* Ownership and lifetime are always explicit

---

## Why the Protocol Is Strict

* Stream sockets do not preserve message boundaries.
* Separating framing from grammar avoids buffer and lifetime bugs.
* Grammar emits intent only — execution is authoritative.
* All validation occurs before any state mutation.
* Strictness simplifies reasoning, debugging, and future extensions.

---

## Further Reading

* `docs/architecture.md` — protocol, execution semantics, and invariants
* `docs/standards.md` — coding and design rules
* `PROJECT_LOG.md` — chronological design and implementation record