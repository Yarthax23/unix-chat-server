## Execution Ordering Audit (v0.1.x)

This document records a manual audit of execution ordering
between grammar, server intent handling, and broadcast delivery.

Confirmed ordering invariants:

* Grammar performs validation before intent emission
* Server applies state mutations before broadcasting events
* Broadcast helpers do not mutate server state
* All protocol-visible output originates in server execution

Detailed step-by-step execution traces are deferred until command interactions become non-trivial (e.g., minigames)