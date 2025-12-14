# TCP Chat Server in C
A personal project to learn low-level networking, sockets, concurrency, and systems programming fundamentals.
The goal is to build a functional multi-level chat server with rooms, message broadcasting, and simple interactive commands.

## Features (Current & Upcoming)
### Implemented
* Basic project skeleton
* Build automation via `Makefile`
* Initial planning and documentation

### Planned (near future)
* TCP Server handling multiple clients
* Broadcasting messages to all connected clients
* Basic text protocol (`\n`-terminated messages)
* Command system (`/rooms`,`/join`,`/quit`, ...)
* Graceful client disconnects

### Planned (mid-term)
* Small built-in minigames (`/roll`,`/guess`,`/vote`)
* Port to Go for learning purposes
* REST API version
* Dockerization
* Optional UI in React

## Protocol Overview
Simple line-base protocol
* Text messages end with `\n`
* Commands start with `/`
* Supported commands (planned):
    * `/rooms` -> list available rooms
    * `/join <room>` -> join a specific room
    * `/quit` -> disconnect

## Build Instructions
### Compile
`make all`

### Run the server
`make run` 

### Clean build artifacts
`make`


## Project Structure
```bash
├── app/
│   └── main.c              # Application entry point
├── docs/
│   ├── architecture.md     # Server architecture overview
│   └── standards.md        # Internal conventions & templates
├── src/
│   ├── app.c               # Application logic (core implementation)
│   ├── server.c            # Server Logic (core implementation)
│   └── server.h            # Prototypes and public structures
├── test/
├── NOTES.md                # Personal notes, sketches, ideas
├── PROJECT_LOG.md          # Daily progress log
├── README.md               # Project documentation (this file)
├── Makefile                # Build automation
└── .gitignore             
```

## Architecture (High-Level Overview)
### Server
* Opens a TCP socket
* Accepts multiple clients
* Stores clients in a list
* Uses `select()` or threads to handle concurrency
* Broadcasts messages or processes commands

### Client
* Connects to server
* Sends user input
* Receive and print messages from others

## Roadmap
### Phase A: Core in C
1. Multi-client TCP Chat
2. Rooms
3. Commands & mini-protocol
4. Minigames
5. Refactor + stability pass

### Phase B: Migration to Go
6. Port logic to Go
7. Build REST API
8. Dockerize

### Phase C: UI
9. Optional React Interface
10. Full documentation + tests

## Testing
Tests will live in `test/` and will include:
* connection tests
* broadcast behavior
* command parsing
* room handling

Run (when available): `make test`

## Contributing
This is a learn-oriented project.
If you want to contribute, please follow:
* `standards.md` (commit style, formatting, guidelines)
* Clean and documented C code
* Small, focused pull requests

## Project Status
Day 1: repository created. Project currently in planning and environment setup stage.