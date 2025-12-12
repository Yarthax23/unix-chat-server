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