## 0. Philosophy

> GitHub is not a “museum of finished projects", it is the history of your process.

---

## 1. Ideas

* Design a simple protocol
* Set a default username: Client i: pass index into client_init
* Implement rooms
* Mini-games / commands:
  * /roll dice
  * /guess secret number
  * /vote shared decisions
  * produced/consumer (e.g. washing machine simulation)
  * change Client's Name, /nick <username>
  * /brainstorm mode that updates and sorts chat ideas (TBD)
* Futuro: 
  * migrar to Go

---

## 2. Things to Research

* Testing frameworks in C (CUnit, Unity, Criterion)
* Basic security / hardening
* Graceful client disconnects
* Signals and process handling (`sigaction`: `signal` is deprecated)
* `epoll` as a scalable alternative to `select`
* Linux and Unix System Programming course https://man7.org/training/

---

## 3. Future Architecture Ideas (Non-binding)
Detailed and authoritative architecture diagrams live in `/docs/architecture.md`. This section tracks future diagram ideas only.
* ASCII diagrams are perfect for `architecture.md` v1
  * Keep them conceptual
  * Don’t try to be “pretty”
  * One diagram per idea
  * Later (much later), migrate to Mermaid or draw.io if desired.
  * See https://www.geeksforgeeks.org/c/socket-programming-cc/ for future reference ideas on diagramming.

### Pending Diagrams
* Client-server flow
* Component diagram:
  * `server.c`
  * client manager
  * futuro: room manager
  * futuro: message dispatcher
  * futuro: protocol parser

### Formal Protocol Specification
  * commands
  * errors
  * states

### Decisiones técnicas futuras
  * non-blocking I/O
  * scalability

### Future protocol improvements

* Server-to-client informational messages:
  * server full
  * client joined / left
* Distinguish broadcast vs client-directed messages
  * This is intentionally deferred to avoid protocol complexity during initial framing work.
* In network-facing servers:
  * log details server-side
  * send generic errors client-side
* Semantic / Command Handler
  

---

## 4. Manual Framing & Robustness Experiments

The following tests were executed to validate framing invariants, buffer limits, and byte-level behavior.

### Observations & Conclusions
* Tested with:
  * `socat - UNIX-CONNECT:/tmp/unix_socket`
  * `nc -U /tmp/unix_socket`.
* UNIX stream sockets are byte streams (confirmed empirically)
* recv() boundaries are unrelated to message boundaries
* Control characters, NUL bytes, high-ASCII and binary data are received correctly
* `printf("%s")` truncates at NUL — expected and debug-only
* Pipes + `socat` create short-lived clients (EOF triggers disconnect)
* Input is treated as opaque bytes at the framing layer
* Sanitization is intentionally deferred to the command parsing layer

### Testing performed
#### Interactive testing 
`socat - UNIX-CONNECT:/tmp/unix_socket`
`nc -U /tmp/unix_socket`

* `recv`  boundaries are still unreliable
* `socat`/`nc` hide fragmentation and coalescing
* Pressing Enter always send `\n`
* Backslash (`\`) is opaque (`\x`, `\n`, `\t`, `\r`, `\0` are not interpreted)

#### Inject input testing
* Message without delimited is not processed
  * printf "hello world" | socat - UNIX-CONNECT:/tmp/unix_socket
* Normal newline-delimited message:
  * printf "hello world\n" | socat - UNIX-CONNECT:/tmp/unix_socket
  `says: hello world`
* Embedded NUL truncates debug output
  * printf "hello\0world\n" | socat - UNIX-CONNECT:/tmp/unix_socket
  `says: hello`
* Coalesced messages
  * printf "hello\nworld\n" | socat - UNIX-CONNECT:/tmp/unix_socket
  `says: hello` ; `says: world`
* CLRF normalization
  * printf "hello\nworld\r\n" | socat - UNIX-CONNECT:/tmp/unix_socket `says: hello` ; `says: world`
* Byte opacity
  * printf "\x3A\n\x3B\n \x24\n\x2C" | socat - UNIX-CONNECT:/tmp/unix_socket `says: :` ; `says: ;` ; `says: $`
* Tabs and spacing  
  * printf "Tabulate\t1\x092\x093\t4\n\tthis,\n  Alright\n" | socat - UNIX-CONNECT:/tmp/unix_socket
  ```
  says: Tabulate        1       2       3       4
  says:         this,
  says:   Alright
  ```
* Format string test (non-exploitable)
  * python3 -c 'print("Z"*80+"\xAddress\n")' | socat - UNIX-CONNECT:/tmp/unix_socket `says: ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ­dress` ; `says: `
* Buffer overflow enforcement
  * python3 -c 'print("B"*1024+"\xDEADBEEF\n")' | socat - UNIX-CONNECT:/tmp/unix_socket
    > 2025/12/14 21:17:07 socat[97850] E read(5, 0x5833c825e000, 8192): Connection reset by peer `buffer overflow`
* Below overflow threshold (\xDE = ASCII Þ)
  * python3 -c 'print("B"*1000+"\xDEADBEEF\n")' | socat - UNIX-CONNECT:/tmp/unix_socket ` says: BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBÞADBEEF` ; `says: `

---

## 5. Explicitly Out of Scope (Current Stage)
The following concerns are acknowledged but intentionally deferred:

* Input sanitization / whitelisting
* Command validation and semantics
* Authentication and authorization
* Encryption (TLS)
* Format-string attacks beyond debug output
* Resource exhaustion attacks beyond basic buffer limits
* Persistent logging infrastructure
* Graceful reconnect semantics
* Production-grade error reporting to clients

These will be revisited once the protocol grammar and semantics are defined.

---

## 6. Roadmap

### Phase A — C Technical Foundation

#### 1. Stream Socket Chat

* Minimum security
  * Robust framing
  * Buffer limits
  * Deferred sanitization to grammar and semantics

#### 2. Salas + comandos

* Data Structures (`Client`, rooms)
* Synchronization
* Commands (`/join`, `/rooms`, etc.)
* Command parser (if (buffer[0] == '/') → structured parsing)

#### 3. Mini-games (see above)

### Phase B — Migración a Go

1. Migrate core logic (TCP Chat)
2. Convert to REST API
3. Dockerize (see SO containers subject)
4. Add basic authentication
5. Tests
6. Solid Documentation
7. UI (Phase C, possibly React)