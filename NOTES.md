## 0. Filosofía

> GitHub no es un “museo de proyectos terminados”, es el historial de tu proceso.

---

## 1. Ideas de funcionalidades

* Implementar salas
* Diseñar un protocolo simple
* Mini-juegos:
  * tirar dado/s
  * número secreto
  * consumidor/productor (lavarropas)
  * simular una pequeña sociedad
* Futuro: 
  * modo /brainstorm actualiza ordenando chat (a decidir)
  * migrar a Go

---

## 2. Cosas para investigar

* Frameworks de testing en C (CUnit, Unity, Criterion)
* Seguridad básica / hardening
* Graceful client disconnects
* Señales y manejo de procesos (`sigaction`: `signal` is deprecated)
* `epoll` como alternativa escalable a `select`

---

## 3. Arquitectura futura
Detailed and authoritative architecture diagrams live in /docs/architecture.md.
This section tracks future diagram ideas only.

* ASCII diagrams are perfect for /docs/architecture.md v1 (many senior engineers do exactly that)
    * Keep them conceptual
    * Don’t try to be “pretty”
    * One diagram per idea
  * Later (much later), you can convert to Mermaid or draw.io if you want.

### Diagramas pendientes
* Flujo cliente-servidor
* Diagrama de componentes
  * server.c
  * gestor de clientes
  * futuro: gestor de salas
  * futuro: dispatcher de mensajes
  * futuro: parser del protocolo

* Especificación formal del protocolo
  * formato de mensaje
  * comandos
  * errores
  * estados
* Decisiones técnicas futuras
  * buffers
  * non-blocking I/O
  * escalabilidad

---

## 4. Roadmap

### **Fase A — Base técnica en C**

#### 1. Chat con sockets stream simple

* [x] Chat con sockets stream
* [x] soporte para múltiples clientes
* [x] `select()` o threads
* seguridad mínima (validación de input básica, whitelist)

#### 2. Salas + comandos

* [x] estructuras de datos (`Client`, lista dinámica, diccionario de salas)
* sincronización
* comandos (`/join`, `/rooms`, etc.)

#### 3. Mini-juegos

* /roll     (tirar dado)
* /guess    (adivinar número)
* /vote     (decisiones compartidas)
* parser de comandos > if (buffer[0] == '/' )


### **Fase B — Migración a Go**

4. Migrar lógica principal (Chat TCP)
5. Convertir en API REST
6. Dockerizar (arrancar viendo tema de contenedores de Sistemas Operativos y su taller)
7. Agregar auth básica
8. Tests
9. Documentación sólida
10. UI en React (Fase C)