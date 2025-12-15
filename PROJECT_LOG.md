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

* [ ] Define command grammar and semantics on top of the line-based protocol.
* [ ] Implement command dispatch (handle_command) with explicit validation rules.
* [ ] Decide on blocking model boundaries (what remains blocking, what may change later).
* [ ] Introduce server-generated messages (server full, client disconnect).
* [ ] Begin documenting protocol commands and error responses.


### Notes

* Relative UNIX socket path bug discovered and fixed; absolute paths are now required.
* Extensive manual testing with nc, socat, printf, and Python (-c 'print("")') confirmed framing behavior under edge cases (CRLF, embedded NULs, large payloads, missing delimiters, overflow).


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