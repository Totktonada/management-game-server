#include "game.h"

void new_game_data (server_info *sinfo)
{
    sinfo->clients_count = 0;
    sinfo->state = G_ST_WAIT_CLIENTS;
    sinfo->step = 0;
    sinfo->level = 2;
    sinfo->buy_raw = NULL;
    sinfo->sell_prod = NULL;
}

void drop_request (client_info *client)
{
    client->build_factory_count = 0;
    client->make_prod_count = 0;
#if 0
    client->buy_raw_count = 0;
    client->buy_raw_cost = 0;
    client->sell_prod_count = 0;
    client->sell_prod_cost = 0;
#endif
}

void new_client_game_data (client_info *client)
{
    /* Nick would be changed in process_new_client () in "main.c". */
    client->nick = NULL;
    client->money = 10000;
    client->raw_count = 4;
    client->prod_count = 2;
    client->factory_count = 2;
    client->step_completed = 0; /* Step not completed */

    drop_request (client);
}

request *new_request (client_info *client,
    unsigned int count)
{
    request *req = (request *) malloc (sizeof (request));
    req->next = NULL;
    req->client = client;
    req->count = count;
    return req;
}

request_group *new_request_group (unsigned int cost)
{
    request_group *req_group = (request_group *)
        malloc (sizeof (request_group));
    req_group->next = NULL;
    req_group->cost = cost;
    req_group->req_count = 0;
    req_group->first_req = NULL;
    return req_group;
}

/* Search request (buy raw) group with delivered cost.
 * If group not found, then make it and return. */
