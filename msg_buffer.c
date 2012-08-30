#include "msg_buffer.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if !defined(MSG_B_INITIAL_SIZE)
#define MSG_B_INITIAL_SIZE 4096
#endif

static void expand(msg_buffer *buf)
{
    uint new_size = buf->size + MSG_B_INITIAL_SIZE;
    char *new_block = (char *) malloc(sizeof(char) * new_size);

    if (buf->size > 0) {
        memcpy(new_block, buf->p, buf->size);
        free(buf->p);
    }

    buf->p = new_block;
    buf->size = new_size;
}

void msg_buffer_new(msg_buffer *buf)
{
    buf->p = NULL;
    buf->size = 0;
    buf->free_idx = 0;
}

void add_char(msg_buffer *buf, char c)
{
    if (buf->free_idx == buf->size) {
        expand(buf);
    }

    buf->p[buf->free_idx] = c;
    ++(buf->free_idx);
}

void add_str(msg_buffer *buf, const char *str)
{
    uint length = strlen(str);

    if (length == 0)
        return;

    while (buf->free_idx + length > buf->size) {
        expand(buf);
    }

    memcpy(buf->p + buf->free_idx, str, length);
    buf->free_idx += length;
}

void msg_buffer_write(msg_buffer *buf, int write_fd)
{
    if (buf->size > 0) {
        write(write_fd, buf->p, buf->free_idx);
        msg_buffer_clear(buf);
    }
}

void msg_buffer_clear(msg_buffer *buf)
{
    if (buf->size > 0) {
        free(buf->p);
        msg_buffer_new(buf);
    }
}
