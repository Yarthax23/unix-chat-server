# Chat TCP en C
Proyecto personal para aprender sockets, sincronización y bases de programación de sistemas.

## Descripción
Un chat compartido en un servidor donde cada cliente puede mandar cualquier mensaje, debe recibir el mensaje de otros clientes y el servidor debe soportar un máximo de clientes al mismo tiempo. Los clientes se pueden conectar y desconectar cuando quieran.

## Protocolo (resumen)
- Mensajes de texto terminados en '\n'
- Comando /salas para listar salas
- Comando /join <sala> para unirse

## Cómo compilar
make all

## Cómo ejecutar
make run

## Estructura del Proyecto
```
├── app/
│   └── main.c              # Punto de entrada de la aplicación
├── docs/
│   └── standards.md        # Referencias, templates
├── src/
│   └── app.c               # Aplicación interactiva (completo)
├── test/
├── NOTES.md                # Diario personal, borrador
├── PROJECT_LOG.md          #
├── README.md               # Este archivo
├── Makefile                # Automatización de tareas
└── .gitignore             
```


## Estado
Dia 1: creación del repositorio. Proyecto en planificación.