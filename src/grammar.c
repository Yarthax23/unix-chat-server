#include "grammar.h"
#include "server.h" // Client

#include <stdlib.h> // EXIT
#include <string.h> // mem*, str*

typedef struct
{
    const char *name;
    size_t len;
    command_type type;
} command_spec;

static const command_spec command_table[] = {
    {"NICK", 4, CMD_NICK},
    {"JOIN", 4, CMD_JOIN},
    {"LEAVE", 5, CMD_LEAVE},
    {"MSG", 3, CMD_MSG},
    {"QUIT", 4, CMD_QUIT}};

static const size_t command_table_len =
    sizeof(command_table) / sizeof(command_table[0]);

static command_type parse_command(const char *msg,
                                  size_t len,
                                  const char **args,
                                  size_t *args_len);
static void handle_nick(Client *c, const char *args, size_t args_len);
static void handle_join(Client *c, const char *args, size_t args_len);
static void handle_leave(Client *c, const char *args, size_t args_len);
static void handle_msg(Client *c, const char *args, size_t args_len);


command_result handle_command(Client *c, const char *msg, size_t len)
{
    const char *args;
    size_t args_len;

    command_type cmd = parse_command(msg, len, &args, &args_len);

    switch (cmd) {
    case CMD_NICK:
        handle_nick(c, args, args_len);
        break;
    case CMD_JOIN:
        handle_join(c, args, args_len);
        break;
    case CMD_LEAVE:
        handle_leave(c, args, args_len);
        break;
    case CMD_MSG:
        handle_msg(c, args, args_len);
        break;
    case CMD_QUIT:
    default:
        return CMD_DISCONNECT;    
    }

    return CMD_OK;
}

static command_type parse_command(const char *msg, size_t len,
                           const char **args, size_t *args_len)
{
    *args = NULL;
    *args_len = 0;
    // Extract command token
    const char *space = memchr(msg, ' ', len);
    size_t cmd_len = space ? (size_t)(space - msg) : len;

    for (size_t i = 0; i < command_table_len; i++)
    {
        const command_spec *spec = &command_table[i];

        // Validate command name
        if (cmd_len == spec->len &&
            memcmp(msg, spec->name, spec->len) == 0)
        {
            if (space)
            {
                *args = space + 1;
                *args_len = len - (cmd_len + 1);
            }
            return spec->type;
        }
    }

    return CMD_INVALID;
}

static void handle_nick(Client *c, const char *args, size_t args_len){
};
static void handle_join(Client *c, const char *args, size_t args_len){
};
static void handle_leave(Client *c, const char *args, size_t args_len){
};
static void handle_msg(Client *c, const char *args, size_t args_len){
};

