#ifndef PARSER_H_SENTRY
#define PARSER_H_SENTRY

#include <strings.h>
#ifndef DAEMON
#include <stdio.h>
#endif
#include "lexer.h"

/* Case insensitive */
#define STR_EQUAL_CASE_INS(str1, str2) (strcasecmp ((str1), (str2)) == 0)

typedef enum type_of_cmd {
    CMD_EMPTY, /* '\n' */
    CMD_HELP,  /* help [command] */
    CMD_NICK,  /* nick [string] */
    CMD_INCR,  /* incr */
    CMD_SHOW,  /* show */
    CMD_WRONG,
    CMD_PROTOCOL_PARSE_ERROR
} type_of_cmd;

#if 0 /* equal to value_of_lex */
typedef union value_of_cmd {
    int number;
    char *str;
} value_of_cmd;
#endif

typedef struct command {
    type_of_cmd type;
    value_of_lex value;
} command;

typedef enum parser_state {
    P_ST_START,
    P_ST_PROCESS_ARG1,
    P_ST_PROCESS_ARG2,
    P_ST_EXPECT_CMD_NAME,
    P_ST_EXPECT_EOLN,
    P_ST_EXPECT_OPTIONAL_STR,
    P_ST_WRONG,
    P_ST_EMPTY,
    P_ST_SKIP,
    P_ST_PROTOCOL_PARSE_ERROR
} parser_state;

typedef struct parser_info {
    parser_state state;
    lexer_info linfo;
    lexeme *cur_lex;
    command tmp_cmd;
    unsigned int request_for_lex:1;
} parser_info;

void new_parser_info (parser_info *pinfo);
void put_new_data_to_parser (parser_info *pinfo,
    char *read_buffer, int read_available);
command *get_cmd (parser_info *pinfo);
void destroy_cmd (command *cmd);

#endif
