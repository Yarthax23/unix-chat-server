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

### Dependencies
+-----------------------------------+
|   server.c                        |
|   ├── owns Client state           |
|   ├── owns sockets                |
|   ├── owns event loop             |
|   ├── owns broadcast_message()    |
|   └── calls grammar               |
|                                   |
|   grammar.c                       |
|   ├── parses messages             |
|   ├── validates arguments         |
|   ├── returns intent              |
|   └── NEVER performs I/O          |
+-----------------------------------+

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

### Layer 1: Framing strategy (current choice)

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

#### Alternative strategies (future)

* Length-prefixed messages (binary protocols)
* Structured encodings (e.g. protobuf, msgpack)

These are intentionally deferred until a clear need arises.

---

### Layer 2: Grammar & Command Parsing (current choice)

**Message model**
* Each message is a single framed line, terminated by `\n`
* Framing guarantees:
    * No embedded `\n`
    * Length ≤ INBUF_SIZE - 1
* Grammar operates on raw bytes
* Commands are ASCII-sensitive
* Parsing is byte-oriented, not locale-aware
    * No automatic detection of a client's language and country settings is issued

**General syntax**
* `<command> [<arg> ...]`
* Fields are separated by single spaces (`0x20`)
* No quoting or escaping
* Trailing spaces are not allowed
* Empty lines are invalid

**Responsibilities**
* Validate syntax
* Interpret meaning (semantics)
* Trigger state changes
* Disconnect on error

**Characteristics**
* Read-only over message bytes
* No buffer mutation
* No memory ownership
* No copying unless needed
* No delimiter handling

Trying to "reuse" framing extraction logic for grammar parsing would blur layers and introduce bugs.

#### Commands
##### NICK
* Set or change username.

    `NICK <username>`
* Rules
    * `<username>` is opaque bytes
    * Length: `1..USERNAME_MAX-1`
    * No spaces
    * May be reassigned at any time
* Effects
    * Updates `client.username`
    * Does not broadcast
* Errors → disconnect
    * Missing argument
    * Too long
    * Contains spaces
##### JOIN
* Enter a room.

    `JOIN <room_id>`
* Rules
    * Parsed using `strtol`
    * Valid range: implementation-defined (e.g. `0..INT_MAX`)
* Effects
    * Sets `client.room_id`
    * Leaving previous room implicitly
    * Broadcasts join event to new room
* Errors → disconnect
    * Missing argument
    * Non-numeric
    * Overflow
##### LEAVE
* Leave current room.

    `LEAVE`
* Effects
    * Sets `room_id = -1`
    * Broadcasts leave event
* Errors → disconnect
    * Extra arguments
##### MSG
* Send a message to current room.

    `MSG <payload>`
* Rules
    * `<payload>` is opaque bytes
    * May contain spaces
    * May contain NULs
    * Empty payload is allowed
* Effects
    * Broadcast payload to all clients in same room
    * Sender may or may not receive echo (policy choice)
* Errors → disconnect
    * Not in a room
    * Missing payload
##### QUIT
* Disconnect voluntarily.

    `QUIT`
* Effects
    * Server closes socket
    * Cleanup via `client_remove`
* Errors → disconnect
    * Extra arguments

**Error Policy**

* Any grammar violation → immediate disconnect
    * No error responses
    * No partial recovery
    * No warnings

This keeps:
* State simple
* Testing deterministic
* Failure modes obvious

**Parsing Strategy (Implementation Notes)**

* Use `memchr` to find first space
* Compare command using `memcmp`
* Avoid `strtok`
* Avoid copying unless necessary
* Do not NUL-terminate unless required

**Non-Goals (Explicit)**

* No quoting
* No escaping
* No UTF-8 validation
* No authentication
* No permission checks

These may be layered later without breaking framing.

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