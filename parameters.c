#include "parameters.h"

void process_cmd_line_parameters(server_info *sinfo, char **argv)
{
    parameters_parser_state state = PARAM_P_ST_START;
    int request_for_next_arg = 0;

    sinfo->max_clients = -1; /* by default - without limitation */
    sinfo->server_ip = NULL; /* by default - "127.0.0.1", see
                                after while */

    while (*argv != NULL) {
        switch (state) {
        case PARAM_P_ST_START:
            if (STR_EQUAL(*argv, "--port")
                || STR_EQUAL(*argv, "-p"))
            {
                state = PARAM_P_ST_PORT;
                request_for_next_arg = 1;
            } else if (STR_EQUAL(*argv, "--max")
                || STR_EQUAL(*argv, "-m"))
            {
                state = PARAM_P_ST_MAX;
                request_for_next_arg = 1;
            } else {
                state = PARAM_P_ST_IP;
            }
            break;
        case PARAM_P_ST_IP:
            if (sinfo->server_ip == NULL) {
                sinfo->server_ip = *argv;
                request_for_next_arg = 1;
            } else {
                state = PARAM_P_ST_ERROR;
            }
            break;
        case PARAM_P_ST_PORT:
            sinfo->listening_port = atoi(*argv);
            if (sinfo->listening_port == 0) {
                state = PARAM_P_ST_ERROR;
            } else {
                state = PARAM_P_ST_START;
                request_for_next_arg = 1;
            }
            break;
        case PARAM_P_ST_MAX:
            sinfo->max_clients = atoi(*argv);
            if (sinfo->max_clients == 0) {
                state = PARAM_P_ST_ERROR;
            } else {
                state = PARAM_P_ST_START;
                request_for_next_arg = 1;
            }
            break;
        case PARAM_P_ST_ERROR:
            fprintf(stderr, "Wrong parameters!\n");
            exit(ES_WRONG_PARAM);
            break;
        }

        if (request_for_next_arg) {
            ++argv;
            request_for_next_arg = 0;
        }
    } /* while */

    if (sinfo->server_ip == NULL) {
        sinfo->server_ip = "127.0.0.1";
    }
}
