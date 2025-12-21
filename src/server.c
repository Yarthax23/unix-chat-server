#include "server.h"  // start_server
#include "grammar.h" // handle_command

#include <stdio.h>      // printf
#include <stdlib.h>     // EXIT tags
#include <string.h>     // memset, strlen
#include <sys/socket.h> // socket
#include <sys/select.h> // fd_set (select)
#include <sys/un.h>     // sockaddr_un

#define MAX_CLIENTS 10
#define SERVER_BACKLOG 16

static int server_socket;
static struct sockaddr_un server_addr;
static Client clients[MAX_CLIENTS];

static int find_free_client(void);
static void clients_init(void);
static void client_init(Client *c, int idx);
static void client_remove(Client *c);
static void client_set_username(Client *c, const char *p, size_t len);

static int broadcast_join(int room_id, Client *c);
static int broadcast_leave(int room_id, Client *c);
static int broadcast_quit(int room_id, Client *c);
static int broadcast_room(int room_id, Client *sender, const char *msg, size_t len);

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
    int n = snprintf(server_addr.sun_path,
                     sizeof(server_addr.sun_path),
                     "%s",
                     socket_path);

    if (n < 0 || (size_t)n >= sizeof(server_addr.sun_path))
        // fatal configuration error
        exit(EXIT_FAILURE);

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

    if (listen(server_socket, SERVER_BACKLOG) == -1)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    clients_init();
    printf("[server] Waiting for connection...\n");

    fd_set readfds;
    int max_fd;
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

            ssize_t n_recv = recv(c->socket,
                                  c->inbuf + c->inbuf_len,
                                  INBUF_SIZE - c->inbuf_len,
                                  0);

            // Client closed or error
            if (n_recv <= 0)
            {
                if (n_recv < 0)
                    perror("recv");
                client_remove(c);
                continue;
            }

            // Handle client
            c->inbuf_len += n_recv;

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
                printf("[server] Client %d sent %s\n", i, msg);
                command_action action = handle_command(c, msg, msg_len);

                switch (action.type)
                {
                case CMD_DISCONNECT:
                    if (action.room_id != -1)
                        if (broadcast_quit(action.room_id, c) == -1)
                        {
                            perror("send: quit");
                            exit(EXIT_FAILURE);
                        }
                    client_remove(c);
                    goto next_client;

                case CMD_SET_NICK:
                    client_set_username(c, action.payload, action.payload_len);
                    break;

                case CMD_JOIN_ROOM:
                    if (c->room_id != -1)
                        if (broadcast_leave(c->room_id, c) == -1)
                        {
                            perror("send: leave");
                            exit(EXIT_FAILURE);
                        }
                    c->room_id = action.room_id;
                    if (broadcast_join(action.room_id, c) == -1)
                    {
                        perror("send: join");
                        exit(EXIT_FAILURE);
                    }
                    break;

                case CMD_LEAVE_ROOM:
                    c->room_id = -1;
                    if (action.room_id != -1)
                        if (broadcast_leave(action.room_id, c) == -1)
                        {
                            perror("send: leave");
                            exit(EXIT_FAILURE);
                        }
                    break;

                case CMD_BROADCAST_MSG:
                    if (c->room_id != -1)
                        if (broadcast_room(c->room_id, c, action.payload, action.payload_len) == -1)
                        {
                            perror("room");
                            exit(EXIT_FAILURE);
                        }
                    break;

                case CMD_OK:
                default:
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

static int find_free_client(void)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].socket == -1)
            return i;
    }
    return -1;
}

static void clients_init(void)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        client_init(&clients[i], i);
    }
}

static void client_init(Client *c, int idx)
{
    memset(c, 0, sizeof(*c));
    c->socket = -1;
    c->room_id = -1;
    c->index = idx;

    snprintf(c->username, USERNAME_MAX, "Client %d", idx);
}

static void client_remove(Client *c)
{
    if (!c)
        return;

    if (c->socket != -1)
        close(c->socket);

    client_init(c, c->index);
}

static void client_set_username(Client *c, const char *p, size_t len)
{
    memcpy(c->username, p, len);
    c->username[len] = '\0';
}

static int broadcast_join(int room_id, Client *c)
{
    // Format server message
    const char event[] = "[server] JOIN";
    char buf[sizeof(event) + USERNAME_MAX + 2]; // space + '\n'

    int written = snprintf(buf, sizeof(buf), "%s %s\n", event, c->username);
    if (written < 0 || (size_t)written >= sizeof(buf))
        return -1;

    // Broadcast
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].socket != -1 &&        // closed socket
            clients[i].socket != c->socket && // sender socket
            clients[i].room_id == room_id)    // filter room

            if (send(clients[i].socket, buf, (size_t)written, 0) == -1)
                return -1;
    }
    return 0;
}

static int broadcast_leave(int room_id, Client *c)
{
    // Format server message
    const char event[] = "[server] LEAVE";
    char buf[sizeof(event) + USERNAME_MAX + 2]; // space + '\n'

    int written = snprintf(buf, sizeof(buf), "%s %s\n", event, c->username);
    if (written < 0 || (size_t)written >= sizeof(buf))
        return -1;

    // Broadcast
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].socket != -1 &&        // closed socket
            clients[i].socket != c->socket && // echo sender
            clients[i].room_id == room_id)    // filter room

            if (send(clients[i].socket, buf, (size_t)written, 0) == -1)
                return -1;
    }
    return 0;
}

static int broadcast_quit(int room_id, Client *c)
{
    // Format server message
    const char event[] = "[server] QUIT";
    char buf[sizeof(event) + USERNAME_MAX + 2]; // space + '\n'

    int written = snprintf(buf, sizeof(buf), "%s %s\n", event, c->username);
    if (written < 0 || (size_t)written >= sizeof(buf))
        return -1;

    // Broadcast
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].socket != -1 &&        // closed socket
            clients[i].socket != c->socket && // echo sender
            clients[i].room_id == room_id)    // filter room

            if (send(clients[i].socket, buf, (size_t)written, 0) == -1)
                return -1;
    }
    return 0;
}

static int broadcast_room(int room_id, Client *sender, const char *msg, size_t len)
{
    // Format message `<username>: <payload>\n`
    char buf[len + USERNAME_MAX + 3]; // ':' space + '\n'

    int written = snprintf(buf, sizeof(buf), "%s: %s\n", sender->username, msg);
    if (written < 0 || (size_t)written >= sizeof(buf))
        return -1;

    // Broadcast
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].socket != -1 &&             // closed socket
            clients[i].socket != sender->socket && // echo sender
            clients[i].room_id == room_id)         // filter room

            if (send(clients[i].socket, buf, (size_t)written, 0) == -1)
                return -1;
    }
    return 0;
}