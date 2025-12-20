## 2025-12-19 – Execution Semantics Finalization & Protocol Lock-In
### Summary

Finalized execution-layer semantics and protocol-visible server events after the intent-based refactor.

Grammar is now strictly pure: it emits intent only and never mutates client state.
All authoritative state changes (username, room membership, lifetime) are owned by the server execution layer.

This entry locks down broadcast timing, server-generated message semantics, and helper contracts, bringing the protocol design to a stable, auditable state.

### Decisions

* Grammar must not mutate `Client` under any circumstance, including username changes.
* Username mutation is an execution-layer responsibility, applied only after validated intent.
* JOIN is a two-phase transition:
    * LEAVE is emitted to the old room (if any)
    * JOIN is emitted to the destination room after mutation
* LEAVE and QUIT are distinct server events with separate broadcast helpers.
* Broadcast helpers are pure fan-out utilities and never perform mutation or policy decisions.
* Protocol-visible server messages are part of the protocol surface, not debug output.

### Added

* Documented server-generated message format and event semantics in `architecture.md`.
* Defined explicit contracts for broadcast helpers (scope, ordering, exclusions).
* `broadcast_quit` helper to represent disconnect semantics distinctly from room leave events.

### Changed

* Centralized username mutation logic in the server execution layer.
* Clarified execution ordering guarantees relative to room membership transitions.
* Refined grammar and execution documentation to consistently use intent-based language.
* Tightened architectural scope notes and removed redundant non-goals.

### Removed

* Implicit grammar-side username mutation.
* Overlapping or ambiguous language around broadcast ordering and scope.
* Redundant “non-goals” blocks once protocol boundaries were fully specified.

### Learnings

* Treating grammar as a pure function (bytes → intent) dramatically simplifies reasoning.
* Separating LEAVE and QUIT events avoids semantic leakage and special cases later.
* Ordering guarantees are semantic contracts for observers, not properties of control flow.
* Once invariants stabilize, documentation should be compressed—not expanded.

### Notes

* `command_action` intentionally does not mirror `Client`; it carries only execution context.
* Broadcasts are scoped to the room where the event is observed, not where the client ends up.
* Printf-only messages are explicitly rejected as non-protocol behavior.
* With execution semantics locked, further work should focus on implementation quality, not design.


## 2025-12-18 – Intent-Based Command Execution Refactor
### Summary

Completed all grammar-level protocol commands and refactored the command execution model to enforce a strict separation between intent extraction and side effects.

Grammar is now purely declarative, while the server is authoritative over state mutation and broadcasting.

**Milestone:** All grammar commands are implemented, validated, and stable.

### Decisions

* Introduced an explicit `command_action` intent object to decouple grammar from execution.
* Unused `.room_id` fields in `command_action` are set to `-1` to avoid ambiguity (`Room 0` is valid).
* Grammar returns intent + context only; the server applies mutations and enforces reordering.

### Added

* Completed documentation for the `JOIN` command.
* `LEAVE` command handler with strict argument validation.
* `MSG` command handler with strict argument validation.
* `command_action` struct to represent validated protocol intent.
* Helper constructors (`action_*`) for semantic, self-documenting intent creation.

### Changed

* Replaced `command_result` into `command_action` for clarity and extensibility.
    * `room_id` represents transition context, not "where to broadcast".
        * Grammar describes intent.
        * Transition context is explicit.
        * Server applies mutation.
    * Payload lifetime are explictly defined.
        * Payload remains opaque bytes.
        * Must remain valid until the server consumes it.
* Refactored all `handle_*` functions.
    * Removed side effects and state mutation.
    * Grammar now performs syntax validation only.
* Updated `server.c` to:
    * Interpret intent objects.
    * Mutate authoritative state.
    * Execute room-scoped broadcasts accordingly.
* Renamed and evolved `broadcast_message` to `broadcast_room` for explicit room scoping.
* Client initialization now assigns a default username.

### Removed

