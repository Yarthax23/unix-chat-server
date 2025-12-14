## Architecture Overview

* Single-process, event-driven server
* I/O multiplexing using select()
* Event-driven I/O using `select` instead of `fork`.
* UNIX domain stream sockets
* Client state stored in fixed-size array
* Protocol framing: planned line-based text protocol (\n-terminated)

```
+-----------------------+
|       Server          |
|-----------------------|
|  select() event loop  |
|                       |
|  +-----------------+  |
|  | server_socket   |<-+-- accept()
|  +-----------------+  |
|          |            |
|          v            |
|  +-----------------+  |
|  | clients[] table |  |
|  |  fd -> Client   |  |
|  +-----------------+  |
|          |            |
|          v            |
|   recv / send         |
+-----------------------+
```


## Client lookup strategy: fd → Client

### Strategy 1: array scan + select
* Client clients[MAX_CLIENTS];
* By index, trivial
* By fd, linear O(n) scan: find_clients_by_fd

### Strategy 2 (future): fd-indexed table + epoll
* Client *clients_by_fd[MAX_FDS];
* clients_by_fd[fd]= &clients[i];
* O(1) lookup


## Planned Protocol: Delimiter-based (\n)
> NOTE: This protocol is documented here as a design plan.
> The current implementation still assumes one recv() yields one command.

### Client struct needs a buffer
```c
typedef struct {
    int socket;
    int room_id;
    char username[USERNAME_MAX];

    char inbuf[1024];
    size_t inbuf_len;
} Client;
```

### Receiving data
```c
int n = recv(fd,
             clients[i].inbuf + clients[i].inbuf_len,
             sizeof(clients[i].inbuf) - clients[i].inbuf_len,
             0);

clients[i].inbuf_len += n;
```

### Extract complete lines
```c
char *newline;
while ((newline = memchr(clients[i].inbuf, '\n',
                          clients[i].inbuf_len))) {

    size_t msg_len = newline - clients[i].inbuf + 1;

    char msg[256];
    memcpy(msg, clients[i].inbuf, msg_len);
    msg[msg_len - 1] = '\0'; // remove '\n'

    handle_message(i, msg);

    memmove(clients[i].inbuf,
            clients[i].inbuf + msg_len,
            clients[i].inbuf_len - msg_len);

    clients[i].inbuf_len -= msg_len;
}
```