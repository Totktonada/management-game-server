#include "parser.h"

void new_parser_info (parser_info *pinfo)
{
    pinfo->state = ST_START;
    new_lexer_info (&(pinfo->linfo));
    pinfo->cur_lex = NULL;
    /* pinfo->tmp_cmd is undefined. */
    pinfo->request_for_lex = 1;
    pinfo->cur_lex_data_used = 0;
}

void put_new_data_to_parser (parser_info *pinfo,
    char *read_buffer, int read_available)
{
    put_new_data_to_lexer (&(pinfo->linfo),
        read_buffer, read_available);
}

/* Returns:
 * 1, if new lex has been got;
 * 0, otherwise. */
int parser_get_lex (parser_info *pinfo)
{
    if (pinfo->cur_lex != NULL)
        destroy_lex (pinfo->cur_lex, ! pinfo->cur_lex_data_used);

    pinfo->cur_lex = get_lex (&(pinfo->linfo));

    if (pinfo->cur_lex == NULL) {
        return 0; /* Request for new data */
    }

    return 1;
}

/* Make new command from pinfo->tmp_cmd. */
command *new_cmd (parser_info *pinfo)
{
    command *cmd = (command *) malloc (sizeof (command));

    cmd->type = pinfo->tmp_cmd.type;
    cmd->value = pinfo->tmp_cmd.value;
    cmd->value2 = pinfo->tmp_cmd.value2;

    return cmd;
}

command *p_st_start (parser_info *pinfo)
{
    switch (pinfo->cur_lex->type) {
    case LEX_WORD:
        pinfo->state = P_ST_EXPECT_CMD_NAME;
        break;
    case LEX_NUMBER:
    case LEX_WRONG_NUMBER:
        pinfo->state = P_ST_WRONG;
        break;
    case LEX_EOLN:
        pinfo->state = P_ST_EMPTY;
        break;
    case LEX_PROTOCOL_PARSE_ERROR:
        pinfo->state = P_ST_PROTOCOL_PARSE_ERROR;
        break;
    }

    return NULL;
}

command *p_st_process_arg1 (parser_info *pinfo)
{
    switch (pinfo->tmp_cmd.type) {
    /* Commands without arguments. */
    case CMD_TURN:
    case CMD_JOIN:
        pinfo->state = P_ST_EXPECT_EOLN;
        break;

    /* Commands with optional str argument. */
    case CMD_HELP:
    case CMD_NICK:
    case CMD_STATUS:
        pinfo->state = P_ST_EXPECT_OPTIONAL_STR;
        break;

    /* Commands with one numerical argument. */
    case CMD_BUILD:
    case CMD_MAKE:
        pinfo->state = P_ST_EXPECT_ONE_NUMBER;
        break;

    /* Commands with two numerical arguments. */
    case CMD_BUY:
    case CMD_SELL:
        pinfo->state = P_ST_EXPECT_TWO_NUMBERS;
        break;

    /* Not possible. */
    default:
#ifndef DAEMON
        fprintf (stderr, "Parser: error in line %d", __LINE__);
#endif
        break; /* For avoid compile error with defined DAEMON macro. */
    }

    return NULL;
}

command *p_st_process_arg2 (parser_info *pinfo)
{
    switch (pinfo->tmp_cmd.type) {
    /* Commands with optional str argument.
     * We already process one argument. */
    case CMD_HELP:
    case CMD_NICK:
    case CMD_STATUS:
    /* Commands with one numerical argument. */
    case CMD_BUILD:
    case CMD_MAKE:
        pinfo->state = P_ST_EXPECT_EOLN;
        break;

    /* Commands with two numerical arguments. */
    case CMD_BUY:
    case CMD_SELL:
        pinfo->state = P_ST_EXPECT_ONE_NUMBER;
        break;

    /* Not possible. */
    default:
#ifndef DAEMON
        fprintf (stderr, "Parser: error in line %d", __LINE__);
#endif
        break; /* For avoid compile error with defined DAEMON macro. */
    }

    return NULL;
}

command *p_st_expect_cmd_name (parser_info *pinfo)
{
    pinfo->request_for_lex = 1;
    pinfo->state = P_ST_PROCESS_ARG1;
    pinfo->tmp_cmd.type = get_cmd_type (pinfo->cur_lex->value.str);

    if (pinfo->tmp_cmd.type == CMD_WRONG) {
        pinfo->request_for_lex = 0;
        pinfo->state = P_ST_WRONG;
    }

    return NULL;
}

command *p_st_expect_eoln (parser_info *pinfo)
{
    command *cmd = NULL;

    switch (pinfo->cur_lex->type) {
    case LEX_EOLN:
        pinfo->state = P_ST_START;
        pinfo->request_for_lex = 1;
        /* pinfo->tmp_cmd already defined
         * (only type or both type and value). */
        cmd = new_cmd (pinfo);
        break;
    case LEX_PROTOCOL_PARSE_ERROR:
        pinfo->state = P_ST_PROTOCOL_PARSE_ERROR;
        break;
    default:
        pinfo->state = P_ST_WRONG;
    }

    return cmd;
}

