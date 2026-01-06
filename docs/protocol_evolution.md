## Overview
This document defines how the unix-chat protocol evolves without breaking validated semantics.
* v0.1.x is closed, future changes are additive or explicitly divergent.
* Current version defines a semantic contract.
    * It MAY NOT reinterpret existing commands
    * It MAY NOT change ordering guarantees

## The HELLO command (Conceptual)

* A declarative protocol preface.
* Not authentication.
* Not negotiation.
* Not a request for capability grants.

`HELLO` constrains the client; it does not empower it. A capability is never "permission". The client agrees to be held to these rules.
* Why is this called `HELLO`? Because it’s the first thing a client may say.

---

## Capabilities vs commands
They are orthogonal, not interchangeable.

### Commands
* Define syntax
* Appear on the wire
* Are parsed by grammar
* Always exist once introduced

### Capabilities
* Define semantics
* Never appear outside `HELLO`
* Gate interpretation, not parsing
* May or may not be used by execution

A command can exist without a capability.
A capability can gate multiple commands.

---
## Protocol Lifecycle States

Each client connection progresses through a small, explicit set of protocol lifecycle states. These states determine which commands are accepted and how protocol semantics are interpreted.

### States
```
CONNECTED
└── PROTO_UNDECIDED
    ├── PROTO_LEGACY
    └── PROTO_NEGOTIATED
```

#### PROTO_UNDECIDED

Initial state for every new connection.

Properties:

* No protocol semantics are yet locked
* The client may send:
  * `HELLO`
  * Any v0.1.x command
* No server-side protocol assumptions are made

Rules:

* `HELLO` is accepted **only** in this state
* Receiving any non-HELLO command triggers a protocol decision

---

#### PROTO_LEGACY

Entered when the first non-HELLO command is processed and no prior
`HELLO` was received.

Properties:

* Protocol semantics are fixed to v0.1.x
* All behavior follows the validated baseline
* Capabilities are empty and ignored

Rules:

* `HELLO` is invalid and causes disconnect
* No protocol negotiation is possible

---

#### PROTO_NEGOTIATED

Entered when a valid `HELLO` was received and the first non-HELLO command
is processed.

Properties:

* Protocol semantics are fixed according to:

  * declared protocol version
  * declared capabilities
* Future behavior may be gated on capabilities

Rules:

* `HELLO` is invalid and causes disconnect
* Unknown capabilities are ignored unless explicitly defined otherwise

---

#### Protocol Lock-In Invariant

Protocol semantics are fixed at the moment the first non-HELLO command is processed. Once a client transitions out of PROTO_UNDECIDED, the protocol mode MUST NOT change for the lifetime of the connection.

This invariant is enforced by execution logic, not grammar.

---

### Why this matters (compression)

With this lifecycle:

* `HELLO` semantics become trivial
* Legacy behavior requires zero duplication
* Future features become local
* No goto, no mode loops, no special casing

Minigames later will simply say:

> “This command requires PROTO_NEGOTIATED + capability X”

Nothing leaks backward.

## HELLO Grammar

* HELLO ::= "HELLO" SP version *(SP capability)
* version ::= DIGIT "." DIGIT
* capability ::= UPPERCASE_TOKEN

Properties:
* Line-based
* Deterministic
* Parsed by grammar only
* Emits intent without side effects

Grammar invariants:
* Grammar does not enforce protocol lifecycle rules
* Grammar validation remains unchanged


---

## HELLO Execution Rules

Execution enforces protocol lifecycle constraints.

* `HELLO` is valid only in PROTO_UNDECIDED
* `HELLO` MAY be sent at most once
* `HELLO` after protocol lock-in causes immediate disconnect
* Unsupported protocol versions cause immediate disconnect
* Unknown capabilities are ignored unless explicitly defined otherwise

Protocol decision rules:

* If the first non-HELLO command is processed without a prior `HELLO`,
* the client transitions to PROTO_LEGACY
* If a valid `HELLO` was received before the first non-HELLO command,
the client transitions to PROTO_NEGOTIATED
* Once transitioned, protocol mode MUST NOT change for the lifetime of the connection

### 1. Rules for unknown commands

* Unknown command → disconnect
* Unknown argument shape → disconnect
* Unknown capability → ignore
* Behavior is never inferred

### 2. Rules for adding new commands

Defining capabilities before you need them leads to:

* fake abstraction
* unused surface
* premature branching

Capabilities exist only to justify the shape of `HELLO` and to prevent you from painting yourself into a corner.

You add the first real capability only when:
* A new command family would otherwise require “if legacy do X else Y”.

### 3. Rules for extending existing commands

### 4. What “backward compatible” means in this project
* Minor versions MAY add commands.
* Minor versions MAY add capabilities.
* Minor versions MUST NOT reinterpret existing commands.
* Minor versions MUST NOT change execution ordering guarantees.