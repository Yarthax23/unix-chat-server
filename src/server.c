#include "server.h" //  start_server
#include <unistd.h> //  unlink
#include <stdio.h>  //  printf
#include <stdlib.h> //  EXIT tags
#include <sys/socket.h> // socket

static int server_socket;
static struct sockaddr_un server_addr;
// static int client_socket;              // Not scalable
// static struct sockaddr_un client_addr; // Not scalable

static Client clients[MAX_CLIENTS];
void init_clients(void){
    for(int i = 0; i < MAX_CLIENTS; i++){
        clients[i].socket = -1;
        clients[i].room_id = -1;
        clients[i].username[0] = '\0';
    }
}

void start_server(const char *socket_path){
    // Validate input
    if (strlen(socket_path) >= sizeof(server_addr.sun_path)) {
        fprintf(stderr, "Socket path too long\n");
        exit(EXIT_FAILURE);
    }

    // Prepare address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    snprintf(server_addr.sun_path, 
             sizeof(server_addr.sun_path),
             "%s",
             socket_path);               
    
    // Remove stale socket file
    unlink(server_addr.sun_path);

    // Create socket
    server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    if (bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 1) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("[server] Waiting for connection...\n");
}
