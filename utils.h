#ifndef UTILS_H_SENTRY
#define UTILS_H_SENTRY

/* Case insensitive. */
#define STR_EQUAL_CASE_INS(str1, str2) (strcasecmp((str1), (str2)) == 0)
/* Case sensitive. */
#define STR_EQUAL(str1, str2) (strcmp((str1), (str2)) == 0)

#if 0 /* Not used. */
#define MIN(n1, n2) (((n1) <= (n2)) ? (n1) : (n2))
#define MAX(n1, n2) (((n1) >= (n2)) ? (n1) : (n2))
#endif

#define LOCALTIME_ERROR(localtime_value) ((localtime_value) == NULL)
#define STRFTIME_ERROR(strftime_value) ((strftime_value) == 0)

/* Why 16? See format in update_time_buf(). */
#define TIME_BUFFER_SIZE 16

#include <time.h>
#include "parser.h"

typedef enum time_buf_type {
    TIME_BUF_PROMPT_AND_RESPONCE,
    TIME_BUF_ASYNC_MSG
} time_buf_type;

type_of_cmd get_cmd_type(char *str);
unsigned int number_to_str(char *buf,
    unsigned int number);
unsigned int get_random(unsigned int max_value);
unsigned int log10i(unsigned int number);
void update_time_buf(char *time_buf, int buf_size,
    time_buf_type type);

#endif
