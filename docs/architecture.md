# Server Architecture
> What must always be true?

## Overview

This project implements a **single-process, event-driven server** using UNIX domain stream sockets.

Key characteristics:

* Single-threaded
* I/O multiplexing via `select()`
* Fixed-size client table
* Line-based text protocol (`\n`-terminated)

The design prioritizes **simplicity, correctness, and explicit state management** over early optimization.

---

## High-level Architecture

```
+-----------------------+
|       Server          |
|-----------------------|
|  select() event loop  |
|                       |
|  +-----------------+  |
|  | server_socket   |<-+-- accept()
|  +-----------------+  |
|          |            |
|          v            |
|  +-----------------+  |
|  | clients[] table |  |
|  |  fd -> Client   |  |
|  +-----------------+  |
|          |            |
|          v            |
|   recv / send         |
+-----------------------+
```

---

## Concurrency Model

### Event-driven I/O with `select()`

The server uses `select()` to wait for readiness events on multiple file descriptors:

* The listening socket (new incoming connections)
* Connected client sockets (incoming data or disconnects)

This allows a single thread to handle multiple clients without blocking on any one of them.

**Important distinction**:

* `select()` provides **I/O multiplexing**, not parallel execution.
* The server remains single-threaded.

---

### Why not `fork()` or threads?

Alternative models were considered:

#### `fork()` per client

* Simple mental model
* Each client blocks independently
* Poor fit for shared state (rooms, broadcasts)
* Higher resource usage

#### Threads + mutexes

* Shared memory model
* Requires careful synchronization
* Risk of race conditions and deadlocks
* More complexity at this stage

`select()` was chosen as a clear, explicit baseline before considering `epoll()`.

---

## Client Management

### Client table

Clients are stored in a fixed-size array:

```c
Client clients[MAX_CLIENTS];
```

Each entry represents one connected client or an unused slot.

Advantages:

* Predictable memory usage
* Simple iteration
* Natural pairing with `select()`’s O(n) scanning model

Future versions may replace this with fd-indexed tables or maps when migrating to `epoll()`.

---

## Protocol Design

### Stream semantics

UNIX stream sockets are **byte streams**:

* Message boundaries are not preserved
* One `recv()` does **not** correspond to one message

Therefore, the protocol must define explicit framing.

---

### Framing strategy (current choice)

**Delimiter-based protocol (****`\n`****)**
* Message delimiter: `\n`
* Each client connection maintains an independent input buffer
* Incoming data may be **fragmented** or **coalesced** arbitrarily by the transport
* Messages are extracted and processed in FIFO order

**Framing Invariants**
* Parsing is strictly delimiter-driven; no data beyond the first delimiter is interpreted
* New bytes are only appended at `inbuf + inbuf_len`
* Complete messages are removed from the buffer after processing
* Maximum message length: INBUF_SIZE - 1

**Message Properties**
* Message payload is treated as opaque bytes 
* Meaning is assigned later by protocol command parsing
* Messages may contain arbitrary bytes except the delimiter (`\n`) 
* Embedded NUL bytes are permitted but may truncate debug output

**Line ending normalization**
* `\n` is the sole message delimiter
* If the byte immediately preceding `\n` is `\r`, it is stripped
* This allows transparent handling of both LF and CRLF input

**Error handling**
* Buffer overflow (no limited within buffer capacity) → disconnect
* `recv() == 0` (peer closed connection) → disconnect
* Malformed command → disconnect

Benefits:

* Simple, deterministic implementation
* Easy debugging with tools such as `nc` and `socat`
* Human-readable protocol traffic

---

### Alternative strategies (future)

* Length-prefixed messages (binary protocols)
* Structured encodings (e.g. protobuf, msgpack)

These are intentionally deferred until a clear need arises.

---

## Design Principles

* Prefer explicit state over implicit behavior
* Do not store unused or speculative fields in structures
* Introduce complexity only when justified by requirements
* Optimize for clarity before scalability

---

## Scope Notes

* This document describes **current architecture**, not future aspirations.
* Detailed protocol specification and room management are defined elsewhere.