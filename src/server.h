#ifndef SERVER_H
#define SERVER_H

#include <sys/un.h>

#define MAX_CLIENTS 10
#define USERNAME_MAX 32
#define SOMAXCONN 16
#define BUFFER_SIZE 512

typedef struct
{
    int socket;  // -1 means unused
    int room_id; // -1 if not in a room
    char username[USERNAME_MAX];
} Client;

// Public server functions
void start_server(const char *socket_path);
void init_clients(void);

//_Helpers
int find_free_client(void);
int find_client_by_fd(int fd);
void client_remove(int idx);

void broadcast_message(const char *msg);

#endif // SERVER_H