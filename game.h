#ifndef GAME_H_SENTRY
#define GAME_H_SENTRY

#define START_MONEY 10000
#define START_RAW   4
#define START_PROD  2
#define START_FACT  2

#define RAW_EXPENSE  300
#define PROD_EXPENSE 500
#define FACT_EXPENSE 1000

#define MAKE_COST    2000
#define BUILD_COST_1 2500
#define BUILD_COST_2 2500

#include "typedefs.h"

typedef struct player_t {
    const char *nick;
    /* Player data */
    sint money;
    uint raw;
    uint prod;
    uint fact;
    /* Count of 1, 2, 3, 4 month old factories. */
    uint building_fact_1;
    uint building_fact_2;
    uint building_fact_3;
    uint building_fact_4;
    /* Game requests. */
    uint req_make;
    uint req_build;
    /* Auction requests. */
    uint req_raw;
    uint req_raw_cost;
    uint req_prod;
    uint req_prod_cost;
    /* Other requests. */
    uint turn:1;
} player_t;

typedef struct bankrupt_t {
    struct bankrupt_t *next;
    char *nick;
    uint month; /* month of bankrupting */
    uint is_winner;
} bankrupt_t;

typedef struct game_t {
    player_t **players; /* Is not NULL. */
    uint players_count;
    uint month;
    uint level; /* [0-4] */
    bankrupt_t *first_bankrupt;
} game_t;

game_t *new_game(player_t **players, uint players_count);

player_t *new_player(const char *nick);

void add_to_bankrupts(bankrupt_t **first_bankrupt, const char *nick,
    uint month);

void add_winner_to_bankrupts(game_t *game);

void free_bankrupts(bankrupt_t *bankrupt);

#endif /* GAME_H_SENTRY */