* Unused `find_client_by_fd` helper.

### Learnings

* Idiomatic C intent objects use `static inline` constructors.
    * File-local.
    * Zero call overhead.
    * Semantic and readable.
    * No ABI exposure.
* Compound literals guarantee zero-initialization of unused fields.
    * Fields in `command_action` are context, not a mirror of `Client` state.
* Separating intents from execution simplifies reasoning about:
    * Object lifetimes
    * Mutation ordering
    * Future features (e.g. nick-change broadcasts)

### Next steps

* [ ] Decide which server-generated messages are part of the protocol surface
* [ ] Specify join / leave notification semantics (wording, scope, ordering)
* [ ] Implement once semantics are finalized

### Notes

* Validation strategy:
    * Presence-forbidden commands use combined check ("must be absent").
        * Reads as one invariant.
        * Best for commands that forbid arguments.
    * Presence-required commands use split checks ("must exist").
        * Each failure has a clear semantic meaning.
        * Better for later-use: different error handling.
* `grammar.c` never performs side effects.
    * It does not send data, close sockets, mutate global state, or broadcast.
    * It only parses input and returns intent.
* Broadcasts target the room where the event is observed, not the room the client ends up in.
* Join/leave notifications have been intentionally deferred until the execution model and intent boundaries stabilized. With `command_action` in place, this feature is now unblocked but still pending semantic specification.


## 2025-12-17 – JOIN Command Implementation
### Summary

Implemented the `JOIN` command under the established grammar rules.
Focused on strict numeric argument validation (`strtol`), overflow handling,
and enforcing disconnect-on-error semantics as defined by the protocol.

### Decisions

* Apply previously established grammar invariants to `JOIN`.
* Use `strtol` with explicit `errno` and end-pointer checks for numeric parsing.
* Treat partially valid input (e.g. `123abc`) as a grammar violation.

### Added

* `JOIN` command handler with strict argument validation.
* Numeric parsing logic with overflow and non-numeric detection.

### Changed

* Grammar dispatch extended to include `JOIN`.

### Learnings

* `strtol` is safe only when:
  * `errno` is checked for overflow
  * the end pointer consumes the full argument
* Numeric parsing bugs are easy to introduce when validation is implicit.

### Next steps

* [x] Implement `LEAVE`
* [x] Implement `MSG`


## 2025-12-16 –  Grammar Implementation and Lifecycle Corrections
### Summary

Implemented the first protocol command (`NICK`) following the previously defined grammar and semantics. Introduced a clean command dispatch mechanism, corrected object lifetime violations between grammar and server layers, and formalized module responsibilities.

**Milestone:** Grammar layer implemented and enforceable.

### Decisions

* From this point forward, all new `typedef`’d types use lowercase `snake_case`.
* Use command specification structures to dispatch and handle protocol messages.
* Adopt `switch`-based command multiplexing.
    * Explicit control flow.
    * Easy to debug and audit.

### Added

* Initial command handling implementation (`NICK`) with strict argument validation.
* First explicit dependency documentation in `architecture.md`.
* Formal command specifications in the grammar layer.

### Changed

* Updated project structure in `README.md`.
* Refactored `client_remove` to operate on `Client *` instead of index.
* Made `Client` explicitly tagged to allow forward declarations and avoid type ambiguity.
    * `Client *c` and `struct Client *c` are now non-conflicting.
* Enforced grammar purity:
    * Grammar code no longer mutates connection lifetime.
    * `handle_command` now returns a `ommand_result`
    * Client teardown is server-owned and authoritative.
    * This preserves object lifetime safety and separation of concerns.

### Learnings

* Module boundaries.
    * Circular header inclusion is what breaks builds.
    * Headers expose what other modules must know to use a module — not what a module happens to use internally.
    * Headers define obligations, not conveniences.
* Validation strategy.
    * Validate incrementally; fail fast.
    * One invariant per check.
    * Easier to reason about and audit.
    * Large compound conditionals are a bug factory.
