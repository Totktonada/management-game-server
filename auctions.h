#ifndef AUCTIONS_H_SENTRY
#define AUCTIONS_H_SENTRY

#include "main.h"
#include "msg_buffer.h"

void flush_auctions_info(server_info *sinfo);
void flush_auction_auxiliary_info(client_info *client);
request *new_request(client_info *client,
    unsigned int count);
void add_buy_raw_request(server_info *sinfo,
    unsigned int cost, request *req);
void add_sell_prod_request(server_info *sinfo,
    unsigned int cost, request *req);
void free_request_by_client(request_group **group_pointer,
    client_info *client);
void make_auction(server_info *sinfo,
    request_group **group_pointer,
    request_type type, unsigned int market_count);


#endif /* AUCTIONS_H_SENTRY */
