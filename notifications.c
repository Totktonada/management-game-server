#include <stdio.h>
#include <stdarg.h>
#include "notifications.h"
#include "messages.h"

#define MAX_LINE_LENGTH 80

void msg_round_new_clients(server_t *server)
{
    client_t *cur_c;

    for (cur_c = server->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        msg_round_new(&(cur_c->write_buf), server->game);
    }
}

void msg_month_over_clients(server_t *server)
{
    client_t *cur_c;

    for (cur_c = server->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        msg_month_over(&(cur_c->write_buf), server->game);
    }
}

void msg_round_over_clients(server_t *server)
{
    client_t *cur_c;

    for (cur_c = server->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        msg_round_over(&(cur_c->write_buf), server->game);
    }
}

void msg_nick_changed_clients(server_t *server, const char *old_nick,
    const char *new_nick)
{
    client_t *cur_c;

    for (cur_c = server->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        msg_nick_changed(&(cur_c->write_buf), old_nick, new_nick);
    }
}

void msg_disconnecting_clients(server_t *server, const char *nick,
    const char *reason)
{
    client_t *cur_c;
    static char buffer[MAX_LINE_LENGTH + 1];

    sprintf(buffer, "disconnected: %s.", reason);

    for (cur_c = server->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        msg_disconnecting(&(cur_c->write_buf), nick, buffer);
    }
}

void msg_info_clients(server_t *server, const char *format, ...)
{
    va_list args;
    client_t *cur_c;
    static char buffer[MAX_LINE_LENGTH + 1];

    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);

    for (cur_c = server->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        msg_info(&(cur_c->write_buf), buffer);
    }
}

void msg_info_players(server_t *server, const char *format, ...)
{
    va_list args;
    client_t *cur_c;
    static char buffer[MAX_LINE_LENGTH + 1];

    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);

    for (cur_c = server->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        if (cur_c->player) {
            msg_info(&(cur_c->write_buf), buffer);
        }
    }
}
