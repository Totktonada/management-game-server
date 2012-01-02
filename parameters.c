#include "parameters.h"

void process_cmd_line_parameters (server_info *sinfo,
    char **argv)
{
    parameters_parser_state state = PARAM_P_ST_START;
    int request_for_next_arg = 0;

    sinfo->expected_clients = -1;

    while (*argv != NULL) {
        switch (state) {
        case PARAM_P_ST_START:
            if (STR_EQUAL (*argv, "--port")
                || STR_EQUAL (*argv, "-p"))
            {
                state = PARAM_P_ST_PORT;
                request_for_next_arg = 1;
            } else if (STR_EQUAL (*argv, "--clients")
                || STR_EQUAL (*argv, "-c"))
            {
                state = PARAM_P_ST_CLIENTS;
                request_for_next_arg = 1;
            } else {
                state = PARAM_P_ST_ERROR;
            }
            break;
        case PARAM_P_ST_PORT:
            sinfo->listening_port = atoi (*argv);
            if (sinfo->listening_port == 0) {
                state = PARAM_P_ST_ERROR;
            } else {
                state = PARAM_P_ST_START;
                request_for_next_arg = 1;
            }
            break;
        case PARAM_P_ST_CLIENTS:
            sinfo->expected_clients = atoi (*argv);
            if (sinfo->expected_clients == 0) {
                state = PARAM_P_ST_ERROR;
            } else {
                state = PARAM_P_ST_START;
                request_for_next_arg = 1;
            }
            break;
        case PARAM_P_ST_ERROR:
            fprintf (stderr, "Wrong parameters!\n");
            exit (ES_WRONG_PARAM);
            break;
        }

        if (request_for_next_arg) {
            ++argv;
            request_for_next_arg = 0;
        }
    } /* while */

    if (sinfo->expected_clients == -1) {
        fprintf (stderr, "Parameter --clients [number]");
        fprintf (stderr, " (-c [number]) must be exist.\n");
        exit (ES_WRONG_PARAM);
    }
}
