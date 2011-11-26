#include "lexer.h"

void new_lexer_info (lexer_info *linfo)
{
    linfo->state = ST_START;
    new_buffer (&(linfo->buf));
    linfo->read_pointer = NULL;
    linfo->read_available = 0;
    /* linfo->c is undefined */
    linfo->request_for_char = 1;
}

void put_new_data_to_lexer (lexer_info *linfo,
    char *read_buffer, int read_available)
{
    linfo->read_pointer = read_buffer;
    linfo->read_available = read_available;
}

/* Returns new lexeme.
 * If (type == LEX_NUMBER) and number processing
 * failed returns lexeme with LEX_WRONG_NUMBER type.
 * Returns LEX_WRONG_NUMBER possible,
 * if type argument is LEX_NUMBER or LEX_WRONG_NUMBER. */
lexeme *new_lex (type_of_lex type, buffer *buf)
{
    lexeme *lex = (lexeme *) malloc (sizeof (lexeme));
    char *tmp_str;
    long int tmp_number;

    lex->type = type;

    switch (type) {
    case LEX_WORD:
        lex->value.str = convert_to_string (buf, 1);
        break;
    case LEX_NUMBER:
        tmp_str = convert_to_string (buf, 1);
        /* We absolutelly sure, that in tmp_str contain
         * only digits, except '\0' in end.
         * Consequently, tmp_number >= 0,
         * if no overflow. */
        errno = 0;
        tmp_number = strtol (tmp_str, NULL, 10);
        if (errno != 0 || tmp_number > INT_MAX) {
            /* overflow */
            lex->type = LEX_WRONG_NUMBER;
            /* lex->value is undefined */
        } else {
            lex->value.number = (int) tmp_number;
        }
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
int get_char (lexer_info *linfo)
{
    if (linfo->read_available <= 0)
        return 0;

    linfo->c = *(linfo->read_pointer);
    ++(linfo->read_pointer);
    --(linfo->read_available);

    return 1;
}

void destroy_lex (lexeme *lex)
{
    if (lex->type == LEX_WORD && lex->value.str != NULL)
        free (lex->value.str);
    free (lex);
}

lexeme *st_start (lexer_info *linfo)
{
    lexeme *lex = NULL;

    /* TODO: switch more pretty, but isdigit () is necessary. */
    if (linfo->c == '\n') {          /* '\n' */
        linfo->state = ST_PROTOCOL_PARSE_ERROR;
    } else if (isblank (linfo->c)) { /* ' ', '\t' */
        linfo->request_for_char = 1;
    } else if (isdigit (linfo->c)) { /* [0-9] */
        linfo->state = ST_NUMBER;
    } else if (linfo->c == '\r') {   /* '\r' */
        linfo->state = ST_EOLN;
        linfo->request_for_char = 1;
    } else {                         /* any other */
        linfo->state = ST_WORD;
    }

    return lex;
}

lexeme *st_word (lexer_info *linfo)
{
    lexeme *lex = NULL;

    if (linfo->c == '\n') {
        linfo->state = ST_PROTOCOL_PARSE_ERROR;
    } else if (isblank (linfo->c) || linfo->c == '\r') {
        /* ' ', '\t', '\r' */
        linfo->state = ST_START;
        lex = new_lex (LEX_WORD, &(linfo->buf));
    } else {
        add_to_buffer (&(linfo->buf), linfo->c);
        linfo->request_for_char = 1;
    }

    return lex;
}

lexeme *st_number (lexer_info *linfo)
{
    lexeme *lex = NULL;

    if (linfo->c == '\n') {
        linfo->state = ST_PROTOCOL_PARSE_ERROR;
    } else if (isblank (linfo->c) || linfo->c == '\r') {
        /* ' ', '\t', '\r' */
        linfo->state = ST_START;
        lex = new_lex (LEX_NUMBER, &(linfo->buf));
    } else if (isdigit (linfo->c)) {
        /* [0-9] */
        add_to_buffer (&(linfo->buf), linfo->c);
        linfo->request_for_char = 1;
    } else {
        linfo->state = ST_WORD;
    }

    return lex;
}

lexeme *st_eoln (lexer_info *linfo)
{
    lexeme *lex = NULL;

    if (linfo->c == '\n') {
        linfo->state = ST_START;
        linfo->request_for_char = 1;
        lex = new_lex (LEX_EOLN, NULL);
    } else {
        linfo->state = ST_PROTOCOL_PARSE_ERROR;
    }

    return lex;
}

lexeme *st_protocol_parse_error (lexer_info *linfo)
{
    return new_lex (LEX_PROTOCOL_PARSE_ERROR, NULL);
}

/* If new data necessary, returned NULL.
 * Otherwise, returned not NULL lexeme. */
lexeme *get_lex (lexer_info *linfo)
{
    lexeme *lex = NULL;

    /* TODO: сделать кавычки. Превращают число в слово
     * и склеивают слова с табами/пробелами. */

    do {
        if (linfo->request_for_char) {
            if (get_char (linfo)) {
                linfo->request_for_char = 0;
            } else {
                /* Request for new data */
                return NULL;
            }
        }

        switch (linfo->state) {
        case ST_START:
            lex = st_start (linfo);
            break;
        case ST_WORD:
            lex = st_word (linfo);
            break;
        case ST_NUMBER:
            lex = st_number (linfo);
            break;
        case ST_EOLN:
            lex = st_eoln (linfo);
            break;
        case ST_PROTOCOL_PARSE_ERROR:
            lex = st_protocol_parse_error (linfo);
            break;
        } /* switch */
    } while (lex == NULL);

    return lex;
}
