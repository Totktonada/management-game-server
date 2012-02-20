#include "msg_buffer.h"

void new_msg_buffer (msg_buffer *buf)
{
#if 0
    msg_buffer *buf =
        (msg_buffer *) malloc (sizeof (msg_buffer));
#endif
    buf->first_block = NULL;
    buf->last_block = NULL;
    buf->common_length = 0;
}

msg_block *new_str_msg_block (const char *str, int str_length)
{
    msg_block *new_msg_block =
        (msg_block *) malloc (sizeof (msg_block));
    new_msg_block->next = NULL;
    new_msg_block->str = str;
    new_msg_block->number = 0;
    new_msg_block->length = str_length;
    return new_msg_block;
}

msg_block *new_number_msg_block (const int number, int *str_length)
{
    msg_block *new_msg_block =
        (msg_block *) malloc (sizeof (msg_block));
    new_msg_block->next = NULL;
    new_msg_block->str = NULL;
    new_msg_block->number = number;
    new_msg_block->length = *str_length =
        log10i (number) + 1;
    return new_msg_block;
}

void add_str_to_msg_buffer (msg_buffer *buf, char *str,
    unsigned int str_length)
{
    if (buf->first_block == NULL) {
        buf->last_block = buf->first_block =
            new_str_msg_block (str, str_length);
    } else {
        buf->last_block = buf->last_block->next =
            new_str_msg_block (str, str_length);
    }

    buf->common_length += str_length;
}

void add_number_to_msg_buffer (msg_buffer *buf,
    unsigned int number)
{
    int str_length;

    if (buf->first_block == NULL) {
        buf->last_block = buf->first_block =
            new_number_msg_block (number, &str_length);
    } else {
        buf->last_block = buf->last_block->next =
            new_number_msg_block (number, &str_length);
    }

    buf->common_length += str_length;
}

/* Make one string from msg_buffer
 * and return it.
 * msg_buffer is cleared. */
char *msg_buffer_to_string (msg_buffer *buf)
{
    msg_block *current = buf->first_block;
    msg_block *next;
    char *str = (char *) malloc (sizeof (char)
        * (buf->common_length + 1));
    char *cur_sym_to = str;

    while (current != NULL) {
        next = current->next;

        if (current->str == NULL) { /* Number block */
            number_to_str (cur_sym_to, current->number);
        } else { /* String block */
            memcpy (cur_sym_to, current->str, current->length);
        }

        cur_sym_to += current->length;
        free (current);

        current = next;
    }

    buf->last_block = buf->first_block = NULL;
    buf->common_length = 0;

    *cur_sym_to = '\0';
    return str;

}

/* Write msg_buffer contents.
 * msg_buffer is cleared (see comment
 * to msg_buffer_to_string ()). */
void write_msg_buffer (msg_buffer *buf, int write_fd)
{
    int length = buf->common_length;
    char *str = msg_buffer_to_string (buf);

    write (write_fd, str, length);
    free (str);
}

/* ==== Testing ====
gcc -g -Wall -ansi -pedantic -c utils.c -o utils.o &&
gcc -g -Wall -ansi -pedantic utils.o msg_buffer.c -o msg_buffer &&
valgrind --leak-check=full --show-reachable=yes \
--log-file=valgrind.log --time-stamp=yes ./msg_buffer \
list of strings and numbers
==================== */

#if 0
void add_argv_to_msg_buffer (msg_buffer *buf, char **argv)
{
    int tmp_number;

    while (*argv != NULL) {
        tmp_number = atoi (*argv);
        if (tmp_number == 0) {
            add_str_to_msg_buffer (buf, *argv, strlen (*argv));
        } else {
            add_number_to_msg_buffer (buf, tmp_number);
        }
        ++argv;
    }
}

int main (int argc, char **argv)
{
    int iteration = 100000;
    msg_buffer buf;

    new_msg_buffer (&buf);

    while (iteration > 0) {
        add_argv_to_msg_buffer (&buf, argv);
        write_msg_buffer (&buf, STDIN_FILENO);
        --iteration;
    }

    return 0;
}
#endif
