#include "game.h"
#include <stdlib.h>
#include <string.h>

game_t *new_game(player_t **players, uint players_count)
{
    game_t *game = (game_t *) malloc(sizeof(game_t));

    game->players = players;
    game->players_count = players_count;
    game->month = 0;
    game->level = 2;
    game->first_bankrupt = NULL;

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

static bankrupt_t *new_bankrupt(const char *nick, uint month,
    uint is_winner)
{
    bankrupt_t *bankrupt = (bankrupt_t *) malloc (sizeof(bankrupt_t));
    uint nick_size = strlen(nick) + 1; /* with '\0' */

    bankrupt->next = NULL;

    bankrupt->nick = (char *) malloc(sizeof(char) * nick_size);
    memcpy(bankrupt->nick, nick, nick_size);

    bankrupt->month = month;
    bankrupt->is_winner = is_winner;

    return bankrupt;
}

void add_to_bankrupts(bankrupt_t **first_bankrupt, const char *nick,
    uint month)
{
    bankrupt_t *bankrupt = new_bankrupt(nick, month, 0);

    bankrupt->next = *first_bankrupt;
    *first_bankrupt = bankrupt;
}

void add_winner_to_bankrupts(game_t *game)
{
    bankrupt_t *bankrupt =
        new_bankrupt(game->players[0]->nick, -1, 1);

    bankrupt->next = game->first_bankrupt;
    game->first_bankrupt = bankrupt;
}

void free_bankrupts(bankrupt_t *bankrupt)
{
    bankrupt_t *next_b = NULL;

    while (bankrupt != NULL) {
        next_b = bankrupt->next;
        free(bankrupt->nick);
        free(bankrupt);
        bankrupt = next_b;
    }
}
