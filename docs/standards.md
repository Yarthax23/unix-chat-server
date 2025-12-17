# Referencia
README.md   -> lo que ve alguien por primera vez (para el visitante)  
* Qué es este proyecto?
    * Cómo funciona la estructura básica del protocolo (el mensaje entre cliente y servidor, muy resumido)
    * Saber cómo contribuir (si aplica)
* Cómo se ejecuta?
    * Cómo compilar, cómo correr
* En qué estado está?
* Decisiones técnicas importantes
* Problemas y soluciones

NOTES.md    -> tu cerebro volcado ahí adentro
* Ideas
* Dudas
* Cosas para investigar
* Decisiones sin cerrar
* Cosas para revisar
* Mini-roadmap
* Futuros features
* Links random
* Recordatorios
* Brain dump personal
* NO incluir doc formal ni convenciones permanentes del repo

LOG.md      -> el diario del proyecto
* lo que hiciste cada día
* qué aprendiste
* qué rompiste/arreglaste
* mini retrospectivas
* proximos pasos

```markdown
# Project Log

## YYYY-MM-DD – Title (optional)

### Summary
- Breve descripción de qué hice hoy y por qué (2–4 líneas).

### Decisions
- Decisiones relevantes tomadas hoy y sus razones.

### Added
- Nuevos archivos, componentes, funciones o features.

### Changed
- Cambios en la estructura, refactors, reorganización, mejoras.

### Fixed
- Bugs o errores corregidos.

### Removed
- Elementos eliminados del proyecto y motivo.

### Learnings
- Qué aprendí hoy que vale la pena registrar

### Next steps
- Qué dejo preparado para la próxima sesión

### Notes (opcional)
- Contexto adicional, dudas abiertas, exploraciones laterales, etc.
```

DOCS       -> Si el proyecto crece
* descripción detallada del protocolo
* arquitectura del servidor
* diagramas
* justificativos técnicos

## Conventional Commits:
-feat: → agregaste funcionalidad nueva para el usuario
-fix: → corregiste un bug
-refactor: → cambiaste código sin agregar funcionalidad
-chore: → cambios menores / mantenimiento
-perf: → mejoras de rendimiento
-style: → cambios de formato (tabs, espacios, etc.)
-docs: → documentación
-build: → cambios que afectan el sistema de build (Makefile, cabal, deps)
-test: → agregaste/modificaste tests

## C Module Design Rules
1. **File‑Local Scope**
    * Any variable or function used only within a single `.c` file **MUST** be declared `static`.

2. **Explicit Shared State**
    * All shared or long‑lived state **MUST** be grouped into explicit structures.
    * Global variables are forbidden unless they represent immutable configuration.

3. **Header File Discipline**
    * Header files (`.h`) **MUST NOT** define storage or state — only types, macros, and function prototypes.
    * Headers **MAY ONLY** contain:
        * Type definitions
        * Macros
        * Function prototypes

4. **Domain Ownership**
    * Each conceptual domain owns its data structures (e.g., the server module defines `Client`).

5. **Design Priority**
    * Prefer **simple, functional, and correct** designs first.
    * Flexibility, abstraction, and scalability are **secondary** concerns and should be introduced only when justified.

6. **Security Boundary Clarity**
    * Code **MUST NOT** rely on struct layout, padding, or field ordering for security.

7. **Golden Networking Rule**
    * Never assume one `recv` = one message, view protocol.

8. **No Dead State**
    * If a field has no consumer, it does not belong in the struct.

9. **Prototype Functions**
    * Every function declaration must fully specify its parameters. No exceptions.

10. **Object Lifetime Safety**
    Never remove or invalidate an object while still executing code that assumes it exists.
    In event-driven systems, signal intent (e.g., disconnect) and perform teardown only at safe boundaries.