* APIs should match how data naturally flows through the system.

### Next steps

* [x] Implement remaining protocol commands (`JOIN`, `LEAVE`, `MSG`, `QUIT`)
* [ ] Add basic server-generated messages (join/leave notifications)
* [ ] Expand manual protocol testing for all commands

### Notes

* `goto` is useful for branching to a cleanup or failure block.
    * Removes duplicated cleanup logic.
    * Avoid for general control flow (spaghetti code).
    * Reference https://www.geeksforgeeks.org/c/goto-statement-in-c/.
* Every commit must build cleanly with the project’s own compiler flags.
* Do not introduce new observable behavior while still implementing the spec that defines it.


## 2025-12-15 –  From Framing to Meaning: Command Grammar
### Summary

Defined the command grammar and semantics on top of the already validated line-based framing layer. Formalized responsibilities and boundaries between framing and grammar parsing, and documented a minimal, unambiguous command set designed for deterministic parsing and early-stage correctness.

No implementation was started today; the focus was on architectural clarity and specification stability before coding.

### Decisions

* Adopt an explicit, unambiguous command grammar with minimal parser complexity.
    * All input is parsed uniformly as commands; no "command vs message" heuristics.
    * Grammar violations result in immediate disconnect.
* Keep grammar parsing strictly read-only over framed message bytes.
    * No buffer mutation, no delimiter handling, no memory ownership.
* Defer room and client modules.
    * Rooms are implicit, defined by shared `room_id`.
    * Broadcasting is implemented by iterating over `clients[]`.
* Treat `room_id = -1` as "not in any room", not a joinable lobby.

### Added

* Grammar and command parsing layer documentation in `architecture.md`.
* Formal command definitions: `NICK`, `JOIN`, `LEAVE`, `MSG`, `QUIT`.
* Explicit error-handling and disconnect policy for grammar violations.

### Changed

* Clarified architectural layering between framing and grammar parsing.
* Documented parsing constraints and non-goals for early protocol versions.

### Learnings

* Protocol grammar design benefits from being explicit and failure-intolerant in early stages.
* Separating framing from grammar avoids subtle buffer ownership and parsing bugs.
* Many familiar chat protocol features (quoting, escaping, free-form messages) introduce significant parser complexity.

### Next steps

* [x] Implement `handle_command()` with strict validation and explicit disconnect paths.
* [ ] Add basic server-generated messages (join/leave notifications).
* [ ] Write targeted tests using `nc`/`socat` for each command and failure mode.

### Notes

* In message protocols, a payload is the content of a message excluding headers and metadata.
    * The term also appears in malware literature, where the payload is the code that performs the malicious action, distinct from the propagation mechanism.


## 2025-12-14 –  Line-Based Framing and Robustness Testing
### Summary

Implemented and validated newline-delimited stream framing for the UNIX socket server. Formalized buffer invariants, overflow handling, and disconnect-on-error policy. Manual adversarial testing with nc/socat confirmed correct behavior under fragmentation, coalescing, CRLF input, and oversized messages.

### Decisions

* Disconnect-on-error chosen to avoid state complexity in early protocol design.
* Buffer overflow is logged server-side; client receives a generic disconnect.

### Added

* Implemented newline-delimited framing with per-client buffers.

### Changed

* Added input buffer for the line-based protocol in the Client struct.
* Factored client_init to prevent stale buffer reuse.
* Clearer responsibilities, main is not responsible for client arrays (init), buffer invariants or socket internals.

### Learnings

* GitHub Documentation on Commit Conventions and Changing a commit message (amend).
* Protocol fundaments: CRLF and LF.
    * Carriage Return (`\r`, 0x0D) + LF, used by Telnet, HTTP, SMTP, many network text protocols.
    * Line Feed (`\n`, 0x0A), used by Unix/Linux line ending.
* `signal-safety`, `attributes` and `mem*` functions man pages or documentation.
* Testing manual framing validation with `nc` or `socat` (AF_UNIX), confirmed framing invariants.