command *p_st_expect_optional_str (parser_info *pinfo)
{
    command *cmd = NULL;

    switch (pinfo->cur_lex->type) {
    case LEX_WORD:
        pinfo->tmp_cmd.value = pinfo->cur_lex->value;
        pinfo->cur_lex_data_used = 1;
        pinfo->request_for_lex = 1;
        pinfo->state = P_ST_PROCESS_ARG2;
        break;
    case LEX_EOLN:
        pinfo->request_for_lex = 1;
        pinfo->state = P_ST_START;
        /* pinfo->tmp_cmd.type already defined. */
        pinfo->tmp_cmd.value.str = NULL;
        cmd = new_cmd (pinfo);
        break;
    case LEX_PROTOCOL_PARSE_ERROR:
        pinfo->state = P_ST_PROTOCOL_PARSE_ERROR;
        break;
    default:
        pinfo->state = P_ST_WRONG;
    }

    return cmd;
}

command *p_st_expect_two_numbers (parser_info *pinfo)
{
    switch (pinfo->cur_lex->type) {
    case LEX_NUMBER:
        pinfo->tmp_cmd.value = pinfo->cur_lex->value;
        pinfo->request_for_lex = 1;
        pinfo->state = P_ST_PROCESS_ARG2;
        break;
    case LEX_PROTOCOL_PARSE_ERROR:
        pinfo->state = P_ST_PROTOCOL_PARSE_ERROR;
        break;
    case LEX_WORD:
    case LEX_EOLN:
    default:
        pinfo->state = P_ST_WRONG;
    }

    return NULL;
}

command *p_st_expect_one_number (parser_info *pinfo)
{
    switch (pinfo->cur_lex->type) {
    case LEX_NUMBER:
        switch (pinfo->tmp_cmd.type) {
        case CMD_BUILD:
        case CMD_MAKE:
            pinfo->tmp_cmd.value = pinfo->cur_lex->value;
            break;
        case CMD_BUY:
        case CMD_SELL:
            pinfo->tmp_cmd.value2 = pinfo->cur_lex->value;
            break;
        default:
            /* Not possible. */
#ifndef DAEMON
            fprintf (stderr, "Parser: error in line %d", __LINE__);
#endif
            return NULL;
        }
        pinfo->request_for_lex = 1;
        pinfo->state = P_ST_EXPECT_EOLN;
        break;
    case LEX_PROTOCOL_PARSE_ERROR:
        pinfo->state = P_ST_PROTOCOL_PARSE_ERROR;
        break;
    case LEX_WORD:
    case LEX_EOLN:
    default:
        pinfo->state = P_ST_WRONG;
    }

    return NULL;
}

command *p_st_wrong (parser_info *pinfo)
{
    pinfo->state = P_ST_SKIP;
    pinfo->tmp_cmd.type = CMD_WRONG;
    return new_cmd (pinfo);
}

command *p_st_empty (parser_info *pinfo)
{
    pinfo->request_for_lex = 1;
    pinfo->state = P_ST_START;
    pinfo->tmp_cmd.type = CMD_EMPTY;
    return new_cmd (pinfo);
}

command *p_st_skip (parser_info *pinfo)
{
    switch (pinfo->cur_lex->type) {
    case LEX_WORD:
    case LEX_NUMBER:
    case LEX_WRONG_NUMBER:
        pinfo->request_for_lex = 1;
        break;
    case LEX_EOLN:
        pinfo->request_for_lex = 1;
        pinfo->state = P_ST_START;
        break;
    case LEX_PROTOCOL_PARSE_ERROR:
        pinfo->state = P_ST_PROTOCOL_PARSE_ERROR;
        break;
    }

    return NULL;
}

command *p_st_protocol_parse_error (parser_info *pinfo)
{
    pinfo->tmp_cmd.type = CMD_PROTOCOL_PARSE_ERROR;
    return new_cmd (pinfo);
}

command *get_cmd (parser_info *pinfo)
{
    command *cmd = NULL;

    do {
        if (pinfo->request_for_lex) {
            if (parser_get_lex (pinfo)) {
                pinfo->request_for_lex = 0;
            } else {
                return NULL; /* Request for new data */
            }
        }

        switch (pinfo->state) {
        case P_ST_START:
            cmd = p_st_start (pinfo);
            break;
        case P_ST_PROCESS_ARG1:
            cmd = p_st_process_arg1 (pinfo);
            break;
        case P_ST_PROCESS_ARG2:
            cmd = p_st_process_arg2 (pinfo);
            break;
        case P_ST_EXPECT_CMD_NAME:
            cmd = p_st_expect_cmd_name (pinfo);
            break;
        case P_ST_EXPECT_EOLN:
            cmd = p_st_expect_eoln (pinfo);
            break;
        case P_ST_EXPECT_OPTIONAL_STR:
            cmd = p_st_expect_optional_str (pinfo);
            break;
        case P_ST_EXPECT_TWO_NUMBERS:
            cmd = p_st_expect_two_numbers (pinfo);
            break;
        case P_ST_EXPECT_ONE_NUMBER:
            cmd = p_st_expect_one_number (pinfo);
            break;
        case P_ST_WRONG:
            cmd = p_st_wrong (pinfo);
            break;
        case P_ST_EMPTY:
            cmd = p_st_empty (pinfo);
            break;
        case P_ST_SKIP:
            cmd = p_st_skip (pinfo);
            break;
        case P_ST_PROTOCOL_PARSE_ERROR:
            cmd = p_st_protocol_parse_error (pinfo);
            break;
        } /* switch */
    } while (cmd == NULL);

    return cmd;
}

void destroy_cmd (command *cmd, int destroy_str)
{
    if (!destroy_str) {
        free (cmd);
        return;
    }

    switch (cmd->type) {
    case CMD_HELP:
    case CMD_NICK:
    case CMD_STATUS:
        if (cmd->value.str != NULL)
            free (cmd->value.str);
        break;
    default:
        break; /* For avoid compile error. */
    }

    free (cmd);
}
