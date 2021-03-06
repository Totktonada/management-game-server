#ifndef LEXER_H_SENTRY
#define LEXER_H_SENTRY

#include <limits.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include "buffer.h"

typedef enum type_of_lex {
    LEX_WORD,
    LEX_NUMBER,
    LEX_EOLN,
    LEX_WRONG_NUMBER,
    LEX_PROTOCOL_PARSE_ERROR
} type_of_lex;

typedef union value_of_lex {
    int number;
    char *str;
} value_of_lex;

typedef struct lexeme {
    type_of_lex type;
    value_of_lex value;
} lexeme;

typedef enum lexer_state {
    ST_START,
    ST_WORD,
    ST_NUMBER,
    ST_EOLN,
    ST_PROTOCOL_PARSE_ERROR
} lexer_state;

typedef struct lexer_info {
    lexer_state state;
    char *read_pointer;
    int read_available;
    buffer buf;
    char c; /* current symbol */
    unsigned int request_for_char:1;
} lexer_info;

void new_lexer_info(lexer_info *linfo);
void put_new_data_to_lexer(lexer_info *linfo,
    char *read_buffer, int read_available);
lexeme *get_lex(lexer_info *linfo);
void destroy_lex(lexeme *lex, int free_data);

#endif
