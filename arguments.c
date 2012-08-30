#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "arguments.h"
#include "typedefs.h"
#include "utils.h"

#define DEFAULT_SERVER_PORT 37187

/* -1 is without limitation. */
#define DEFAULT_MAX_CLIENTS -1

/* In seconds. */
#define DEFAULT_TIME_BEFORE_ROUND 60

typedef enum arguments_parser_state {
    ARGS_P_ST_START,
    ARGS_P_ST_IP,
    ARGS_P_ST_PORT,
    ARGS_P_ST_MAX,
    ARGS_P_ST_TIME,
    ARGS_P_ST_ERROR
} arguments_parser_state;

void process_arguments(arguments_t *arguments, const char **argv)
{
    arguments_parser_state state = ARGS_P_ST_START;
    uint request_for_next_arg = 0;

    /* Defaults */
    arguments->server_ip = NULL;
    arguments->server_port = DEFAULT_SERVER_PORT;
    arguments->max_clients = DEFAULT_MAX_CLIENTS;
    arguments->time_before_round = DEFAULT_TIME_BEFORE_ROUND;

    while (*argv != NULL) {
        switch (state) {
        case ARGS_P_ST_START:
            if (STR_EQUAL(*argv, "--port")
                || STR_EQUAL(*argv, "-p"))
            {
                state = ARGS_P_ST_PORT;
                request_for_next_arg = 1;
            } else if (STR_EQUAL(*argv, "--max")
                || STR_EQUAL(*argv, "-m"))
            {
                state = ARGS_P_ST_MAX;
                request_for_next_arg = 1;
            } else if (STR_EQUAL(*argv, "--time")
                || STR_EQUAL(*argv, "-t"))
            {
                state = ARGS_P_ST_TIME;
                request_for_next_arg = 1;
            } else {
                state = ARGS_P_ST_IP;
            }
            break;
        case ARGS_P_ST_IP:
            if (arguments->server_ip == NULL) {
                arguments->server_ip = *argv;
                state = ARGS_P_ST_START;
                request_for_next_arg = 1;
            } else {
                state = ARGS_P_ST_ERROR;
            }
            break;
        case ARGS_P_ST_PORT:
            arguments->server_port = atoi(*argv);
            if (arguments->server_port == 0) {
                state = ARGS_P_ST_ERROR;
            } else {
                state = ARGS_P_ST_START;
                request_for_next_arg = 1;
            }
            break;
        case ARGS_P_ST_MAX:
            arguments->max_clients = atoi(*argv);
            if (arguments->max_clients == 0) {
                state = ARGS_P_ST_ERROR;
            } else {
                state = ARGS_P_ST_START;
                request_for_next_arg = 1;
            }
            break;
        case ARGS_P_ST_TIME:
            arguments->time_before_round = atoi(*argv);
            if (arguments->time_before_round == 0
                && !STR_EQUAL(*argv, "0"))
            {
                state = ARGS_P_ST_ERROR;
            } else {
                state = ARGS_P_ST_START;
                request_for_next_arg = 1;
            }
            break;
        case ARGS_P_ST_ERROR:
            fprintf(stderr, "Wrong arguments!\n");
            exit(ES_WRONG_ARGS);
            break;
        }

        if (request_for_next_arg) {
            ++argv;
            request_for_next_arg = 0;
        }
    } /* while */
}
