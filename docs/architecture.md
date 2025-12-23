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

The v0.1.x series establishes and validates the initial protocol semantics and execution model.

v0.1.x uses blocking sockets coordinated via `select()`. This decision is intentional and may be revisited in future iterations.


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

Incoming data flows through the server as a strict pipeline:
```
recv → byte stream → framing → grammar → intent → execution → delivery → send
```
Each stage has exclusive responsibility and must not perform work belonging to a later stage.


### Dependencies
```
+-------------------------------+
|   server.c                    |
|   ├── owns Client state       |
|   ├── owns sockets            |
|   ├── owns event loop         |
|   ├── owns broadcast_*        |
|   └── calls grammar           |
|                               |
|   grammar.c                   |
|   ├── parses messages         |
|   ├── validates arguments     |
|   ├── returns intent          |
|   └── NEVER performs I/O      |
+-------------------------------+
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

**Delimiter-based protocol (`\n`)**
* Message delimiter: `\n`
* Each client connection maintains an independent input buffer
* Incoming data may be **fragmented** or **coalesced** arbitrarily by the transport
* Messages are extracted and processed in FIFO order

**Framing Invariants**

* Parsing is delimiter-driven (`\n`)
* Bytes are appended only at `inbuf + inbuf_len`
* Complete messages are processed FIFO and removed
* Maximum message length: `INBUF_SIZE - 1`

**Message Properties**
* Message payload is treated as opaque bytes 
* Messages may contain arbitrary bytes except the delimiter (`\n`) 
* Embedded NUL bytes are permitted but may truncate debug output

**Line ending normalization**
* `\n` is the sole message delimiter
* If the byte immediately preceding `\n` is `\r`, it is stripped
* This allows transparent handling of both LF and CRLF input

**Disconnect Conditions**
* Buffer overflow (no delimiter found within buffer capacity)
* `recv() == 0` (peer closed connection)

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

Each framed line is parsed as a single command.

**Grammar responsibilities**
* Validate syntax and arguments
* Interpret command semantics
* Emit intent (`command_action`)
* Signal disconnect on error

**Characteristics**
* Pure and read-only over input bytes
* No buffer mutation
* No I/O
* No state ownership

#### Commands
##### NICK
* Declare intent to set or change username.

    `NICK <username>`
* Rules
    * `<username>` is opaque bytes
    * Length: `1..USERNAME_MAX-1`
    * Must not contain spaces
    * May be reassigned at any time
* Effects
    * Set or change the client's username
    * No room membership changes
    * No broadcasts
* Errors
    * Missing argument
    * Argument too long
    * Argument contains spaces
##### JOIN
* Declare intent to enter a room.

    `JOIN <room_id>`
* Rules
    * `<room_id>` parsed using `strtol`
    * Valid range: `0..MAX_ROOM_ID` (protocol-defined bound)
    * No extra arguments
* Effects
    * If already in a room: emit LEAVE to old room
    * Set `client.room_id` to `<room_id>`
    * Emit JOIN to destination room
* Errors
    * Missing argument
    * Non-numeric argument
    * Overflow or underflow
##### LEAVE
* Declare intent to leave current room.

    `LEAVE`
* Rules
    * No arguments allowed
* Effects
    * Emit LEAVE to current room
    * Set `client.room_id = -1`
* Errors
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
    * Broadcasts payload to all clients in sender's room
    * Sender echo behavior is implementation-defined but consistent
* Errors
    * Client is not in a room
    * Missing payload (no space after `MSG`)
##### QUIT
* Declare intent to disconnect.

    `QUIT`
* Rules
    * No arguments allowed
* Effects
    * If in a room: emit QUIT to that room
    * Remove client via `client_remove`
    * Close socket
* Errors
    * Extra arguments
---
**Error Policy**

Any protocol violation results in immediate disconnect.
There are no error replies, partial recovery, or warnings.


This keeps state simple, testing deterministic and failure modes obvious.

**Parsing Strategy (Non-Normative Implementation Notes)**

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

The server executes validated intent and emits all protocol-visible behavior.
All observable behavior flows through server-owned execution.

Protocol-visible output produced by the server falls into two categories:
* Server events, which describe server-observed state transitions
* Room messages, which relay client-authored payloads

Only the former are considered server events.

#### Server Events
Server events are messages authored by the server to announce changes in system state (e.g. room membership or client lifetime).

`[server] <EVENT> <ARGS>\n`
* `[server]` is a literal ASCII prefix
* `<EVENT>` is an uppercase ASCII token
* `<ARGS>` are space-separated, event-specific arguments
* No quoting or escaping

These messages are protocol-defined output, not debug or logging output.

##### JOIN
* Emitted when a client becomes a member of a room.

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
* Emitted when a client disconnects.

    `[server] QUIT <username>`

    * Emitted before client removal
    * Only if client is in a room
    * Broadcast scope: client's current room
    * Not delivered to the disconnecting client

---

#### Room Message Delivery
Client-authored messages (MSG) are delivered as room messages and do not use the server event format.

`<username>: <payload>\n`
* "`: `" is a literal two-byte delimiter (colon + space)

The server does not inspect, modify, or interpret payload bytes.

---

#### Execution Invariant
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

**Grammar MUST NOT**
* Mutate `Client`
* Change username or room membership
* Perform I/O
* Broadcast messages
* Close sockets
* Remove clients
* Modify buffers


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