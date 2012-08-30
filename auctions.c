#include <stdlib.h>

#include "auctions.h"
#include "typedefs.h"
#include "game.h"
#include "utils.h"

typedef struct request {
    struct request *next;
    player_t *player;
    uint count;
} request;

typedef struct request_group {
    struct request_group *next;
    uint cost;
    uint req_count;
    request *first_req;
} request_group;

static request *new_request(player_t *player, request_type type)
{
    request *req = (request *) malloc(sizeof(request));

    req->next = NULL;
    req->player = player;

    if (type == REQUEST_RAW) {
        req->count = player->req_raw;
    } else { /* type == REQUEST_PROD */
        req->count = player->req_prod;
    }

    return req;
}

static request_group *new_request_group(uint cost)
{
    request_group *req_group = (request_group *)
        malloc(sizeof(request_group));
    req_group->next = NULL;
    req_group->cost = cost;
    req_group->req_count = 0;
    req_group->first_req = NULL;
    return req_group;
}

/* Search request group with delivered cost.
 * If group not found, then make it and return. */
static request_group *search_group(request_group **group_p, uint cost,
    request_type type)
{
    request_group *cur, *next, *tmp;

    if (*group_p == NULL) {
        *group_p = new_request_group(cost);
        return *group_p;
    }

    if ((type == REQUEST_RAW && cost > (*group_p)->cost)
        || (type == REQUEST_PROD && cost < (*group_p)->cost))
    {
        tmp = new_request_group(cost);
        tmp->next = *group_p;
        *group_p = tmp;
        return *group_p;
    } else if (cost == (*group_p)->cost) {
        return *group_p;
    }

    cur = *group_p;

    do {
        next = cur->next;

        if (next == NULL ||
            (type == REQUEST_RAW && cost > next->cost)
            || (type == REQUEST_PROD && cost < next->cost))
        {
            tmp = new_request_group(cost);
            tmp->next = next;
            cur->next = tmp;
            return tmp;
        } else if (cost == next->cost) {
            return next;
        }

        cur = next;
    } while (1);
}

/* Frees all requests in group, group and returns
 * next group in list. */
static request_group *free_and_get_next_group(request_group *group)
{
    request_group *next_group = group->next;
    request *cur_req, *next_req;

    /* frees all requests in group. */
    cur_req = group->first_req;
    free(group);

    while (cur_req != NULL) {
        next_req = cur_req->next;
        free(cur_req);
        cur_req = next_req;
    }

    return next_group;
}

/* Returns actual request, move cost in this request group
 * to (*cost_p). Remove request from list and remove
 * empty group from list. */
static request *get_request(request_group **group_p, uint *cost_p)
{
    request_group *group = *group_p;
    request *req, *prev_req;
    uint random;

    req = group->first_req;
    *cost_p = group->cost;

    /* Get random request (req) from group. */
    /* random in [0; (req_count - 1)] */
    random = get_random(group->req_count - 1);
    prev_req = NULL;

    while (random != 0) {
        prev_req = req;
        req = req->next;
        --random;
    }

    /* Remove request from list. */
    if (prev_req == NULL) {
        group->first_req = req->next;
    } else {
        prev_req->next = req->next;
    }

    --(group->req_count);

    if (group->req_count == 0) {
        *group_p = free_and_get_next_group(group);
    }

    return req;
}

/* Add all requests to our structures, flush in player requests. */
static request_group *put_to_groups(game_t *game, request_type type)
{
    uint i;
    uint cost;
    request_group *first_group = NULL;
    request_group *cur_g = NULL;
    request *req;
    player_t *player;

    for (i = 0; i < game->players_count; ++i) {
        player = game->players[i];

        if ((type == REQUEST_RAW && player->req_raw == 0)
            || (type == REQUEST_PROD && player->req_prod == 0))
        {
            continue;
        }

        req = new_request(player, type);

        cost = (type == REQUEST_RAW) ?
            player->req_raw_cost : player->req_prod_cost;

        /* Get exists or new group */
        cur_g = search_group(&first_group, cost, type);

        /* Add request to this group (to begin of list) */
        req->next = cur_g->first_req;
        cur_g->first_req = req;
        ++(cur_g->req_count);

        /* Flush player requests */
        if (type == REQUEST_RAW) {
            player->req_raw = 0;
            player->req_raw_cost = 0;
        } else { /* (type == REQUEST_PROD) */
            player->req_prod = 0;
            player->req_prod_cost = 0;
        }
    }

    return first_group;
}

void make_auction(game_t *game, request_type type, uint m_count)
{
    request_group *group = put_to_groups(game, type);
    request *req;
    uint cost;

    /* Process requests, put them back to player requests. */
    while (m_count != 0 && group != NULL) {
        req = get_request(&group, &cost);

        if (m_count < req->count)
            req->count = m_count;

        if (type == REQUEST_RAW) {
            req->player->req_raw = req->count;
            req->player->req_raw_cost = cost;
        } else { /* (type == REQUEST_PROD) */
            req->player->req_prod = req->count;
            req->player->req_prod_cost = cost;
        }

        m_count -= req->count;
        free(req);
    }

    /* Flush leftover. */
    while (group != NULL) {
        group = free_and_get_next_group(group);
    } /* after while (group == NULL) */
}
