#ifndef NOTIFICATIONS_H_SENTRY
#define NOTIFICATIONS_H_SENTRY

#include "main.h"
#include "game.h"

void msg_round_new_clients(server_t *server);

void msg_month_over_clients(server_t *server);


void msg_round_over_clients(server_t *server);

void msg_nick_changed_clients(server_t *server, const char *old_nick,
    const char *new_nick);

void msg_disconnecting_clients(server_t *server, const char *nick,
    const char *reason);

void msg_info_clients(server_t *server, const char *format, ...);

void msg_info_players(server_t *server, const char *format, ...);

#endif /* NOTIFICATIONS_H_SENTRY */
