#ifndef UTILS_H_SENTRY
#define UTILS_H_SENTRY

/* Case insensitive. */
#define STR_EQUAL_CASE_INS(str1, str2) (strcasecmp ((str1), (str2)) == 0)
/* Case sensitive. */
#define STR_EQUAL(str1, str2) (strcmp ((str1), (str2)) == 0)

#define MIN(n1, n2) (((n1) <= (n2)) ? (n1) : (n2))
#define MAX(n1, n2) (((n1) >= (n2)) ? (n1) : (n2))

#include "parser.h"

type_of_cmd get_cmd_type (char *str);
unsigned int number_to_str (char *buf,
    unsigned int number);
unsigned int get_random (unsigned int max_value);

#endif
