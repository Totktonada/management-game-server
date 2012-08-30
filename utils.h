#ifndef UTILS_H_SENTRY
#define UTILS_H_SENTRY

#include <string.h>
#include "typedefs.h"
#include "main.h"

/* Case insensitive. */
#define STR_EQUAL_CASE_INS(str1, str2) (strcasecmp((str1), (str2)) == 0)
/* Case sensitive. */
#define STR_EQUAL(str1, str2) (strcmp((str1), (str2)) == 0)

#define MIN(n1, n2) (((n1) <= (n2)) ? (n1) : (n2))
#define MAX(n1, n2) (((n1) >= (n2)) ? (n1) : (n2))

/* Why 16? You can low reduce it. See format in comment
 * for 'update_time_buf' func. */
#define TIME_BUFFER_SIZE 16

/* Check the string for corresponding to the one of commands. */
command_kind get_command_kind(const char *str);

/* Get random number from 0 to max_value (inclusive). */
uint get_random(uint max_value);

/* Length of number string representation:
 * log10i(number) + 1 . */
uint log10i(uint number);

/* It writes symbols to buf from 0 position, '\0' symbol is written
 * to the end. (2^32-1) contains 10 symbols, therefore for uint
 * range buf must have size 11.
 * Returns: size of string without '\0'. */
uint number_to_str(char *buf, uint number);

/* Mallocs and returns string with unique nickname. */
char *first_vacant_nick(const client_t *first_client);

/* Get current (local) time and place it to time_buf
 * in format "[%H:%M:%S] ". On fail place "\0" to strbuf
 * (if his size not zero). Mininum buffer size for successful
 * updating time buffer defined by TIME_BUFFER_SIZE macro.
 * Returns: 1 on success (time updated), 0 otherwise. */
uint update_time_buf(char *time_buf, uint buf_size);

/* Returns:
 * client with nick equal to nick argument;
 * NULL, if same client not found. */
client_t *get_client_by_nick(client_t *first_client,
    const char *nick);

/* Returns:
 * constant string described reason for nickname disallow.
 * NULL, if nick is valid. */
const char *is_valid_nick(const char *nick);

/* Get constant string corresponding to 'reason' argument. */
const char *get_reason_string(disconnect_reasons reason);

#endif
