#include "parser.h"
#include "typedefs.h"
#include "utils.h"

#ifndef DAEMON
#include <stdio.h>
#endif

/* Returns:
 * 1, if new lex has been got;
 * 0, otherwise. */
static int parser_get_lex(parser_t *parser)
{
    if (parser->cur_lex != NULL)
        destroy_lex(parser->cur_lex, ! parser->cur_lex_data_used);

    parser->cur_lex_data_used = 0;
    parser->cur_lex = get_lex(&(parser->lexer));

    if (parser->cur_lex == NULL) {
        return 0; /* Request for new data */
    }

    return 1;
}

/* Make new command from parser->tmp_cmd. */
static command_t *new_command(parser_t *parser)
{
    command_t *cmd = (command_t *) malloc(sizeof(command_t));

    cmd->type = parser->tmp_cmd.type;
    cmd->value = parser->tmp_cmd.value;
    cmd->value2 = parser->tmp_cmd.value2;

    return cmd;
}

static command_t *p_st_start(parser_t *parser)
{
    switch (parser->cur_lex->type) {
    case LEX_WORD:
        parser->state = P_ST_EXPECT_CMD_NAME;
        break;
    case LEX_NUMBER:
    case LEX_WRONG_NUMBER:
        parser->state = P_ST_WRONG;
        break;
    case LEX_EOLN:
        parser->state = P_ST_EMPTY;
        break;
    case LEX_PROTOCOL_PARSE_ERROR:
        parser->state = P_ST_PROTOCOL_PARSE_ERROR;
        break;
    }

    return NULL;
}

static command_t *p_st_process_arg1(parser_t *parser)
{
    switch (parser->tmp_cmd.type) {
    /* Commands without arguments. */
    case CMD_CLIENTS:
    case CMD_PLAYERS:
    case CMD_REQUESTS:
    case CMD_MARKET:
    case CMD_TURN:
    case CMD_JOIN:
        parser->state = P_ST_EXPECT_EOLN;
        break;

    /* Commands with optional str argument. */
    case CMD_HELP:
    case CMD_NICK:
        parser->state = P_ST_EXPECT_OPTIONAL_STR;
        break;

    /* Commands with one numerical argument. */
    case CMD_BUILD:
    case CMD_MAKE:
        parser->state = P_ST_EXPECT_ONE_NUMBER;
        break;

    /* Commands with two numerical arguments. */
    case CMD_BUY:
    case CMD_SELL:
        parser->state = P_ST_EXPECT_TWO_NUMBERS;
        break;

    /* Not possible. */
    default:
#ifndef DAEMON
        fprintf(stderr, "Parser: error in line %d", __LINE__);
#endif
        break; /* For avoid compile error with defined DAEMON macro. */
    }

    return NULL;
}

static command_t *p_st_process_arg2(parser_t *parser)
{
    switch (parser->tmp_cmd.type) {
    /* Commands with optional str argument.
     * We already process one argument. */
    case CMD_HELP:
    case CMD_NICK:
    /* Commands with one numerical argument. */
    case CMD_BUILD:
    case CMD_MAKE:
        parser->state = P_ST_EXPECT_EOLN;
        break;

    /* Commands with two numerical arguments. */
    case CMD_BUY:
    case CMD_SELL:
        parser->state = P_ST_EXPECT_ONE_NUMBER;
        break;

    /* Not possible. */
    default:
#ifndef DAEMON
        fprintf(stderr, "Parser: error in line %d", __LINE__);
#endif
        break; /* For avoid compile error with defined DAEMON macro. */
    }

    return NULL;
}

static command_t *p_st_expect_cmd_name(parser_t *parser)
{
    parser->request_for_lex = 1;
    parser->state = P_ST_PROCESS_ARG1;
    parser->tmp_cmd.type = get_command_kind(parser->cur_lex->value.str);

    if (parser->tmp_cmd.type == CMD_WRONG) {
        parser->request_for_lex = 0;
        parser->state = P_ST_WRONG;
    }

    return NULL;
}

static command_t *p_st_expect_eoln(parser_t *parser)
{
    command_t *cmd = NULL;

    switch (parser->cur_lex->type) {
    case LEX_EOLN:
        parser->state = P_ST_START;
        parser->request_for_lex = 1;
        /* parser->tmp_cmd already defined
         * (only type or both type and value). */
        cmd = new_command(parser);
        break;
    case LEX_PROTOCOL_PARSE_ERROR:
        parser->state = P_ST_PROTOCOL_PARSE_ERROR;
        break;
    default:
        parser->state = P_ST_WRONG;
    }

    return cmd;
}

static command_t *p_st_expect_optional_str(parser_t *parser)
{
    command_t *cmd = NULL;

    switch (parser->cur_lex->type) {
    case LEX_WORD:
        parser->tmp_cmd.value = parser->cur_lex->value;
        parser->cur_lex_data_used = 1;
        parser->request_for_lex = 1;
        parser->state = P_ST_PROCESS_ARG2;
        break;
    case LEX_EOLN:
        parser->request_for_lex = 1;
        parser->state = P_ST_START;
        /* parser->tmp_cmd.type already defined. */
        parser->tmp_cmd.value.str = NULL;
        cmd = new_command(parser);
        break;
    case LEX_PROTOCOL_PARSE_ERROR:
        parser->state = P_ST_PROTOCOL_PARSE_ERROR;
        break;
    default:
        parser->state = P_ST_WRONG;
    }

    return cmd;
}

