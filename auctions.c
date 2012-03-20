#include "auctions.h"

void flush_auctions_info(server_info *sinfo)
{
    sinfo->buy_raw = NULL;
    sinfo->sell_prod = NULL;
}

void flush_auction_auxiliary_info(client_info *client)
{
    client->buy_raw_count = 0;
    client->buy_raw_cost = 0;
    client->sell_prod_count = 0;
    client->sell_prod_cost = 0;
}

request *new_request(client_info *client,
    unsigned int count)
{
    request *req = (request *) malloc(sizeof(request));
    req->next = NULL;
    req->client = client;
    req->count = count;
    return req;
}

request_group *new_request_group(unsigned int cost)
{
    request_group *req_group = (request_group *)
        malloc(sizeof(request_group));
    req_group->next = NULL;
    req_group->cost = cost;
    req_group->req_count = 0;
    req_group->first_req = NULL;
    return req_group;
}

/* Search request (buy raw) group with delivered cost.
 * If group not found, then make it and return. */
request_group *search_buy_raw_group(server_info *sinfo,
    unsigned int cost)
{
    request_group *cur, *next, *tmp;

    if (sinfo->buy_raw == NULL) {
        sinfo->buy_raw = new_request_group(cost);
        return sinfo->buy_raw;
    }

    if (cost > sinfo->buy_raw->cost) {
        tmp = new_request_group(cost);
        tmp->next = sinfo->buy_raw;
        sinfo->buy_raw = tmp;
        return sinfo->buy_raw;
    } else if (cost == sinfo->buy_raw->cost) {
        return sinfo->buy_raw;
    }

    cur = sinfo->buy_raw;

    do {
        next = cur->next;

        if (next == NULL || cost > next->cost)
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

/* Search request (sell production) group with delivered cost.
 * If group not found, then make it and return. */
request_group *search_sell_prod_group(server_info *sinfo,
    unsigned int cost)
{
    request_group *cur, *next, *tmp;

    if (sinfo->sell_prod == NULL) {
        sinfo->sell_prod = new_request_group(cost);
        return sinfo->sell_prod;
    }

    if (cost < sinfo->sell_prod->cost) {
        tmp = new_request_group(cost);
        tmp->next = sinfo->sell_prod;
        sinfo->sell_prod = tmp;
        return sinfo->sell_prod;
    } else if (cost == sinfo->sell_prod->cost) {
        return sinfo->sell_prod;
    }

    cur = sinfo->sell_prod;

    do {
        next = cur->next;

        if (next == NULL || cost < next->cost)
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

void add_buy_raw_request(server_info *sinfo,
    unsigned int cost, request *req)
{
    request_group *group = search_buy_raw_group(sinfo, cost);

    req->next = group->first_req;
    group->first_req = req;
    ++(group->req_count);
}

void add_sell_prod_request(server_info *sinfo,
    unsigned int cost, request *req)
{
    request_group *group = search_sell_prod_group(sinfo, cost);

    req->next = group->first_req;
    group->first_req = req;
    ++(group->req_count);
}

void try_to_free_group(request_group **group_pointer,
    request_group *group, request_group *prev_group)
{
    if (group->req_count != 0)
        return;

    /* Group empty */
    if (prev_group == NULL) {
        *group_pointer = group->next;
    } else {
        prev_group->next = group->next;
    }

    free(group);
}

/* Remove from structures and free pointer client.
 * Returns:
 * 1, if request by pointed client found;
 * 0, otherwise. */
int free_request_in_group_by_client(request_group **group_pointer,
    client_info *client, request_group *group,
    request_group *prev_group)
{
    request *prev_req = NULL;
    request *cur_req = group->first_req;

    while (cur_req != NULL) {
        if (cur_req->client != client) {
            prev_req = cur_req;
            cur_req = cur_req->next;
            continue;
        }

        /* Request found. */
        --(group->req_count);

        if (cur_req == group->first_req) {
            /* Free first request in group. */
            group->first_req = cur_req->next;
            try_to_free_group(group_pointer, group, prev_group);
        } else {
            prev_req->next = cur_req->next;
            /* We absolutely sure that group is not empty. */
        }

        free(cur_req);
        return 1;
    }

    return 0;
}

/* Free and remove from structures
 * request added by pointed client.
 * If request by this client not found or
 * (client == NULL) then do nothing. */
void free_request_by_client(request_group **group_pointer,
    client_info *client)
{
    request_group *prev_group = NULL;
    request_group *group = *group_pointer;

    if (client == NULL)
        return;

    while (group != NULL) {
        if (free_request_in_group_by_client(
            group_pointer, client,
            group, prev_group))
        {
            return;
        }

        prev_group = group;
        group = group->next;
    }
}

/* Frees all requests in group, group and returns
 * next group in list. */
request_group *free_and_get_next_group(request_group *group)
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
 * to (*cost_pointer).
 * Remove request from list and remove empty group from list. */
request *get_request(request_group **group_pointer,
    unsigned int *cost_pointer)
{
    request_group *group = *group_pointer;
    request *req, *prev_req;
    int random;

    req = group->first_req;
    *cost_pointer = group->cost;

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
        *group_pointer = free_and_get_next_group(group);
    }

    return req;
}

void auction_notify_all(server_info *sinfo, request_type type,
    request *req, unsigned int cost)
{
    client_info *cur_c;

    for (cur_c = sinfo->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        if (cur_c == req->client)
            continue;

        if (type == REQUEST_RAW) {
            ADD_S(&(cur_c->write_buf), "[Raw auction] [Player ");
        } else { /* (type == REQUEST_PROD) */
            ADD_S(&(cur_c->write_buf), "[Prod. auction] [Player ");
        }
        ADD_S_STRLEN(&(cur_c->write_buf), req->client->nick);
        if (type == REQUEST_RAW) {
            ADD_S(&(cur_c->write_buf), "] Buy raws: ");
        } else { /* (type == REQUEST_PROD) */
            ADD_S(&(cur_c->write_buf), "] Sell prod.: ");
        }
        ADD_N(&(cur_c->write_buf), req->count);
        ADD_S(&(cur_c->write_buf), " / ");
        ADD_N(&(cur_c->write_buf), cost);
        ADD_S(&(cur_c->write_buf), "  (count / cost of one)\n");
    }
}

void make_auction_request(server_info *sinfo, request_type type,
    request *req, unsigned int cost)
{
    switch (type) {
    case REQUEST_RAW:
        VAR_CHANGE(&(req->client->write_buf),
            "[Raw auction] Raw: ", &(req->client->raw_count),
            req->count, "\n");
        VAR_CHANGE_MULT(&(req->client->write_buf),
            "[Raw auction] Money: ", &(req->client->money),
            req->count, -((int) cost), "\n");
        break;
    case REQUEST_PROD:
        VAR_CHANGE(&(req->client->write_buf),
            "[Prod. auction] Prod.: ", &(req->client->prod_count),
            -((int) req->count), "\n");
        VAR_CHANGE_MULT(&(req->client->write_buf),
            "[Prod. auction] Money: ", &(req->client->money),
            req->count, cost, "\n");
    }

    auction_notify_all(sinfo, type, req, cost);
}

void make_auction(server_info *sinfo,
    request_group **group_pointer,
    request_type type, unsigned int market_count)
{
    request *req;
    unsigned int cost;

    while (market_count != 0 && *group_pointer != NULL) {
        req = get_request(group_pointer, &cost);
        if (market_count < req->count)
            req->count = market_count;
        make_auction_request(sinfo, type, req, cost);
        market_count -= req->count;
        free(req);
    }

    while (*group_pointer != NULL) {
        *group_pointer = free_and_get_next_group(*group_pointer);
    } /* after while (*group_pointer == NULL) */
}