request_group *search_buy_raw_group (server_info *sinfo,
    unsigned int cost)
{
    request_group *cur, *next, *tmp;

    if (sinfo->buy_raw == NULL) {
        sinfo->buy_raw = new_request_group (cost);
        return sinfo->buy_raw;
    }

    if (cost > sinfo->buy_raw->cost) {
        tmp = new_request_group (cost);
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
            tmp = new_request_group (cost);
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
request_group *search_sell_prod_group (server_info *sinfo,
    unsigned int cost)
{
    request_group *cur, *next, *tmp;

    if (sinfo->sell_prod == NULL) {
        sinfo->sell_prod = new_request_group (cost);
        return sinfo->sell_prod;
    }

    if (cost < sinfo->sell_prod->cost) {
        tmp = new_request_group (cost);
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
            tmp = new_request_group (cost);
            tmp->next = next;
            cur->next = tmp;
            return tmp;
        } else if (cost == next->cost) {
            return next;
        }

        cur = next;
    } while (1);
}

void add_buy_raw_request (server_info *sinfo,
    unsigned int cost, request *req)
{
    request_group *group = search_buy_raw_group (sinfo, cost);

    req->next = group->first_req;
    group->first_req = req;
    ++(group->req_count);
}

void add_sell_prod_request (server_info *sinfo,
    unsigned int cost, request *req)
{
    request_group *group = search_sell_prod_group (sinfo, cost);

    req->next = group->first_req;
    group->first_req = req;
    ++(group->req_count);
}

#ifndef DAEMON
void print_cmd (command *cmd)
{
    switch (cmd->type) {
    case CMD_EMPTY:
        printf ("[CMD_EMPTY]\n");
        break;
    case CMD_HELP:
        printf ("[CMD_HELP]\n");
        break;
    case CMD_NICK:
        printf ("[CMD_NICK]\n");
        break;
    case CMD_STATUS:
        printf ("[CMD_STATUS]\n");
        break;
    case CMD_BUILD:
        printf ("[CMD_BUILD]\n");
        break;
    case CMD_PROD:
        printf ("[CMD_PROD]\n");
        break;
    case CMD_BUY:
        printf ("[CMD_BUY]\n");
        break;
    case CMD_SELL:
        printf ("[CMD_SELL]\n");
        break;
    case CMD_TURN:
        printf ("[CMD_TURN]\n");
        break;
    case CMD_WRONG:
        printf ("[CMD_WRONG]\n");
        break;
    case CMD_PROTOCOL_PARSE_ERROR:
        /* Not possible */
        printf ("[CMD_PROTOCOL_PARSE_ERROR]\n");
        break;
    }
}
#endif

void write_number (int write_fd, unsigned int number)
{
    /* See comment to number_to_str (). */
    char *buf = (char *) malloc (11 * sizeof (char));
    int size = number_to_str (buf, number);
    write (write_fd, buf, size);
    free (buf);
}

void do_cmd_help (int write_fd, char *cmd_name)
{
    const char buf_without_cmd[] = "\
Available commands:\n\
* help [command]\n\
* nick [string]\n\
* status [username | --all | -a]\n\
* build count\n\
* prod count\n\
* buy count cost\n\
* sell count cost\n\
* turn\n\
Command parser is case insensitive and have fun,\
 when you type commands in uppercase.\n\
";
    const char buf_cmd_help[] = "\
help [command]\n\
If have not argument, print available commands.\
 If argument is command name, print short help\
 about this command.\n\
";
    const char buf_unknown_cmd[] = "\
Wrong argument: typed command not found.\n\
";
    if (cmd_name == NULL) {
        write (write_fd, buf_without_cmd,
            sizeof (buf_without_cmd) - 1);
        return;
    }

    /* TODO */
    switch (get_cmd_type (cmd_name)) {
    case CMD_HELP:
        write (write_fd, buf_cmd_help,
            sizeof (buf_cmd_help) - 1);
        break;
    case CMD_NICK:
        break;
    case CMD_STATUS:
        break;
    case CMD_BUILD:
        break;
    case CMD_PROD:
        /* TODO: information about cost of one production. */
        break;
    case CMD_BUY:
        break;
    case CMD_SELL:
        break;
    case CMD_TURN:
        break;
    case CMD_WRONG:
        write (write_fd, buf_unknown_cmd,
            sizeof (buf_unknown_cmd) - 1);
        break;
    case CMD_EMPTY:
    case CMD_PROTOCOL_PARSE_ERROR:
        /* Not possible */
        break;
    }
}

void do_cmd_nick (client_info *client, char *nick)
{
    char msg_your_nickname[] = "Your nickname: ";
    char msg_newline[] = "\n";

    if (nick == NULL) {
        write (client->fd, msg_your_nickname,
            sizeof (msg_your_nickname) - 1);
        write (client->fd, client->nick,
            strlen (client->nick));
        write (client->fd, msg_newline,
            sizeof (msg_newline) - 1);
        return;
    }

    /* TODO: change nick. */
    /* TODO: forbid nick, starts with '-'. */
}

void write_common_information (int write_fd, server_info *sinfo)
{
    char msg_newline[] = "\n";
    char msg_cl_count[] = "Connected clients count: ";
    char msg_step[] = "Step: ";

    write (write_fd, msg_cl_count, sizeof (msg_cl_count) - 1);
    write_number (write_fd, sinfo->clients_count);
    write (write_fd, msg_newline, sizeof (msg_newline) - 1);

    write (write_fd, msg_step, sizeof (msg_step) - 1);
    write_number (write_fd, sinfo->step);
    write (write_fd, msg_newline, sizeof (msg_newline) - 1);
}

/* TODO: nick -> username ? */
void write_client_information (int write_fd, client_info *client)
{
    char msg_newline[] = "\n";
    char msg_client_info_head[] = "-------------------\n";
    char msg_nick[] = "Username: ";
    char msg_money[] = "Money: ";
    char msg_raw_count[] = "Raws: ";
    char msg_prod_count[] = "Productions: ";
    char msg_factory_count[] = "Factories: ";
    char msg_step_completed[] = "Step completed: ";

    write (write_fd, msg_client_info_head,
        sizeof (msg_client_info_head) - 1);

    write (write_fd, msg_nick, sizeof (msg_nick) - 1);
    write (write_fd, client->nick, strlen (client->nick));
    write (write_fd, msg_newline, sizeof (msg_newline) - 1);

    write (write_fd, msg_money, sizeof (msg_money) - 1);
    write_number (write_fd, client->money);
    write (write_fd, msg_newline, sizeof (msg_newline) - 1);

    write (write_fd, msg_raw_count, sizeof (msg_raw_count) - 1);
    write_number (write_fd, client->raw_count);
    write (write_fd, msg_newline, sizeof (msg_newline) - 1);

    write (write_fd, msg_prod_count, sizeof (msg_prod_count) - 1);
    write_number (write_fd, client->prod_count);
    write (write_fd, msg_newline, sizeof (msg_newline) - 1);

    write (write_fd, msg_factory_count,
        sizeof (msg_factory_count) - 1);
    write_number (write_fd, client->factory_count);
    write (write_fd, msg_newline, sizeof (msg_newline) - 1);

    write (write_fd, msg_step_completed,
        sizeof (msg_step_completed) - 1);
    write_number (write_fd, client->step_completed);
    /* TODO: write "true" or "false". */
    write (write_fd, msg_newline, sizeof (msg_newline) - 1);

    /* TODO: write information about requests */
}

/* Returns:
 * client with nick equal to nick argument;
 * NULL, if same client not found. */
client_info *get_client_by_nick (client_info *first_client,
    char *nick)
{
    client_info *cur_client;

    for (cur_client = first_client;
        cur_client != NULL;
        cur_client = cur_client->next)
    {
        if (STR_EQUAL (nick, cur_client->nick)) {
            return cur_client;
        }
    }

    return NULL;
}

void do_cmd_status (server_info *sinfo, client_info *client,
    char *nick)
{
    char msg_client_not_found[] = "\
Client with same username not found, try \"status --all\".\n";
    int write_info_for_all_clients = 0;
    int write_info_for_myself = 0;
    client_info *pointed_client = NULL;
    client_info *cur_client;

    if (nick == NULL) {
        write_info_for_myself = 1;
    } else if (STR_EQUAL_CASE_INS (nick, "--all")
        || STR_EQUAL_CASE_INS (nick, "-a"))
    {
        write_info_for_all_clients = 1;
    } else {
        pointed_client = get_client_by_nick (sinfo->first_client, nick);
        if (pointed_client == NULL) {
            write (client->fd, msg_client_not_found,
                sizeof (msg_client_not_found) - 1);
            return;
        }
    }

    write_common_information (client->fd, sinfo);

    if (write_info_for_all_clients) {
        for (cur_client = sinfo->first_client;
            cur_client != NULL;
            cur_client = cur_client->next)
        {
            write_client_information (client->fd, cur_client);
        }
    } else if (write_info_for_myself) {
        write_client_information (client->fd, client);
    } else {
        write_client_information (client->fd, pointed_client);
    }
}

void do_cmd_build (client_info *client, int count)
{
    char msg_request_stored[] = "\
Your request stored to processing on end of step.\n";

    /* TODO: check money. */
    client->build_factory_count = count;

    write (client->fd, msg_request_stored,
        sizeof (msg_request_stored) - 1);
}

void do_cmd_prod (client_info *client, int count)
{
    char msg_request_stored[] = "\
Your request stored to processing on end of step.\n";
    char msg_too_few_factories[] = "\
You have too few factories. Request rejected.\n";

    /* TODO: check money (nessassary?). */
    if (count > client->factory_count) {
        write (client->fd, msg_too_few_factories,
            sizeof (msg_too_few_factories) - 1);
        return;
    }

    client->make_prod_count = count;

    write (client->fd, msg_request_stored,
        sizeof (msg_request_stored) - 1);
}

void do_cmd_buy (server_info *sinfo, client_info *client,
    int count, int cost)
{
    char msg_request_stored[] = "\
Your request stored to processing on end of step.\n";
    request *req;

    /* TODO: check values in dependence on level. */
    req = new_request (client, count);
    add_buy_raw_request (sinfo, cost, req);

    write (client->fd, msg_request_stored,
        sizeof (msg_request_stored) - 1);
}

void do_cmd_sell (server_info *sinfo, client_info *client,
    int count, int cost)
{
    char msg_request_stored[] = "\
Your request stored to processing on end of step.\n";
    request *req;

    /* TODO: check values in dependence on level. */
    req = new_request (client, count);
    add_sell_prod_request (sinfo, cost, req);

    write (client->fd, msg_request_stored,
        sizeof (msg_request_stored) - 1);
}

void do_cmd_turn (server_info *sinfo, client_info *client)
{
    char msg_step_completed[] = "\
Step already completed, wait for other clients.\n";
    char msg_wait_clients[] = "\
Server wait for full count of clients. Please wait.\n";
    client_info *cur_client;
    int all_clients_step_completed;

    if (sinfo->state == G_ST_WAIT_CLIENTS) {
        write (client->fd, msg_wait_clients,
            sizeof (msg_wait_clients) - 1);
        return;
    }

    if (client->step_completed) {
        write (client->fd, msg_step_completed,
            sizeof (msg_step_completed) - 1);
        return;
    }

    client->step_completed = 1;

    all_clients_step_completed = 1;
    for (cur_client = sinfo->first_client;
        cur_client != NULL;
        cur_client = cur_client->next)
    {
        if (! cur_client->step_completed) {
            all_clients_step_completed = 0;
            break;
        }
    }

    if (all_clients_step_completed) {
        game_process_next_step (sinfo);
    }
}

void do_cmd_wrong (int write_fd)
{
    const char buf[] = "\
Wrong command. Try \"help\".\n";
    /* Write string without '\0'. */
    write (write_fd, buf, sizeof (buf) - 1);
}

/* Returns:
 * 0, if command not permitted in current server state;
 * 1, otherwise. */
int permit_command (server_info *sinfo, command *cmd)
{
    switch (cmd->type) {
    case CMD_EMPTY:
    case CMD_HELP:
    case CMD_STATUS:
    case CMD_NICK:
        return 1;
#if 0
    /* Last client can not change nick. */
    case CMD_NICK:
        return (sinfo->state == G_ST_WAIT_CLIENTS);
#endif

    case CMD_BUILD:
    case CMD_PROD:
    case CMD_BUY:
    case CMD_SELL:
    case CMD_TURN:
        return (sinfo->state == G_ST_IN_GAME);

    case CMD_WRONG:
    case CMD_PROTOCOL_PARSE_ERROR:
        /* Continue processing in execute_cmd (). */
        return 1;
    }

    return 1;
}

void execute_cmd (server_info *sinfo,
    client_info *client, command *cmd)
{
    char msg_not_permitted[] = "\
This command not permitted in current server state.\n";

    if (! permit_command (sinfo, cmd)) {
        /* TODO: more detailed responses. */
        write (client->fd, msg_not_permitted,
            sizeof (msg_not_permitted));
        return;
    }

    switch (cmd->type) {
    case CMD_EMPTY:
        /* Do nothing. */
        break;
    case CMD_HELP:
        do_cmd_help (client->fd, cmd->value.str);
        break;
    case CMD_NICK:
        do_cmd_nick (client, cmd->value.str);
        break;
    case CMD_STATUS:
        do_cmd_status (sinfo, client, cmd->value.str);
        break;
    case CMD_BUILD:
        do_cmd_build (client, cmd->value.number);
        break;
    case CMD_PROD:
        do_cmd_prod (client, cmd->value.number);
        break;
    case CMD_BUY:
        do_cmd_buy (sinfo, client,
            cmd->value.number, cmd->value2.number);
        break;
    case CMD_SELL:
        do_cmd_sell (sinfo, client,
            cmd->value.number, cmd->value2.number);
        break;
    case CMD_TURN:
        do_cmd_turn (sinfo, client);
        break;
    case CMD_WRONG:
        do_cmd_wrong (client->fd);
        break;
    case CMD_PROTOCOL_PARSE_ERROR:
        /* Not possible */
        break;
    }
}

/* Returns:
 * 0, if client connected
 * 1, if client count full. */
int game_process_new_client (server_info *sinfo)
{
    if (sinfo->state != G_ST_WAIT_CLIENTS) {
        return 1;
    }

    ++(sinfo->clients_count);
    if (sinfo->clients_count == sinfo->expected_clients) {
        sinfo->state = G_ST_IN_GAME;
    }

    return 0;
}

void grant_request (server_info *sinfo, client_info *client)
{
    /* to transform
    client->build_factory_count = 0;
    client->make_prod_count = 0;
    client->buy_raw_count = 0;
    client->buy_raw_cost = 0;
    client->sell_prod_count = 0;
    client->sell_prod_cost = 0;
    */
}

void after_step_expenses (client_info *client)
{
    /* TODO: write messages about money change. */
    client->money -= RAW_EXPENSES * client->raw_count;
    client->money -= PROD_EXPENSES * client->prod_count;
    client->money -= FACTORY_EXPENSES * client->factory_count;
}

void process_client_bankrupt (server_info *sinfo, client_info *client)
{
    char msg_bankrupt[] = "You are bankrupt!\n";

    /* TODO: maybe, do not disconnect client and permit only
     * "help" and "status" commands. */
    /* TODO: messages about this event for all clients. */
    write (client->fd, msg_bankrupt,
        sizeof (msg_bankrupt) - 1);
    unregister_client (sinfo, client);
    client_disconnect (sinfo, client, 1);
    free (client);
}

#if 0
void buy_raw_one_group (request *req)
{
    request *first_in_group = req;
    unsigned int group_size = 0;
    request *next;
    unsigned int random_from_group;

    while (req != NULL) {
        next = req->next;
        ++group_size;
        if (next == NULL || next->cost != req->cost)
            break;
        req = next;
    }

    random_from_group = /* TODO */;

    for (i = 0; i < random_from_group; ++i)
        req = req->next;
    make_request (req);

    /* TODO: remove request form list. */
}
#endif

/* Frees all requests in group, group and returns
 * next group in list. */
request_group *free_and_get_next_group (request_group *group)
{
    request_group *next_group = group->next;
    request *cur_req, *next_req;

    /* frees all requests in group. */
    cur_req = group->first_req;
    free (group);

    while (cur_req != NULL) {
        next_req = cur_req->next;
        free (cur_req);
        cur_req = next_req;
    }

    return next_group;
}

void buy_raw_auction (server_info *sinfo)
{
    /* TODO: bank_raw_count */
    int bank_raw_count = 6;
    request_group *group = sinfo->buy_raw;
    request *req, *prev_req;
    int random;

    while (group != NULL) {
        while (group->req_count != 0) {
            req = group->first_req;

            /* Get random request (req) from group. */
            random = (int) (((double) (group->req_count - 1)) * rand ()
                / (RAND_MAX + 1.0)); /* [0; (group->req_count - 1)] */
            prev_req = NULL;
            while (random != 0) {
                prev_req = req;
                req = req->next;
                --random;
            }

            if (bank_raw_count < req->count) {
                req->client->raw_count += bank_raw_count;
                bank_raw_count = 0;
            } else {
                bank_raw_count -= req->count;
                req->client->raw_count += req->count;
                if (prev_req == NULL) {
                    group->first_req = req->next;
                } else {
                    prev_req = req->next;
                }
                free (req);
                --(group->req_count);
            }

            if (bank_raw_count == 0) {
                goto free_groups;
            }
        } /* while by requests. */

        group = free_and_get_next_group (group);
    } /* while by groups. */

free_groups:
    while (group != NULL) {
        group = free_and_get_next_group (group);
    }
    sinfo->buy_raw = NULL;
}

void sell_prod_auction (server_info *sinfo)
{
    /* TODO */
}

/* Make production and build factories. */
void grant_requests (client_info *client)
{
    /* TODO: messages for clients. */

    /* Make production. */
    client->raw_count -= client->make_prod_count;
    client->money -= client->make_prod_count * MAKE_PROD_COST;

    /* Build factories. */
#if 0
    client->money -=
        client->build_factory_count * MAKE_FACTORY_FIRST_HALF;
#endif

    client->factory_count += client->building_factory_4;
    client->building_factory_4 = client->building_factory_3;
    client->building_factory_3 = client->building_factory_2;
    client->building_factory_2 = client->building_factory_1;
    client->building_factory_1 = client->build_factory_count;

    client->money -=
        client->building_factory_1 * MAKE_FACTORY_FIRST_HALF;
    client->money -=
        client->building_factory_4 * MAKE_FACTORY_SECOND_HALF;
}

void game_process_next_step (server_info *sinfo)
{
    client_info *cur_client;

    ++(sinfo->step);

    for (cur_client = sinfo->first_client;
        cur_client != NULL;
        cur_client = cur_client->next)
    {
        cur_client->step_completed = 0;

        after_step_expenses (cur_client);
        if (cur_client->money < 0) {
            process_client_bankrupt (sinfo, cur_client);
        }
    } /* for */

    /* TODO: check order of auctions and other requests. */
    buy_raw_auction (sinfo);
    sell_prod_auction (sinfo);

    for (cur_client = sinfo->first_client;
        cur_client != NULL;
        cur_client = cur_client->next)
    {
        grant_requests (cur_client);
    }
}