static command_t *p_st_expect_two_numbers(parser_t *parser)
{
    switch (parser->cur_lex->type) {
    case LEX_NUMBER:
        parser->tmp_cmd.value = parser->cur_lex->value;
        parser->request_for_lex = 1;
        parser->state = P_ST_PROCESS_ARG2;
        break;
    case LEX_PROTOCOL_PARSE_ERROR:
        parser->state = P_ST_PROTOCOL_PARSE_ERROR;
        break;
    case LEX_WORD:
    case LEX_EOLN:
    default:
        parser->state = P_ST_WRONG;
    }

    return NULL;
}

static command_t *p_st_expect_one_number(parser_t *parser)
{
    switch (parser->cur_lex->type) {
    case LEX_NUMBER:
        switch (parser->tmp_cmd.type) {
        case CMD_BUILD:
        case CMD_MAKE:
            parser->tmp_cmd.value = parser->cur_lex->value;
            break;
        case CMD_BUY:
        case CMD_SELL:
            parser->tmp_cmd.value2 = parser->cur_lex->value;
            break;
        default:
            /* Not possible. */
#ifndef DAEMON
            fprintf(stderr, "Parser: error in line %d", __LINE__);
#endif
            return NULL;
        }
        parser->request_for_lex = 1;
        parser->state = P_ST_EXPECT_EOLN;
        break;
    case LEX_PROTOCOL_PARSE_ERROR:
        parser->state = P_ST_PROTOCOL_PARSE_ERROR;
        break;
    case LEX_WORD:
    case LEX_EOLN:
    default:
        parser->state = P_ST_WRONG;
    }

    return NULL;
}

static command_t *p_st_wrong(parser_t *parser)
{
    parser->state = P_ST_SKIP;
    parser->tmp_cmd.type = CMD_WRONG;
    return new_command(parser);
}

static command_t *p_st_empty(parser_t *parser)
{
    parser->request_for_lex = 1;
    parser->state = P_ST_START;
    parser->tmp_cmd.type = CMD_EMPTY;
    return new_command(parser);
}

static command_t *p_st_skip(parser_t *parser)
{
    switch (parser->cur_lex->type) {
    case LEX_WORD:
    case LEX_NUMBER:
    case LEX_WRONG_NUMBER:
        parser->request_for_lex = 1;
        break;
    case LEX_EOLN:
        parser->request_for_lex = 1;
        parser->state = P_ST_START;
        break;
    case LEX_PROTOCOL_PARSE_ERROR:
        parser->state = P_ST_PROTOCOL_PARSE_ERROR;
        break;
    }

    return NULL;
}

static command_t *p_st_protocol_parse_error(parser_t *parser)
{
    parser->tmp_cmd.type = CMD_PROTOCOL_PARSE_ERROR;
    return new_command(parser);
}

void new_parser(parser_t *parser)
{
    parser->state = ST_START;
    new_lexer(&(parser->lexer));
    parser->cur_lex = NULL;
    /* parser->tmp_cmd is undefined. */
    parser->request_for_lex = 1;
    parser->cur_lex_data_used = 0;
}

void put_new_data_to_parser(parser_t *parser,
    char *read_buffer, int read_available)
{
    put_new_data_to_lexer(&(parser->lexer),
        read_buffer, read_available);
}

command_t *get_command(parser_t *parser)
{
    command_t *cmd = NULL;

    do {
        if (parser->request_for_lex) {
            if (parser_get_lex(parser)) {
                parser->request_for_lex = 0;
            } else {
                return NULL; /* Request for new data */
            }
        }

        switch (parser->state) {
        case P_ST_START:
            cmd = p_st_start(parser);
            break;
        case P_ST_PROCESS_ARG1:
            cmd = p_st_process_arg1(parser);
            break;
        case P_ST_PROCESS_ARG2:
            cmd = p_st_process_arg2(parser);
            break;
        case P_ST_EXPECT_CMD_NAME:
            cmd = p_st_expect_cmd_name(parser);
            break;
        case P_ST_EXPECT_EOLN:
            cmd = p_st_expect_eoln(parser);
            break;
        case P_ST_EXPECT_OPTIONAL_STR:
            cmd = p_st_expect_optional_str(parser);
            break;
        case P_ST_EXPECT_TWO_NUMBERS:
            cmd = p_st_expect_two_numbers(parser);
            break;
        case P_ST_EXPECT_ONE_NUMBER:
            cmd = p_st_expect_one_number(parser);
            break;
        case P_ST_WRONG:
            cmd = p_st_wrong(parser);
            break;
        case P_ST_EMPTY:
            cmd = p_st_empty(parser);
            break;
        case P_ST_SKIP:
            cmd = p_st_skip(parser);
            break;
        case P_ST_PROTOCOL_PARSE_ERROR:
            cmd = p_st_protocol_parse_error(parser);
            break;
        } /* switch */
    } while (cmd == NULL);

    return cmd;
}

void destroy_command(command_t *cmd)
{
    if ((cmd->type == CMD_HELP ||
        cmd->type == CMD_NICK) &&
        cmd->value.str != NULL)
    {
        free(cmd->value.str);
    }

    free(cmd);
}
