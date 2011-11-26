#include "runner.h"

void new_game_data (game_data *gdata)
{
    gdata->counter = 0;
}

#ifndef DAEMON
void print_cmd (command *cmd)
{
    switch (cmd->type) {
    case CMD_EMPTY:
        printf ("[CMD_EMPTY]\n");
        break;
    case CMD_WRONG:
        printf ("[CMD_WRONG]\n");
        break;
    case CMD_NICK:
        printf ("[CMD_NICK]\n");
        break;
    case CMD_INCR:
        printf ("[CMD_INCR]\n");
        break;
    case CMD_SHOW:
        printf ("[CMD_SHOW]\n");
        break;
    case CMD_HELP:
        printf ("[CMD_HELP]\n");
        break;
    case CMD_PROTOCOL_PARSE_ERROR:
        /* Not possible */
        printf ("[CMD_PROTOCOL_PARSE_ERROR]\n");
        break;
    }
}
#endif

void do_cmd_incr (game_data *gdata, int write_fd)
{
    ++gdata->counter;
}

void do_cmd_show (game_data *gdata, int write_fd)
{
    /* (2^32-1) contain 10 symbols plus '\n'. */
    char *buf = (char *) malloc (11 * sizeof (char));
    unsigned int number = gdata->counter;
    unsigned int i = 0;
    unsigned int del = 1;

    while (number / 10 >= del) {
        del *= 10;
        ++i;
    }

    buf[i + 1] = '\n';
    i = 0;

    do {
        buf[i] = '0' + ((number / del) % 10);
        del /= 10;
        ++i;
    } while (del > 0);

    write (write_fd, buf, i + 1);
    free (buf);
}

void do_cmd_wrong (int write_fd)
{
    const char buf[] = "\
Wrong command. Try \"help\".\n";
    /* Write string without '\0'. */
    write (write_fd, buf, sizeof (buf) - 1);
}

void execute_cmd (game_data *gdata, int write_fd,
    command *cmd)
{
    switch (cmd->type) {
    case CMD_EMPTY:
        /* Do nothing. */
        break;
    case CMD_HELP:
        break;
    case CMD_NICK:
        break;
    case CMD_INCR:
        do_cmd_incr (gdata, write_fd);
        break;
    case CMD_SHOW:
        do_cmd_show (gdata, write_fd);
        break;
    case CMD_WRONG:
        do_cmd_wrong (write_fd);
        break;
    case CMD_PROTOCOL_PARSE_ERROR:
        /* Not possible */
        break;
    }
}
