#ifndef UTILS_H_SENTRY
#define UTILS_H_SENTRY

/* Case insensitive. */
#define STR_EQUAL_CASE_INS(str1, str2) (strcasecmp ((str1), (str2)) == 0)
/* Case sensitive. */
#define STR_EQUAL(str1, str2) (strcmp ((str1), (str2)) == 0)

#include "parser.h"

type_of_cmd get_cmd_type (char *str);
int number_to_str (char *buf, int number);

#endif