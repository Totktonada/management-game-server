#include "end_month.h"
#include "auctions.h"
#include "market.h"
#include "game.h"
#include "notifications.h"

static void grant_raw_request(player_t *player)
{
    if (player->req_raw == 0)
        return;

    player->raw += player->req_raw;
    player->money -= player->req_raw_cost;
}

static void grant_prod_request(player_t *player)
{
    if (player->req_prod == 0)
        return;

    player->prod -= player->req_prod;
    player->money += player->req_prod_cost;
}

static void grant_make_request(player_t *player)
{
    if (player->req_make == 0)
        return;

    player->raw  -= player->req_make;
    player->prod += player->req_make;
    player->money -= player->req_make * MAKE_COST;
}

static void grant_build_request(player_t *player)
{
    player->fact += player->building_fact_4;
    player->building_fact_4 = player->building_fact_3;
    player->building_fact_3 = player->building_fact_2;
    player->building_fact_2 = player->building_fact_1;
    player->building_fact_1 = player->req_build;

    player->money -= player->building_fact_1 * BUILD_COST_1;
    player->money -= player->building_fact_4 * BUILD_COST_2;
}

static void after_month_expenses(player_t *player)
{
    player->money -= player->raw * RAW_EXPENSE;
    player->money -= player->prod * PROD_EXPENSE;
    player->money -= player->fact * FACT_EXPENSE;
}

/* client MUST be player. */
void player_to_client(game_t *game, client_t *client)
{
    uint i;

    for (i = 0; i < game->players_count; ++i) {
        if (game->players[i] == client->player)
            break;
    }

    free(client->player);
    client->player = NULL;

    --(game->players_count);

    for (; i < game->players_count; ++i) {
        game->players[i] = game->players[i + 1];
    }

    /* (i == game->players_count), index of last entry in array. */
    game->players[i] = NULL;
}

void bankrupts_to_clients(server_t *server)
{
    client_t *cur_client;

    for (cur_client = server->first_client;
        cur_client != NULL;
        cur_client = cur_client->next)
    {
        if (cur_client->player == NULL)
            continue;

        if (cur_client->player->money < 0) {
            player_to_client(server->game, cur_client);
        }
    }
}

uint ready_to_new_month(game_t *game)
{
    uint i;

    if (game == NULL)
        return 0;

    for (i = 0; i < game->players_count; ++i) {
        if (! game->players[i]->turn)
            return 0;
    }

    return 1;
}

void end_month(game_t *game)
{
    uint i;
    player_t *player;

    make_auction(game, REQUEST_RAW,
        market_raw(game->level, game->players_count));
    make_auction(game, REQUEST_PROD,
        market_prod(game->level, game->players_count));

    for (i = 0; i < game->players_count; ++i) {
        player = game->players[i];

        grant_raw_request(player);
        grant_prod_request(player);
        grant_make_request(player);
        grant_build_request(player);
        after_month_expenses(player);
    } /* for */
}

void new_month(game_t *game)
{
    uint i;
    player_t *player;

    for (i = 0; i < game->players_count; ++i) {
        player = game->players[i];

        /* Flush requests */
        player->req_raw       = 0;
        player->req_raw_cost  = 0;
        player->req_prod      = 0;
        player->req_prod_cost = 0;
        player->req_make      = 0;
        player->req_build     = 0;
        player->turn          = 0;
    }

    game->level = market_next_level(game->level);
    ++(game->month);
}
