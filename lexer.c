#include "lexer.h"

/* Returns new lexeme.
 * If (type == LEX_NUMBER) and number processing
 * failed returns lexeme with LEX_WRONG_NUMBER type.
 * Returns LEX_WRONG_NUMBER possible,
 * if type argument is LEX_NUMBER or LEX_WRONG_NUMBER.
 * This function clear buffer if type is LEX_WORD or LEX_NUMBER. */
static lexeme *new_lex(type_of_lex type, buffer *buf)
{
    lexeme *lex = (lexeme *) malloc(sizeof(lexeme));
    char *tmp_str;
    long tmp_number;

    lex->type = type;

    switch (type) {
    case LEX_WORD:
        lex->value.str = convert_to_string(buf, 1);
        break;
    case LEX_NUMBER:
        tmp_str = convert_to_string(buf, 1);
        /* We absolutelly sure, that in tmp_str contain
         * only digits, except '\0' in end.
         * Consequently, tmp_number >= 0,
         * if no overflow. */
        errno = 0;
        tmp_number = strtol(tmp_str, NULL, 10);
        if (errno != 0 || tmp_number > INT_MAX) {
            /* overflow */
            lex->type = LEX_WRONG_NUMBER;
            /* lex->value is undefined */
        } else {
            lex->value.number = (int) tmp_number;
        }
        free(tmp_str);
        break;
    default:
        /* lex->value is undefined. */
        break;
    }

    return lex;
}

/* Returns:
 * 1, if new char readed;
 * 0, otherwise. */
static int get_char(lexer_t *lexer)
{
    if (lexer->read_available <= 0)
        return 0;

    lexer->c = *(lexer->read_pointer);
    ++(lexer->read_pointer);
    --(lexer->read_available);

    return 1;
}

static lexeme *st_start(lexer_t *lexer)
{
    lexeme *lex = NULL;

    if (lexer->c == '\n') {          /* '\n' */
        lexer->state = ST_PROTOCOL_PARSE_ERROR;
    } else if (isblank(lexer->c)) { /* ' ', '\t' */
        lexer->request_for_char = 1;
    } else if (isdigit(lexer->c)) { /* [0-9] */
        lexer->state = ST_NUMBER;
    } else if (lexer->c == '\r') {   /* '\r' */
        lexer->state = ST_EOLN;
        lexer->request_for_char = 1;
    } else {                         /* any other */
        lexer->state = ST_WORD;
    }

    return lex;
}

static lexeme *st_word(lexer_t *lexer)
{
    lexeme *lex = NULL;

    if (lexer->c == '\n') {
        lexer->state = ST_PROTOCOL_PARSE_ERROR;
        clear_buffer(&(lexer->buf));
    } else if (isblank(lexer->c) || lexer->c == '\r') {
        /* ' ', '\t', '\r' */
        lexer->state = ST_START;
        lex = new_lex(LEX_WORD, &(lexer->buf));
    } else {
        add_to_buffer(&(lexer->buf), lexer->c);
        lexer->request_for_char = 1;
    }

    return lex;
}

static lexeme *st_number(lexer_t *lexer)
{
    lexeme *lex = NULL;

    if (lexer->c == '\n') {
        lexer->state = ST_PROTOCOL_PARSE_ERROR;
        clear_buffer(&(lexer->buf));
    } else if (isblank(lexer->c) || lexer->c == '\r') {
        /* ' ', '\t', '\r' */
        lexer->state = ST_START;
        lex = new_lex(LEX_NUMBER, &(lexer->buf));
    } else if (isdigit(lexer->c)) {
        /* [0-9] */
        add_to_buffer(&(lexer->buf), lexer->c);
        lexer->request_for_char = 1;
    } else {
        lexer->state = ST_WORD;
    }

    return lex;
}

static lexeme *st_eoln(lexer_t *lexer)
{
    lexeme *lex = NULL;

    if (lexer->c == '\n') {
        lexer->state = ST_START;
        lexer->request_for_char = 1;
        lex = new_lex(LEX_EOLN, NULL);
    } else {
        lexer->state = ST_PROTOCOL_PARSE_ERROR;
    }

    return lex;
}

static lexeme *st_protocol_parse_error()
{
    return new_lex(LEX_PROTOCOL_PARSE_ERROR, NULL);
}

void new_lexer(lexer_t *lexer)
{
    lexer->state = ST_START;
    new_buffer(&(lexer->buf));
    lexer->read_pointer = NULL;
    lexer->read_available = 0;
    /* lexer->c is undefined */
    lexer->request_for_char = 1;
}

void put_new_data_to_lexer(lexer_t *lexer,
    char *read_buffer, int read_available)
{
    lexer->read_pointer = read_buffer;
    lexer->read_available = read_available;
}

lexeme *get_lex(lexer_t *lexer)
{
    lexeme *lex = NULL;

    do {
        if (lexer->request_for_char) {
            if (get_char(lexer)) {
                lexer->request_for_char = 0;
            } else {
                /* Request for new data */
                return NULL;
            }
        }

        switch (lexer->state) {
        case ST_START:
            lex = st_start(lexer);
            break;
        case ST_WORD:
            lex = st_word(lexer);
            break;
        case ST_NUMBER:
            lex = st_number(lexer);
            break;
        case ST_EOLN:
            lex = st_eoln(lexer);
            break;
        case ST_PROTOCOL_PARSE_ERROR:
            lex = st_protocol_parse_error();
            break;
        } /* switch */
    } while (lex == NULL);

    return lex;
}

void destroy_lex(lexeme *lex, int free_data)
{
    if (free_data
        && lex->type == LEX_WORD
        && lex->value.str != NULL)
    {
        free(lex->value.str);
    }
    free(lex);
}
