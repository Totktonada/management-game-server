#ifndef MSG_BUFFER_H_SENTRY
#define MSG_BUFFER_H_SENTRY

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "utils.h"

/* string block: number == 0;
 * number block: str == NULL. */
typedef struct msg_block {
	struct msg_block *next;
	const char *str;
    unsigned int number;
    unsigned int length;
    int is_copy;
} msg_block;

typedef struct msg_buffer {
    msg_block *first_block;
    msg_block *last_block;
    unsigned int common_length;
} msg_buffer;

void new_msg_buffer (msg_buffer *buf);
int is_msg_buffer_empty (msg_buffer *buf);
void replace_last_block_with_copy (msg_buffer *buf);
void add_prefix_to_msg_buffer (msg_buffer *buf,
    const char *prefix, unsigned int prefix_length);
void add_str_to_msg_buffer (msg_buffer *buf,
    const char *str, unsigned int str_length);
void add_number_to_msg_buffer (msg_buffer *buf,
    unsigned int number);
void write_msg_buffer (msg_buffer *buf, int write_fd);
void add_half_to_msg_buffer (msg_buffer *buf,
    unsigned int number);

/* Abbreviation components:
 * S - string;
 * N - number;
 * H - half of number; */

#define ADD_PREFIX_STRLEN(buf, prefix) \
do { \
    add_prefix_to_msg_buffer (buf, prefix, \
        strlen (prefix)); \
} while (0)

#define ADD_S(buf, s1) \
do { \
    add_str_to_msg_buffer (buf, s1, sizeof (s1) - 1); \
} while (0)

#define ADD_S_STRLEN(buf, s1) \
do { \
    add_str_to_msg_buffer (buf, s1, strlen (s1)); \
} while (0)

#define ADD_S_STRLEN_MAKE_COPY(buf, s1) \
do { \
    add_str_to_msg_buffer (buf, s1, strlen (s1)); \
    replace_last_block_with_copy (buf); \
} while (0)

#define ADD_N(buf, n1) \
do { \
    if (n1 < 0) { \
        add_str_to_msg_buffer (buf, "-", 1); \
        add_number_to_msg_buffer (buf, -n1); \
    } else { \
        add_number_to_msg_buffer (buf, n1); \
    } \
} while (0)

#define ADD_SNS(buf, s1, n1, s2) \
do { \
    add_str_to_msg_buffer (buf, s1, sizeof (s1) - 1); \
    add_number_to_msg_buffer (buf, n1); \
    add_str_to_msg_buffer (buf, s2, sizeof (s2) - 1); \
} while (0)

#define ADD_SNSHS(buf, s1, n1, s2, n2, s3) \
do { \
    add_str_to_msg_buffer (buf, s1, sizeof (s1) - 1); \
    add_number_to_msg_buffer (buf, n1); \
    add_str_to_msg_buffer (buf, s2, sizeof (s2) - 1); \
    add_half_to_msg_buffer (buf, n2); \
    add_str_to_msg_buffer (buf, s3, sizeof (s3) - 1); \
} while (0)

#define VAR_CHANGE(buf, s1, var_addr, change, s2) \
do { \
    ADD_S (buf, s1); \
    ADD_N (buf, *var_addr); \
    if (change >= 0) { \
        ADD_S (buf, " + "); \
        ADD_N (buf, change); \
    } else { \
        ADD_S (buf, " - "); \
        ADD_N (buf, -change); \
    } \
    *var_addr += change; \
    ADD_S (buf, " = "); \
    ADD_N (buf, *var_addr); \
    ADD_S (buf, s2); \
} while (0)

#define VAR_CHANGE_MULT(buf, s1, var_addr, count, change, s2) \
do { \
    ADD_S (buf, s1); \
    ADD_N (buf, *var_addr); \
    if (change >= 0) { \
        ADD_S (buf, " + ("); \
        ADD_N (buf, count); \
        ADD_S (buf, " * "); \
        ADD_N (buf, change); \
    } else { \
        ADD_S (buf, " - ("); \
        ADD_N (buf, count); \
        ADD_S (buf, " * "); \
        ADD_N (buf, -change); \
    } \
    ADD_S (buf, ")"); \
    *var_addr += (change * count); \
    ADD_S (buf, " = "); \
    ADD_N (buf, *var_addr); \
    ADD_S (buf, s2); \
} while (0)

/* Similar macro I see in [1]. See "#define write_str".
 * [1] http://code.turnkeylinux.org/busybox/networking/telnet.c */

#endif
