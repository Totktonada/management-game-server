#ifndef GAME_H_SENTRY
#define GAME_H_SENTRY

#ifndef DAEMON
#include <stdio.h>
#endif
#include <unistd.h>
#include "parser.h"
#include "utils.h"

typedef enum game_state {
    G_ST_WAIT_USERS
    /* TODO */
} game_state;

typedef struct game_data {
    unsigned int users_count;
    game_state state;
    unsigned int step;
} game_data;

typedef struct user_game_data {
    char *nick;
    unsigned int money;
    unsigned int raw_count;
    unsigned int prod_count;
    unsigned int factory_count;
} user_game_data;

void new_game_data (game_data *gdata);
void new_user_game_data (user_game_data *user_gdata);
#ifndef DAEMON
void print_cmd (command *cmd);
#endif
void execute_cmd (game_data *gdata,
    user_game_data *user_gdata,
    int write_fd, command *cmd);


#endif
