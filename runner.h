#ifndef RUNNER_H_SENTRY
#define RUNNER_H_SENTRY

#ifndef DAEMON
#include <stdio.h>
#endif
#include <unistd.h>
#include "parser.h"

typedef struct game_data {
    unsigned int counter;
    unsigned int clients_count;
} game_data;

void new_game_data (game_data *gdata);
#ifndef DAEMON
void print_cmd (command *cmd);
#endif
void execute_cmd (game_data *gdata, int write_fd,
    command *cmd);

#endif
