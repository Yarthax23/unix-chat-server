#include "server.h"  // start_server
#include "grammar.h" // handle_command

#include <stdio.h>      // printf
#include <stdlib.h>     // EXIT tags
#include <string.h>     // memset, strlen
#include <sys/socket.h> // socket
#include <sys/select.h> // fd_set (select)
#include <sys/un.h>     // sockaddr_un

static int server_socket;
static struct sockaddr_un server_addr;
static Client clients[MAX_CLIENTS];

static fd_set readfds;
int max_fd;

void start_server(const char *socket_path)
{
    // Validate input
    if (strlen(socket_path) >= sizeof(server_addr.sun_path))
    {
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
    if (server_socket == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, SOMAXCONN) == -1)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    clients_init();
    printf("[server] Waiting for connection...\n");
    while (1)
    {
        FD_ZERO(&readfds);

        FD_SET(server_socket, &readfds);
        max_fd = server_socket;

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i].socket != -1)
            {
                FD_SET(clients[i].socket, &readfds);
                if (clients[i].socket > max_fd)
                    max_fd = clients[i].socket;
            }
        }

        int ready = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (ready == -1)
        {
            perror("select");
            // Allows control and cleanup
            break;
        }

        //  New Client
        if (FD_ISSET(server_socket, &readfds))
        {
            int idx = find_free_client();
            if (idx == -1)
            {
                // Server full: accept & inmediately close
                int tmp = accept(server_socket, NULL, NULL);
                close(tmp);
            }
            else
            {
                clients[idx].socket = accept(server_socket, NULL, NULL);
                if (clients[idx].socket == -1)
                {
                    perror("accept");
                    continue;
                };
            }
        }

        //  Existing Client sends data
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            Client *c = &clients[i];

            // Slot unused/inactive
            if (c->socket == -1)
                continue;

            if (!FD_ISSET(c->socket, &readfds))
                continue;

            ssize_t n = recv(c->socket,
                             c->inbuf + c->inbuf_len,
                             INBUF_SIZE - c->inbuf_len,
                             0);
                             
            // Client closed or error
            if (n <= 0)
            {
                if (n < 0)
                    perror("recv");
                client_remove(c);
                continue;
            }

            // Handle client
            c->inbuf_len += n;

            // Overflow check
            if (c->inbuf_len == INBUF_SIZE)
            {
                if (!memchr(c->inbuf, '\n', c->inbuf_len))
                {
                    fprintf(stderr, "[server] Client %d buffer overflow\n", i);
                    client_remove(c);
                    continue;
                }
            }

            // Parsing
            while (1)
            {
                char *nl = (char *)memchr(c->inbuf, '\n', c->inbuf_len);
                if (!nl)
                    break;

                size_t msg_len = (size_t)(nl - c->inbuf);

                // Strip CR if present
                if (msg_len > 0 && c->inbuf[msg_len - 1] == '\r')
                    msg_len--;

                // Extract msg & make it null-terminated for convenience
                char msg[INBUF_SIZE];
                memcpy(msg, c->inbuf, msg_len);
                msg[msg_len] = '\0';

                // Process msg
                printf("[server] Client %d says: %s\n", i, msg);
                int old_room = c->room_id;
                command_result res = handle_command(c, msg, msg_len);

                switch (res)
                {
                case CMD_DISCONNECT:
                    client_remove(c);
                    broadcast_leave(old_room, c);
                    goto next_client;

                case CMD_JOIN_ROOM:
                    broadcast_join(c->room_id, c);
                    break;

                case CMD_LEAVE_ROOM:
                    broadcast_leave(old_room, c);
                    break;

                case CMD_BROADCAST_MSG:
                    broadcast_room(c->room_id, c, msg, msg_len);
                    break;
                }

                // Remove processed bytes (+1 for '\n')
                size_t remaining = c->inbuf_len - (nl - c->inbuf + 1);
                memmove(c->inbuf, nl + 1, remaining);
                c->inbuf_len = remaining;
            }
        next_client:
        }
    }

    close(server_socket);
    unlink(server_addr.sun_path);
}

int find_free_client(void)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].socket == -1)
            return i;
    }
    return -1;
}

int find_client_by_fd(int fd)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].socket == fd)
            return i;
    }
    return -1;
}

void clients_init(void)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        client_init(&clients[i], i);
    }
}

void client_init(Client *c, int idx)
{
    memset(c, 0, sizeof(*c));
    c->socket = -1;
    c->room_id = -1;

    snprintf(c->username,
             USERNAME_MAX,
             "Client %d",
             idx);
}

void client_remove(Client *c)
{
    if (!c || c->socket != -1)
    {
        close(c->socket);
    }
    client_init(c, find_client_by_fd(c->socket));
}

void broadcast_join(int room_id, Client *c)
{
    (void)room_id;
    (void)c;
};
void broadcast_leave(int room_id, Client *c)
{
    (void)room_id;
    (void)c;
};
void broadcast_room(int room_id, Client *sender, const char *msg, size_t len)
{
    (void)room_id;
    (void)sender;
    (void)msg;
    (void)len;
};
void broadcast_msg(int room_id, Client *sender, const char *msg, size_t len)
{
    (void)room_id;
    (void)sender;
    (void)msg;
    (void)len;
};