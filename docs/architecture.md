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
```
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
```

* Grammar returns validated intent; the server applies state changes and broadcasts.

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

### Layer 1: Framing strategy

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

### Layer 2: Grammar & Command Parsing

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
* Trailing spaces are invalid
* Empty lines are invalid

**Responsibilities**
* Validate syntax and arguments
* Interpret semantics
* Emit intent (`command_action`)
* Signal disconnect on error

**Characteristics**
* Read-only over message bytes
* No buffer mutation
* No I/O
* No memory ownership
* No delimiter handling

Trying to "reuse" framing extraction logic for grammar parsing would blur layers and introduce bugs.
Grammar is pure: it performs validation and intent construction only.

#### Commands
##### NICK
* Declare intent to set or change username.

    `NICK <username>`
* Rules
    * `<username>` is opaque bytes
    * Length: `1..USERNAME_MAX-1`
    * Must no contain spaces
    * May be reassigned at any time
* Effects
    * Grammar emits intent to change username
    * Server applies username mutation
    * No room membership changes
    * No broadcasts
* Errors → disconnect
    * Missing argument
    * Argument too long
    * Argument contains spaces
##### JOIN
* Declare intent to enter a room.

    `JOIN <room_id>`
* Rules
    * `<room_id>` parsed using `strtol`
    * Valid range: implementation-defined (`0..INT_MAX`)
    * No extra arguments
* Effects
    * Grammar emits intent to join `<room_id>`
    * Server execution:
        * If already in a room: emit LEAVE to old room
        * Set `clients.room_id` to `<room_id>`
        * Emit JOIN to destination room
* Errors → disconnect
    * Missing argument
    * Non-numeric argument
    * Overflow or underflow
##### LEAVE
* Declare intent to leave current room.

    `LEAVE`
* Rules
    * No arguments allowed
* Effects
    * Grammar emits intent to leave current room
    * Server execution:
        * Emit LEAVE to current room
        * Set `client.room_id = -1`
* Errors → disconnect
    * Extra arguments
##### MSG
* Declare intent to send a message to the current room.

    `MSG <payload>`
* Rules
    * `<payload>` is opaque bytes
    * May contain spaces
    * May contain embedded NUL bytes
    * Empty payload is allowed
    * Payload begins after the first space
* Effects
    * Grammar emits intent to broadcast payload
    * Server execution:
        * Broadcasts payload to all clients in sender's room
        * Sender echo behavior is implementation-defined but consistent
* Errors → disconnect
    * Client is not in a room
    * Missing payload (no space after `MSG`)
##### QUIT
* Declare intent to disconnect.

    `QUIT`
* Rules
    * No arguments allowed
* Effects
    * Grammar emits intent to disconnect
    * Server execution:
        * If in a room: emit QUIT to that room
        * Remove client via `client_remove`
        * Close socket
* Errors → disconnect
    * Extra arguments
---
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

* No quoting or escaping
* No UTF-8 validation
* No authentication
* No permission checks

These may be layered later without breaking framing.

---

### Layer 3: Executing Room-Scoped Broadcasts and Server Messages

The server executes validated intent and emits protocol-visible events.
All observable behavior flows through server-owned execution.

**Server message format**

`[server] <EVENT> <ARGS>\n`

* `[server]` is a literal ASCII prefix
* `<EVENT>` is an uppercase ASCII token
* `<ARGS>` are space-separated, event-specific arguments
* No quoting or escaping

These messages are observable protocol behavior, not debug output.

#### Server Events
##### JOIN
* Emited when a client becomes a member of a room.

    `[server] JOIN <username>`

    * Emitted after `client.room_id` mutation
    * Broadcast scope: destination room
    * Not delivered to the joining client

##### LEAVE
* Emitted when a client leaves a room voluntarily.

    `[server] LEAVE <username>`

    * Emitted before `client.room_id` mutation
    * Broadcast scope: client's current room
    * Not delivered to the leaving client

##### QUIT
* Emited when a client disconnects.

    `[server] QUIT <username>`

    * Emitted before client removal
    * Only if client is in a room
    * Broadcast scope: client's current room
    * Not delivered to the disconnecting client

---
#### Execution Invariant
* Grammar emits intent only
* Server owns all state mutation
* Server determines broadcast scope and ordering
* Helpers format and fan out messages only
* All protocol-visible effects reflect execution order

`command_action` does not mirror `Client`, it carries just enough information for the server to execute intent safely and in order.

#### Grammar Constraints
**Grammar MAY**
* Parse input bytes
* Validate syntax and arguments
* Construct `command_action`
* Reject malformed input

**Grammar MUST NOT**
* Mutate `Client`
* Change room membership
* Change username
* Perform I/O
* Broadcast
* Close Sockets
* Remove clients
* Touch buffers

#### Broadcast Helpers
* Perform formatting and fan-out only
* Iterate clients and filter by room
* Write bytes to sockets
* Never mutate state
* Assume correct ordering and context from the caller

#### Debugging Note
Printf-only messages are not part of the protocol:
* Invisible to clients
* Non-reproducible
* Guaranteed to be removed

Scope note: The protocol specifies only room membership transitions and message delivery.
UX features (timestamps, counts, acknowledgements, error replies) are intentionally out of scope.

---

## Design Principles

* Prefer explicit state over implicit behavior
* Do not store unused or speculative fields in structures
* Introduce complexity only when justified by requirements
* Optimize for clarity before scalability

---

## Scope Notes

* This document describes **current, implemented architecture**, not future aspirations.
* The protocol specifies only room membership transitions and message delivery.
* UX features (timestamps, counts, acknowledgements, error replies) are intentionally out of scope.