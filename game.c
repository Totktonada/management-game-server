#include "game.h"
#include <stdlib.h>

game_t *new_game(player_t **players, uint players_count)
{
    game_t *game = (game_t *) malloc(sizeof(game_t));

    game->players = players;
    game->players_count = players_count;
    game->month = 0;
    game->level = 2;

    return game;
}

player_t *new_player(const char *nick)
{
    player_t *player = (player_t *) malloc (sizeof(player_t));

    player->nick = nick;

    player->money = START_MONEY;
    player->raw   = START_RAW;
    player->prod  = START_PROD;
    player->fact  = START_FACT;

    player->building_fact_1 = 0;
    player->building_fact_2 = 0;
    player->building_fact_3 = 0;
    player->building_fact_4 = 0;

    player->req_make  = 0;
    player->req_build = 0;

    player->req_raw       = 0;
    player->req_raw_cost  = 0;
    player->req_prod      = 0;
    player->req_prod_cost = 0;

    player->turn = 0;

    return player;
}