### Next steps

* [x] Define command grammar and semantics on top of the line-based protocol.
* [x] Implement command dispatch (handle_command) with explicit validation rules.
* [ ] Decide on blocking model boundaries (what remains blocking, what may change later).
* [ ] Introduce server-generated messages (server full, client disconnect).
* [x] Begin documenting protocol commands and error responses.


### Notes

* Relative UNIX socket path bug discovered and fixed; absolute paths are now required.
* Extensive manual testing with nc, socat, printf, and Python (-c 'print("")') confirmed framing behavior under edge cases (CRLF, embedded NULs, large payloads, missing delimiters, overflow).
* You cannot destroy the current object and then continue executing code that assumes it exists.


## 2025-12-13 –  Transition to Event-Driven Server Architecture
### **Summary**

Implemented a single‑process AF_UNIX stream server with safe static initialization. Introduced a fixed‑size client table and laid the groundwork for a `select()`‑based event loop. Replaced port‑based networking with filesystem socket paths.

### **Decisions**

* Use bounded string functions instead of unbounded copies.
    * Avoid `strcpy`, `strcat`; prefer `snprintf`, `strn*` with explicit size checks.
    * Note that `snprintf` silently truncates and requires return-value checking.

* Adopt `select` for I/O multiplexing.
    * Single‑threaded, event‑driven model.
    * Chosen over `fork` and threads for clarity and teaching value.
* Represent clients using a fixed‑size array (clients[MAX_CLIENTS]).
    * Avoid dynamic allocation at this stage.
* Do not store peer `sockaddr_un` in `Client`.
    * AF_UNIX + `SOCK_STREAM` peer address is not useful post-`accept`.
    * Server logic does not depend on client path identity.
    * Eliminates unused state and simplifies the data model.
* Document module-level design rules in `standards.md`.

### **Added**

* AF_UNIX server socket setup (`socket`, `bind`, `listen`, `unlink`).
* Client table initialization logic.
* Initial `select`‑based server architecture (single process, multiplexed I/O).

### **Changed**

* Replaced port-based addressing with filesystem socket paths.
* Consolidated server responsibilites into `server.c`, `server.h`.

### **Removed**

* Storage of peer address (`sockaddr_un`) from the `Client` structure.

### **Learnings**

* Conventions and correctness for header files.
* `memset(..., 0, ...)` establishes a safe baseline for structs passed to the kernel:
    * Ensures `sun_path` is NULL-terminated.
    * Clears unused bytes.
    * Prevents leaking uninitialized data.
* Previously introduced to a fork()-per-client server model.
* Blocking I/O in child processes is simple but inefficient and awkward for shared state.
* Switched to a single-process, event-driven model using select().
* select() performs I/O multiplexing (not concurrency): the processs waits for any FD to become ready.
    * One thread waits on many file descriptors.
    * The process never blocks on the “wrong” operation.
* Limitations of select() (FD limits, O(n) scanning) are acceptable at this scale and make it a good stepping stone toward epoll().
* Comparison of models:
    * select():
        * FD limit (often 1024), O(n) scanning, fd_sets rebuilt every loop.
        * Well‑suited for small servers and learning.
    * epoll():
        * No hard FD limit, kernel‑tracked readiness, O(1) event delivery.
        * Better for large‑scale systems.
* Stream sockets are byte streams:
    * Message boundaries are not preserved.
    * One recv() ≠ one message.
* Current implementation assumes complete commands per `recv()`.
    * Proper protocol framing will be introduced in a later milestone.
    * Stream sockets are byte streams; protocol framing (delimiter-based) will be implemented in a future milestone.
* First practical uses of `git reset --soft` and `git commit --amend`.

### **Next steps**

* [ ] Review blocking vs. non-blocking sockets behavior.
* [x] Define buffer sizing and message parsing strategy.
* [ ] Continue studying I/O multiplexing and concurrency models.
* [x] Prepare for protocol framing (deilimer-based).
* [ ] Explore `epoll` theory.

