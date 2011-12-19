#include "runner.h"

void new_game_data (game_data *gdata)
{
    gdata->users_count = 0;
    gdata->state = G_ST_WAIT_USERS;
    gdata->step = 0;
}

void new_user_game_data (user_game_data *user_gdata)
{
    user_gdata->nick = NULL; /* See process_new_client ()
        in "main.c". NULL for ignore in first_vacant_nick (). */
    user_gdata->money = 10000;
    user_gdata->raw_count = 4;
    user_gdata->prod_count = 2;
    user_gdata->factory_count = 2;
}

#ifndef DAEMON
void print_cmd (command *cmd)
{
    switch (cmd->type) {
    case CMD_EMPTY:
        printf ("[CMD_EMPTY]\n");
        break;
    case CMD_HELP:
        printf ("[CMD_HELP]\n");
        break;
    case CMD_NICK:
        printf ("[CMD_NICK]\n");
        break;
    case CMD_STATUS:
        printf ("[CMD_STATUS]\n");
        break;
    case CMD_PROD:
        printf ("[CMD_PROD]\n");
        break;
    case CMD_BUY:
        printf ("[CMD_BUY]\n");
        break;
    case CMD_SELL:
        printf ("[CMD_SELL]\n");
        break;
    case CMD_BUILD:
        printf ("[CMD_BUILD]\n");
        break;
    case CMD_TURN:
        printf ("[CMD_TURN]\n");
        break;
    case CMD_WRONG:
        printf ("[CMD_WRONG]\n");
        break;
    case CMD_PROTOCOL_PARSE_ERROR:
        /* Not possible */
        printf ("[CMD_PROTOCOL_PARSE_ERROR]\n");
        break;
    }
}
#endif

void write_number (int write_fd, unsigned int number)
{
    /* See comment to number_to_str (). */
    char *buf = (char *) malloc (11 * sizeof (char));
    int size = number_to_str (buf, number);
    write (write_fd, buf, size);
    free (buf);
}

void do_cmd_help (int write_fd, char *cmd_name)
{
    const char buf_without_cmd[] = "\
Available commands:\n\
* help [command]\n\
* nick [string]\n\
* status [username]\n\
* prod amount cost\n\
* buy amount cost\n\
* sell amount cost\n\
* build\n\
* turn\n\
Command parser is case insensitive and have fun,\
 when you type commands in uppercase.\n\
";
    const char buf_cmd_help[] = "\
help [command]\n\
If have not argument, print available commands.\
 If argument is command name, print short help\
 about this command.\n\
";
    const char buf_unknown_cmd[] = "\
Wrong argument: typed command not found.\n\
";
    if (cmd_name == NULL) {
        write (write_fd, buf_without_cmd,
            sizeof (buf_without_cmd) - 1);
        return;
    }

    /* TODO */
    switch (get_cmd_type (cmd_name)) {
    case CMD_HELP:
        write (write_fd, buf_cmd_help,
            sizeof (buf_cmd_help) - 1);
        break;
    case CMD_NICK:
        break;
    case CMD_STATUS:
        break;
    case CMD_PROD:
        break;
    case CMD_BUY:
        break;
    case CMD_SELL:
        break;
    case CMD_BUILD:
        break;
    case CMD_TURN:
        break;
    case CMD_WRONG:
        write (write_fd, buf_unknown_cmd,
            sizeof (buf_unknown_cmd) - 1);
        break;
    case CMD_EMPTY:
    case CMD_PROTOCOL_PARSE_ERROR:
        /* Not possible */
        break;
    }
}

void do_cmd_nick (user_game_data *user_gdata, int write_fd,
    char *nick)
{
    char msg_your_nickname[] = "Your nickname: ";
    char msg_newline[] = "\n";

    if (nick == NULL) {
        write (write_fd, msg_your_nickname,
            sizeof (msg_your_nickname) - 1);
        write (write_fd, user_gdata->nick,
            sizeof (user_gdata->nick)); /* TODO: fix size. */
        write (write_fd, msg_newline,
            sizeof (msg_newline) - 1);
        return;
    }

    /* TODO */
}

/* TODO: nick -> username ? */
void do_cmd_status (game_data *gdata, int write_fd,
    char *nick)
{
    char msg_cl_count[] = "Clients count: ";
    char msg_newline[] = "\n";

    if (nick == NULL) {
        write (write_fd, msg_cl_count,
            sizeof (msg_cl_count) - 1);
        write_number (write_fd, gdata->users_count);
        write (write_fd, msg_newline,
            sizeof (msg_newline) - 1);
        return;
    }

    /* TODO */
}

void do_cmd_prod (game_data *gdata, int write_fd,
    int amount, int cost)
{
    /* TODO */
}

void do_cmd_buy (game_data *gdata, int write_fd,
    int amount, int cost)
{
    /* TODO */
}

void do_cmd_sell (game_data *gdata, int write_fd,
    int amount, int cost)
{
    /* TODO */
}

void do_cmd_build (game_data *gdata, int write_fd)
{
    /* TODO */
}

void do_cmd_turn (game_data *gdata, int write_fd)
{
    /* TODO */
}

void do_cmd_wrong (int write_fd)
{
    const char buf[] = "\
Wrong command. Try \"help\".\n";
    /* Write string without '\0'. */
    write (write_fd, buf, sizeof (buf) - 1);
}

void execute_cmd (game_data *gdata,
    user_game_data *user_gdata,
    int write_fd, command *cmd)
{
    switch (cmd->type) {
    case CMD_EMPTY:
        /* Do nothing. */
        break;
    case CMD_HELP:
        do_cmd_help (write_fd, cmd->value.str);
        break;
    case CMD_NICK:
        do_cmd_nick (user_gdata, write_fd, cmd->value.str);
        break;
    case CMD_STATUS:
        do_cmd_status (gdata, write_fd, cmd->value.str);
        break;
    case CMD_PROD:
        do_cmd_prod (gdata, write_fd,
            cmd->value.number, cmd->value2.number);
        break;
    case CMD_BUY:
        do_cmd_buy (gdata, write_fd,
            cmd->value.number, cmd->value2.number);
        break;
    case CMD_SELL:
        do_cmd_sell (gdata, write_fd,
            cmd->value.number, cmd->value2.number);
        break;
    case CMD_BUILD:
        do_cmd_build (gdata, write_fd);
        break;
    case CMD_TURN:
        do_cmd_turn (gdata, write_fd);
        break;
    case CMD_WRONG:
        do_cmd_wrong (write_fd);
        break;
    case CMD_PROTOCOL_PARSE_ERROR:
        /* Not possible */
        break;
    }
}
