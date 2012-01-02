#include "game.h"

void new_game_data (server_info *sinfo)
{
    sinfo->clients_count = 0;
    sinfo->state = G_ST_WAIT_CLIENTS;
    sinfo->step = 0;
}

void new_client_game_data (client_info *client)
{
    /* Nick would be changed in process_new_client () in "main.c". */
    client->nick = NULL;
    client->money = 10000;
    client->raw_count = 4;
    client->prod_count = 2;
    client->factory_count = 2;
    client->step_completed = 0; /* Not completed */
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

void do_cmd_nick (client_info *client, char *nick)
{
    char msg_your_nickname[] = "Your nickname: ";
    char msg_newline[] = "\n";

    if (nick == NULL) {
        write (client->fd, msg_your_nickname,
            sizeof (msg_your_nickname) - 1);
        write (client->fd, client->nick,
            strlen (client->nick));
        write (client->fd, msg_newline,
            sizeof (msg_newline) - 1);
        return;
    }

    /* TODO */
}

/* TODO: nick -> username ? */
void do_cmd_status (server_info *sinfo, int write_fd,
    char *nick)
{
    char msg_cl_count[] = "Clients count: ";
    char msg_newline[] = "\n";

    if (nick == NULL) {
        write (write_fd, msg_cl_count, sizeof (msg_cl_count) - 1);
        write_number (write_fd, sinfo->clients_count);
        write (write_fd, msg_newline, sizeof (msg_newline) - 1);
        return;
    }

    /* TODO */
}

void do_cmd_prod (server_info *sinfo, int write_fd,
    int amount, int cost)
{
    /* TODO */
}

void do_cmd_buy (server_info *sinfo, int write_fd,
    int amount, int cost)
{
    /* TODO */
}

void do_cmd_sell (server_info *sinfo, int write_fd,
    int amount, int cost)
{
    /* TODO */
}

void do_cmd_build (server_info *sinfo, int write_fd)
{
    /* TODO */
}

void do_cmd_turn (server_info *sinfo, int write_fd)
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

void execute_cmd (server_info *sinfo,
    client_info *client, command *cmd)
{
    switch (cmd->type) {
    case CMD_EMPTY:
        /* Do nothing. */
        break;
    case CMD_HELP:
        do_cmd_help (client->fd, cmd->value.str);
        break;
    case CMD_NICK:
        do_cmd_nick (client, cmd->value.str);
        break;
    case CMD_STATUS:
        do_cmd_status (sinfo, client->fd, cmd->value.str);
        break;
    case CMD_PROD:
        do_cmd_prod (sinfo, client->fd,
            cmd->value.number, cmd->value2.number);
        break;
    case CMD_BUY:
        do_cmd_buy (sinfo, client->fd,
            cmd->value.number, cmd->value2.number);
        break;
    case CMD_SELL:
        do_cmd_sell (sinfo, client->fd,
            cmd->value.number, cmd->value2.number);
        break;
    case CMD_BUILD:
        do_cmd_build (sinfo, client->fd);
        break;
    case CMD_TURN:
        do_cmd_turn (sinfo, client->fd);
        break;
    case CMD_WRONG:
        do_cmd_wrong (client->fd);
        break;
    case CMD_PROTOCOL_PARSE_ERROR:
        /* Not possible */
        break;
    }
}

/* Returns:
 * 0, if client connected
 * 1, if client count full. */
int game_process_new_client (server_info *sinfo)
{
    if (sinfo->state != G_ST_WAIT_CLIENTS) {
        return 1;
    }

    ++(sinfo->clients_count);
    if (sinfo->clients_count == sinfo->expected_clients) {
        sinfo->state = G_ST_IN_GAME;
    }

    return 0;
}

void game_process_next_step (server_info *sinfo,
    client_info *first_client)
{
}
