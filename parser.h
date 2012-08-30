#ifndef PARSER_H_SENTRY
#define PARSER_H_SENTRY

#include "lexer.h"
#include "typedefs.h"

typedef struct command_t {
    command_kind type;
    value_of_lex value;
    value_of_lex value2;
} command_t;

typedef enum parser_state {
    P_ST_START,
    P_ST_PROCESS_ARG1,
    P_ST_PROCESS_ARG2,
    P_ST_EXPECT_CMD_NAME,
    P_ST_EXPECT_EOLN,
    P_ST_EXPECT_OPTIONAL_STR,
    P_ST_EXPECT_TWO_NUMBERS,
    P_ST_EXPECT_ONE_NUMBER,
    P_ST_WRONG,
    P_ST_EMPTY,
    P_ST_SKIP,
    P_ST_PROTOCOL_PARSE_ERROR
} parser_state;

typedef struct parser_t {
    parser_state state;
    lexer_t lexer;
    lexeme *cur_lex;
    command_t tmp_cmd;
    uint request_for_lex:1;
    uint cur_lex_data_used:1;
} parser_t;

void new_parser(parser_t *parser);

void put_new_data_to_parser(parser_t *parser,
    char *read_buffer, int read_available);

command_t *get_command(parser_t *parser);

void destroy_command(command_t *cmd);

#endif
