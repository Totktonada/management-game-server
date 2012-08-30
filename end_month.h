#ifndef END_MONTH_H_SENTRY
#define END_MONTH_H_SENTRY

#include "main.h"
#include "game.h"

void player_to_client(game_t *game, client_t *client);

void bankrupts_to_clients(server_t *server);

uint ready_to_new_month(game_t *game);

void end_month(game_t *game);

void new_month(game_t *game);

#endif /* END_MONTH_H_SENTRY */
