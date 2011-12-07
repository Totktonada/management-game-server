#ifndef PARAMETERS_H_SENTRY
#define PARAMETERS_H_SENTRY

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "main.h"

/* Exit status, if user type wrong
 * command line parameters. */
#define ES_WRONG_PARAM 2

#define STR_EQUAL(str1, str2) (strcmp ((str1), (str2)) == 0)

typedef enum parameters_parser_state {
    PARAM_P_ST_START,
    PARAM_P_ST_PORT,
    PARAM_P_ST_ERROR
} parameters_parser_state;

#if 0
typedef parameters_parser_info {
    parameters_parser_state state;
    server_info *sinfo;
} parameters_parser_info;
#endif

void process_cmd_line_parameters (server_info *sinfo,
    char **argv);

#endif
