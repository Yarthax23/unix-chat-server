#ifndef SERVER_H
#define SERVER_H

#include <unistd.h> // size_t

#define MAX_CLIENTS 10
#define USERNAME_MAX 32
#define SOMAXCONN 16
#define INBUF_SIZE 1024

typedef struct
{
    int socket;  // -1 means unused
    int room_id; // -1 if not in a room
    char username[USERNAME_MAX];
    char inbuf[INBUF_SIZE];
    size_t inbuf_len;
} Client;

// Public server functions
void start_server(const char *socket_path);
void clients_init(void);

//_Helpers
int find_free_client(void);
int find_client_by_fd(int fd);
void client_init(Client *c);
void client_remove(int idx);

void broadcast_message(const char *msg);

#endif // SERVER_H