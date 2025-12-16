#ifndef GRAMMAR_H
#define GRAMMAR_H

#include <unistd.h> // size_t

typedef enum
{
    CMD_NICK,
    CMD_JOIN,
    CMD_LEAVE,
    CMD_MSG,
    CMD_QUIT,
    CMD_INVALID
} command_type;

struct Client;
void handle_command(struct Client *c, const char *msg, size_t len);



#endif // GRAMMAR_H