### **Notes**

* Struct layout is not a security boundary.
    * Field order and padding do not prevent memory corruption.
* Prefer break inside the main event loop to allow graceful shutdown:
    * Close client sockets.
    * Unlink the UNIX socket file.
    * Leave the process in a clean state.
* Use exit(EXIT_FAILURE) only outside controlled teardown paths.
* A struct should reflect what the system actually uses, not speculative future needs.

* Design decisions should be justified by correctness and clarity, not assumed defensive side effects.
* Security & Memory Safety Philosophy.
    * Strict bounds checking.
    * Proper initialization of all data.
    * Avoiding buffer overflows, especially on fixed-size arrays.
    * Clearing memory before reuse when appropriate.
    * Avoiding use-after-free and double-free errors.


## 2025-12-12 –  Initial server module setup
### **Summary**

Set up the initial server module with start_server(), integrated it in main.c, updated Makefile to simplify header includes, and documented decisions and configuration changes in PROJECT_LOG.

### **Decisions**

* Use snake_case instead of CamelCase (implemented via new macro).

### **Added**

* `server.h`, `server.c`.
* `keybindings.json` to make Shift+Space type underscore `_` in VS Code (Multi Command).
* `'I./src` CFLAG to make `#include` simple & concise.

### **Changed**

* `settings.json` to define macro for inserting `_`.

### **Removed**

* ...

### **Learnings**

* Reviewed Sockets documentation, boilerplate & notes.
* Reviewed how often to commit
* Discovered `xev` command: a debugging & development tool to print X events.
* Keybindings in VS Code can override system keys if the command does not exist.

### **Next steps**

* [x] Continue implementing server features in server.c / server.h.
* [x] Decide whether to use select() or threads + mutex for concurrency.
* [x] Explore select() for handling multiple clients concurrently.
* [x] Implement basic client handling using select().
* [ ] Research sigaction() for proper signal handling in fork() (if needed in future).
* [ ] Begin implementing broadcast_message() function.

### **Notes**

* System-wide Shift+Space remapping on Linux is tricky with XKB; AutoKey is a safer alternative if needed.


## 2025-12-11 –  Initial setup
### **Summary**

Set up the initial repository structure and development workflow. Defined conventions, added build automation, and prepared the base layout for the upcoming TCP chat server.

### **Decisions**

* Use **English** for all public-facing documentation (`README`, `docs`, commit messages).
* Keep personal notes in Spanish inside `NOTES.md`.
* Use **Makefile** for build tasks (`make all`, `make run`, `make clean`).
* Use a `docs/` folder for standards, architecture, and protocol documentation.
* Adopt **Conventional Commits**.

### **Added**

* `Makefile` with build/run/clean targets
* `docs/standards.md` for project conventions
* Initial folder structure: `app`, `src`, `test`, `docs`
* Project planning in `README.md`
* `.gitignore` rules for C builds and binaries

### **Changed**

* Reorganized files into the new structure
* Cleaned repository layout to follow a long-term scalable pattern
* Improved documentation structure and naming

### **Removed**

* Temporary/unstructured files from the initial creation stage

### **Learnings**

* Clarified the difference between `README`, `NOTES`, `PROJECT_LOG`, and `docs/` as separate documentation layers.
* Understood why GitHub isn’t a “museum of finished projects” but a historical record of the process.
* Reviewed Conventional Commit types and how they map to real work.

### **Next steps**

* [x] Implement the minimal stream server with `socket()`, `bind()`, `listen()`, `accept()`.
* [x] Decide between `select()` vs. threads + mutex for concurrency.
* [x] Write the first architecture sketches for `/docs/architecture.md`.

### **Notes**

* Investigate options for client handling: blocking vs nonblocking sockets.
* Evaluate buffer sizes and message parsing strategy for line-based protocol.