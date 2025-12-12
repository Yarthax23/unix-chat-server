## 0. Filosofía

> GitHub no es un “museo de proyectos terminados”, es el historial de tu proceso.

---

## 1. Ideas de funcionalidades

* Implementar salas
* Pensar un pequeño protocolo
* Mini-juegos:

  * tirar dado/s
  * número secreto
  * consumidor/productor (lavarropas)
  * simular una pequeña sociedad
* Futuro: migrar a Go

---

## 2. Cosas para investigar

* Convención de commits (https://www.conventionalcommits.org/es/v1.0.0-beta.3 ; https://github.com/KarmaPulse/git-commit-message-conventions ; https://gist.github.com/qoomon/5dfcdf8eec66a051ecd85625518cfd13)
* Frameworks de testing en C (CUnit, Unity, Criterion)
* Explotación / seguridad básica
* Concurrencia: varios clientes leyendo/escribiendo simultáneamente
* “Graceful client disconnects”
  * cómo detectar desconexión normal (`recv() == 0`)
  * desconexión abrupta (`recv() < 0`, errno=ECONNRESET)
  * remover del array/lista (reajustes/liberar mem)
  * cerrar socket
  * informar al resto (opcional) [server] user Juan disconnected
  * no dejar FD inválidos en `select()`
* Concurrency models & signals
  * `select()` vs threads + mutex
    - Single-threaded, event-driven
    - Easier to maintain for small number of clients
    - No mutex needed if carefully managing structures
    - Limitations in scalability
    vs
    - Each client in separate thread
    - Shared structures require mutex
    - Risk of deadlocks, higher memory usage
    - Conceptually similar to fork() but threads share memory
  * fork() + copy-on-write
    - Each client in a separate process
    - Memory not shared (copy-on-write)
    - Communication via sockets/pipes
    - Less convenient for chat app
  * señales y manejo de procesos (`exit()`, `wait()`, `signal()` deprecated, investigar `sigaction()`)
  * epoll() (Linux, escalable)
    - Scalable alternative to select()
    - Efficient with many clients
    - More complex to implement

---

## 3. Arquitectura futura

(“Detailed architecture diagrams will live in /docs/” se refiere a esta sección)

### 3.1 Diagramas a crear

* **Flujo cliente-servidor**
* **Diagrama de componentes**

  * server.c
  * gestor de clientes
  * gestor de salas
  * dispatcher de mensajes
  * parser del protocolo
* **Secuencias**

  * cómo se procesa `/join`
* **Especificación del protocolo**

  * formato de mensaje
  * comandos
  * errores
  * estados
* **Decisiones técnicas**

  * por qué `select()` y no threads
  * cómo se gestiona memoria
  * política de buffers

---

## 4. Dudas

* ¿Qué modelo de sincronización usar?

  * A: `select()`
  * B: threads + mutex
    * ¿relación con `fork()`?
    * señales vs `pthread_*`
  * C: epoll (Linux)
* Zonas críticas para acceso concurrente

---

## 5. Roadmap

### **Fase A — Base técnica en C**

#### 1. Chat TCP simple

* socket(), bind(), listen(), accept(), connect(), recv(), send()
* soporte para múltiples clientes
* `select()` o threads
* seguridad mínima (validación de input básica, whitelist)

#### 2. Salas + comandos

* sincronización
* estructuras de datos (`Client`, lista dinámica, diccionario de salas)
    * typedef struct {
            int socket;
            char username[32];
            int room_id;
        } Client
* comandos (`/join`, `/rooms`, etc.)

#### 3. Mini-juegos

* /roll     (tirar dado)
* /guess    (adivinar número)
* /vote     (decisiones compartidas)
* parser de comandos > if (buffer[0] == '/' )

---

### **Fase B — Migración a Go**

4. Migrar lógica principal
5. Convertir en API REST
6. Dockerizar (arrancar viendo tema de contenedores de Sistemas Operativos y su taller)
7. Agregar auth básica
8. Tests
9. Documentación sólida
10. UI en React (Fase C)