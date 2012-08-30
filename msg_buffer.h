#ifndef MSG_BUFFER_H_SENTRY
#define MSG_BUFFER_H_SENTRY

#include "typedefs.h"

typedef struct msg_buffer {
    char *p;
    uint size;
    uint free_idx;
} msg_buffer;

/* No malloc msg_buffer! */
void msg_buffer_new(msg_buffer *buf);

void add_char(msg_buffer *buf, char c);

void add_str(msg_buffer *buf, const char *str);

/* Internally call msg_buffer_clear. */
void msg_buffer_write(msg_buffer *buf, int write_fd);

void msg_buffer_clear(msg_buffer *buf);

#endif /* MSG_BUFFER_H_SENTRY */
