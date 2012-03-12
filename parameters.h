#ifndef PARAMETERS_H_SENTRY
#define PARAMETERS_H_SENTRY

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "main.h"
#include "utils.h"

/* Exit status, if client type wrong
 * command line parameters. */
#define ES_WRONG_PARAM 2

typedef enum parameters_parser_state {
    PARAM_P_ST_START,
    PARAM_P_ST_IP,
    PARAM_P_ST_PORT,
    PARAM_P_ST_MAX,
    PARAM_P_ST_ERROR
} parameters_parser_state;

void process_cmd_line_parameters(server_info *sinfo,
    char **argv);

#endif
