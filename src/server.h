#ifndef SERVER_H
#define SERVER_H

#include <unistd.h> // size_t

#define USERNAME_MAX 32
#define INBUF_SIZE 1024

/* Protocol semantics are fixed once the
first non-HELLO command is processed. */
typedef enum {
    PROTO_UNDECIDED,
    PROTO_LEGACY,
    PROTO_NEGOTIATED
} protocol_mode_t;

typedef struct Client
{
    int socket;  // -1 means unused
    int room_id; // -1 if not in a room
    int index;   // owned identity
    protocol_mode_t proto_mode;
    char username[USERNAME_MAX];
    char inbuf[INBUF_SIZE];
    size_t inbuf_len;
} Client;

// Public server functions
void start_server(const char *socket_path);

#endif // SERVER_H