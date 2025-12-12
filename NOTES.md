GitHub no es un "museo de proyectos terminados".
Es un historial de tu proceso.


Ideas:
* implementar salas
* pensar un pequeño protocolo
* mini-juegos: tirar dado/s, número secreto
* después migrar a Go

Cosas para investigar:
* Convención de Commits. https://www.conventionalcommits.org/es/v1.0.0-beta.3 ; https://github.com/KarmaPulse/git-commit-message-conventions ; https://gist.github.com/qoomon/5dfcdf8eec66a051ecd85625518cfd13 

Dudas: 
* revisar qué modelo de sincronización usar
    * opc A: select()
    * opc B: threads + mutex <duda : es lo mismo que fork() ? -> wait(), signal vs sigaction>
    * opc C (avanzada): epoll (Linux)

Futuros features:

Links:

Notas personales:

---



[] Proyecto 1: Chat TCP simple en C

*Fase A Base técnica en C (2-6 semanas)*
1. Chat TCP simple en C
    * sockets
        * socket(), bind(), listen(), accept(), connect(), recv()/send()
    * cliente/servidor
    * enviar/recibir texto
    * seguridad mínima (validación de input básica)
    * manejo de varios clientes con select/poll o threads
2. Agregar salas + interacción
    * sincronización (mutex, select, poll, epoll)
    * manejo de estructuras de datos
        * typedef struct {
            int socket;
            char username[32];
            int room_id;
        } Client
        * lista dinámica de clientes
        * diccionario de salas (hash map simple o array)
    * protocolito simple de mensajes
3. Mini-juegos dentro del chat
    * adivinar número
    * tirar dados
    * decisiones compartidas
    * empezar a pensar "comandos" dentro del chat > if (buffer[0] == '/')
        * /roll
        * /guess 1-10
        * /vote start

*FASE B Migración a Go (cuando ya fluya C)*  
4. Migrar la lógica principal a Go  
5. Convertirlo en API Rest simple
    * Qué es una API ? Qué es hacer APIs en Go? Qué es una API Rest?  
6. Dockerizar
    * Qué es dockerizar? -> meter programa en contenedor aislado (arrancar viendo tema de contenedores de Sistemas Operativos y su taller)  
7. Agregar auth básica  
8. Tests  
9. Documentación sólida  
10. Más adelante: UI en React (Fase C)
