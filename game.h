#ifndef GAME_H_SENTRY
#define GAME_H_SENTRY

#ifndef DAEMON
#include <stdio.h>
#endif
#include <unistd.h>
#include <string.h>
#include "parser.h"
#include "utils.h"
#include "main.h"

#define RAW_EXPENSES 300
#define PROD_EXPENSES 500
#define FACTORY_EXPENSES 1000

#define MAKE_PROD_COST 2000
#define MAKE_FACTORY_FIRST_HALF 2500
#define MAKE_FACTORY_SECOND_HALF 2500

#if 0
typedef enum market_request_type {
    BUY_RAW_REQUEST,
    SELL_PROD_REUEST
} market_request_type;
#endif

void new_game_data (server_info *sinfo);
void new_client_game_data (client_info *client);

#ifndef DAEMON
void print_cmd (command *cmd);
#endif

void execute_cmd (server_info *sinfo,
    client_info *client, command *cmd);

int game_process_new_client (server_info *sinfo);
void game_process_next_step (server_info *sinfo);

#endif
