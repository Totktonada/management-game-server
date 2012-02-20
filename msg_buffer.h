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
} msg_block;

typedef struct msg_buffer {
    msg_block *first_block;
    msg_block *last_block;
    unsigned int common_length;
} msg_buffer;

void new_msg_buffer (msg_buffer *buf);
void add_str_to_msg_buffer (msg_buffer *buf,
    char *str, unsigned int str_length);
void add_number_to_msg_buffer (msg_buffer *buf,
    unsigned int number);
void write_msg_buffer (msg_buffer *buf, int write_fd);

#define ADD_S(buf, str); \
add_str_to_msg_buffer (buf, str, sizeof (str) - 1);

#define ADD_SN(buf, str, number); \
add_str_to_msg_buffer (buf, str, sizeof (str) - 1); \
add_number_to_msg_buffer (buf, number);

#define ADD_SNS(buf, str1, number, str2); \
add_str_to_msg_buffer (buf, str1, sizeof (str1) - 1); \
add_number_to_msg_buffer (buf, number); \
add_str_to_msg_buffer (buf, str2, sizeof (str2) - 1);

#endif
