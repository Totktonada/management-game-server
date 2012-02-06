#include "game.h"

/* Static data */
/* =========== */

/* In massive (implicatium_value * 2) */
const unsigned int raw_count_per_player[5] =
    { 2, 3, 4, 5, 6 };
const unsigned int prod_count_per_player[5] =
    { 6, 5, 4, 3, 2 };

const unsigned int min_raw_price[5] =
    { 800, 650, 500, 400, 300 };
const unsigned int max_prod_price[5] =
    { 6500, 6000, 5500, 5000, 4500 };

/* In massive (implicatium_value * 12) */
/* Sum by string is 12. */
const unsigned int level_change_probability[5][5] = {
    { 4, 4, 2, 1, 1 },
    { 3, 4, 3, 1, 1 },
    { 1, 3, 4, 3, 1 },
    { 1, 1, 3, 4, 3 },
    { 1, 1, 2, 4, 4 }
};

/*   Messages  */
/* =========== */

/* Command help */
const char msg_help_without_cmd[] = "\
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
const char msg_cmd_help[] = "\
help [command]\n\
If have not argument, print available commands.\
 If argument is command name, print short help\
 about this command.\n\
";
const char msg_unknown_cmd[] = "\
Wrong argument: typed command not found.\n\
";

/* Command nick */
char msg_your_nickname[] = "\
Your nickname: ";

/* Common information */
/* TODO: fix mishmash with little and large strigns. */
char msg_cl_count[] = "\
Connected clients count: ";
char msg_step[] = "\
Step: ";
char msg_market_level[] = "\
Market level: ";
char msg_market_info_head[] = "\
----Market info----\n";
char msg_market_raw_count[] = "\
Raw on market (in all / per player): ";
char msg_min_raw_price[] = "\
Min raw price: ";
char msg_market_prod_count[] = "\
Production need on market (in all / per player): ";
char msg_max_prod_price[] = "\
Max production price: ";
char msg_level_change_probability[] = "\
Level change probability (n / 12): ";

/* Client information */
char msg_client_info_head[] = "\
----Client info----\n";
char msg_nick[] = "\
Username: ";
char msg_money[] = "\
Money: ";
char msg_raw_count[] = "\
Raws: ";
char msg_prod_count[] = "\
Productions: ";
char msg_factory_count[] = "\
Factories: ";
char msg_is_step_completed[] = "\
Step completed: ";

/* Command status */
char msg_client_not_found[] = "\
Client with same username not found, try \"status --all\".\n";

/* For requests. */
char msg_request_stored[] = "\
Okay! Your request stored to after-step processing.\n";

/* For requests. Command prod. */
char msg_too_few_factories[] = "\
You have too few factories. Request rejected.\n\
See information by \"status\" command.\n";

/* For requests. Commands buy and sell. */
char msg_cost_out_of_range[] = "\
Your cost is out of range. Request rejected.\n\
See information by \"status\" command.\n";
char msg_count_out_of_range[] = "\
Your count is out of range. Request rejected.\n\
See information by \"status\" command.\n";
char msg_have_not_prod[] = "\
Your have not so much productions. Request rejected.\n\
See information by \"status your_username\" command.\n";

/* Command turn */
char msg_step_completed[] = "\
Step already completed, wait for other clients.\n";
char msg_wait_clients[] = "\
Server wait for full count of clients. Please wait.\n";

/* Wrong command */
const char msg_wrong_cmd[] = "\
Wrong command. Try \"help\".\n";

/* Command allowing */
char msg_not_in_game[] = "\
This command is forbidden before start the game.\n";

/* Process client bankrupt */
char msg_bankrupt[] = "\
You are bankrupt!\n";

/* Auxiliary messages */
char msg_newline[] = "\n";
char msg_dot_five[] = ".5";
char msg_numbers_separator_1[] = " / ";
char msg_numbers_separator_2[] = ", ";

/* =========== */

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
}

void new_client_game_data (client_info *client)
{
    /* Nick would be changed in process_new_client () in "main.c". */
    client->nick = NULL;
    client->money = START_MONEY;
    client->raw_count = START_RAW_COUNT;
    client->prod_count = START_PROD_COUNT;
    client->factory_count = START_FACTORY_COUNT;
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

/* Calculate raw count for current market level.
 * This function used sinfo->clients_count for calculation,
 * therefore it returns correct value for current step
 * only before or after clients to go bankrupt.
 * Rounding to lower number. */
unsigned int get_market_raw_count (server_info *sinfo)
{
    return (unsigned int) ((raw_count_per_player[sinfo->level]
        * sinfo->clients_count) / 2);
}

/* Calculate production need count for current market level.
 * This function used sinfo->clients_count for calculation,
 * therefore it returns correct value for current step
 * only before or after clients to go bankrupt.
 * Rounding to lower number. */
unsigned int get_market_prod_count (server_info *sinfo)
{
    return (unsigned int) ((prod_count_per_player[sinfo->level]
        * sinfo->clients_count) / 2);
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

/* Print number to descriptor write_fd.
 * If (newline != 0), then write newline after number. */
void write_number (int write_fd, unsigned int number, int newline)
{
    /* Why 11? See comment to number_to_str (). */
    char *buf = (char *) malloc (11 * sizeof (char));
    int size = number_to_str (buf, number);

    write (write_fd, buf, size);
    free (buf);

    if (newline)
        write (write_fd, msg_newline, sizeof (msg_newline) - 1);
}

/* Print (number / 2) to descriptor write_fd.
 * If (newline != 0), then write newline after number. */
void write_number_half (int write_fd, unsigned int number, int newline)
{
    /* Why 11? See comment to number_to_str (). */
    char *buf = (char *) malloc (11 * sizeof (char));
    int size = number_to_str (buf, (unsigned int) (number / 2));

    write (write_fd, buf, size);
    free (buf);

    if ((number % 2) == 1)
        write (write_fd, msg_dot_five, sizeof (msg_dot_five) - 1);

    if (newline)
        write (write_fd, msg_newline, sizeof (msg_newline) - 1);
}

void do_cmd_help (int write_fd, char *cmd_name)
{
    if (cmd_name == NULL) {
        write (write_fd, msg_help_without_cmd,
            sizeof (msg_help_without_cmd) - 1);
        return;
    }

    /* TODO */
    switch (get_cmd_type (cmd_name)) {
    case CMD_HELP:
        write (write_fd, msg_cmd_help,
            sizeof (msg_cmd_help) - 1);
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
        write (write_fd, msg_unknown_cmd,
            sizeof (msg_unknown_cmd) - 1);
        break;
    case CMD_EMPTY:
    case CMD_PROTOCOL_PARSE_ERROR:
        /* Not possible */
        break;
    }
}

void do_cmd_nick (client_info *client, char *nick)
{
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
    int i;

    write (write_fd, msg_cl_count, sizeof (msg_cl_count) - 1);
    write_number (write_fd, sinfo->clients_count, 1);

    write (write_fd, msg_step, sizeof (msg_step) - 1);
    write_number (write_fd, sinfo->step, 1);

    write (write_fd, msg_market_level, sizeof (msg_market_level) - 1);
    write_number (write_fd, sinfo->level + 1, 1); /* This is index! */

    write (write_fd, msg_market_info_head,
        sizeof (msg_market_info_head) - 1);

    write (write_fd, msg_market_raw_count,
        sizeof (msg_market_raw_count) - 1);
    write_number (write_fd, get_market_raw_count (sinfo), 0);
    write (write_fd, msg_numbers_separator_1,
        sizeof (msg_numbers_separator_1) - 1);
    write_number_half (write_fd,
        raw_count_per_player[sinfo->level], 1);

    write (write_fd, msg_min_raw_price, sizeof (msg_min_raw_price) - 1);
    write_number (write_fd, min_raw_price[sinfo->level], 1);

    write (write_fd, msg_market_prod_count,
        sizeof (msg_market_prod_count) - 1);
    write_number (write_fd, get_market_prod_count (sinfo), 0);
    write (write_fd, msg_numbers_separator_1,
        sizeof (msg_numbers_separator_1) - 1);
    write_number_half (write_fd,
        prod_count_per_player[sinfo->level], 1);

    write (write_fd, msg_max_prod_price, sizeof (msg_max_prod_price) - 1);
    write_number (write_fd, max_prod_price[sinfo->level], 1);

    write (write_fd, msg_level_change_probability,
        sizeof (msg_level_change_probability) - 1);
    for (i = 0; i < 5; ++i) {
        write_number (write_fd,
            level_change_probability[sinfo->level][i], 0);
        if (i < 4) {
            write (write_fd, msg_numbers_separator_2,
                sizeof (msg_numbers_separator_2) - 1);
        } else {
            write (write_fd, msg_newline, sizeof (msg_newline) - 1);
        }
    }
}

/* TODO: nick -> username ? */
void write_client_information (int write_fd, client_info *client)
{
    write (write_fd, msg_client_info_head,
        sizeof (msg_client_info_head) - 1);

    write (write_fd, msg_nick, sizeof (msg_nick) - 1);
    write (write_fd, client->nick, strlen (client->nick));
    write (write_fd, msg_newline, sizeof (msg_newline) - 1);

    write (write_fd, msg_money, sizeof (msg_money) - 1);
    write_number (write_fd, client->money, 1);

    write (write_fd, msg_raw_count, sizeof (msg_raw_count) - 1);
    write_number (write_fd, client->raw_count, 1);

    write (write_fd, msg_prod_count, sizeof (msg_prod_count) - 1);
    write_number (write_fd, client->prod_count, 1);

    write (write_fd, msg_factory_count,
        sizeof (msg_factory_count) - 1);
    write_number (write_fd, client->factory_count, 1);

    write (write_fd, msg_is_step_completed,
        sizeof (msg_is_step_completed) - 1);
    write_number (write_fd, client->step_completed, 1);
    /* TODO: write "true" or "false". */

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
    /* TODO: check money (nessessary?). */
    client->build_factory_count = count;

    write (client->fd, msg_request_stored,
        sizeof (msg_request_stored) - 1);
}

void do_cmd_prod (client_info *client, int count)
{
    /* TODO: check money (nessessary?). */
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
    request *req;

    if (count > get_market_raw_count (sinfo)) {
        write (client->fd, msg_count_out_of_range,
            sizeof (msg_count_out_of_range) - 1);
        return;
    }

    if (cost < min_raw_price[sinfo->level]) {
        write (client->fd, msg_cost_out_of_range,
            sizeof (msg_cost_out_of_range) - 1);
        return;
    }

    req = new_request (client, count);
    add_buy_raw_request (sinfo, cost, req);

    write (client->fd, msg_request_stored,
        sizeof (msg_request_stored) - 1);
}

void do_cmd_sell (server_info *sinfo, client_info *client,
    int count, int cost)
{
    request *req;

    if (count > get_market_prod_count (sinfo)) {
        write (client->fd, msg_count_out_of_range,
            sizeof (msg_count_out_of_range) - 1);
        return;
    }

    if (count > client->prod_count) {
        write (client->fd, msg_have_not_prod,
            sizeof (msg_have_not_prod) - 1);
        return;
    }

    if (cost > max_prod_price[sinfo->level]) {
        write (client->fd, msg_cost_out_of_range,
            sizeof (msg_cost_out_of_range) - 1);
        return;
    }

    req = new_request (client, count);
    add_sell_prod_request (sinfo, cost, req);

    write (client->fd, msg_request_stored,
        sizeof (msg_request_stored) - 1);
}

void do_cmd_turn (server_info *sinfo, client_info *client)
{
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
    /* Write string without '\0'. */
    write (write_fd, msg_wrong_cmd, sizeof (msg_wrong_cmd) - 1);
}

/* Returns:
 * 0, if command forbidden in current server state.
 * Also, write reason of rejection to the client.
 * 1, otherwise (command allowed). */
int allow_command (server_info *sinfo, int write_fd, command *cmd)
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
        if (sinfo->state == G_ST_IN_GAME) {
            return 1;
        } else {
            write (write_fd, msg_not_in_game,
                sizeof (msg_not_in_game) - 1);
            return 0;
        }

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
    if (! allow_command (sinfo, client->fd, cmd)) {
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
        /* TODO: messages for clients. */
    }

    return 0;
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
    /* TODO: maybe, do not disconnect client and permit only
     * "help" and "status" commands. */
    /* TODO: messages about this event for all clients. */
    write (client->fd, msg_bankrupt,
        sizeof (msg_bankrupt) - 1);
    unregister_client (sinfo, client);
    client_disconnect (sinfo, client, 1);
    free (client);
}

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

/* Returns actual request, move cost in this request group
 * to (*cost_pointer).
 * Remove request from list and remove empty group from list. */
request *get_request (request_group **group_pointer,
    unsigned int *cost_pointer)
{
    request_group *group = *group_pointer;
    request *req, *prev_req;
    int random;

    req = group->first_req;
    *cost_pointer = group->cost;

    /* Get random request (req) from group. */
    /* random in [0; (req_count - 1)] */
    random = get_random (group->req_count - 1);
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
        prev_req = req->next;
    }

    --(group->req_count);

    if (group->req_count == 0) {
        *group_pointer = free_and_get_next_group (group);
    }

    return req;
}

void buy_raw_auction (server_info *sinfo,
    unsigned int market_raw_count)
{
    request *req;
    request_group *group = sinfo->buy_raw;
    unsigned int real_buy_raw_count, cost;

    while (market_raw_count != 0 && group != NULL) {
        req = get_request (&group, &cost);

        real_buy_raw_count = MIN (market_raw_count, req->count);
        req->client->raw_count += real_buy_raw_count;
        req->client->money -= (real_buy_raw_count * cost);
        market_raw_count -= real_buy_raw_count;
    }

    while (group != NULL) {
        group = free_and_get_next_group (group);
    }

    sinfo->buy_raw = NULL;
}

void sell_prod_auction (server_info *sinfo,
    unsigned int market_prod_count)
{
    request *req;
    request_group *group = sinfo->sell_prod;
    unsigned int real_sell_prod_count, cost;

    while (market_prod_count != 0 && group != NULL) {
        req = get_request (&group, &cost);

        real_sell_prod_count = MIN (market_prod_count, req->count);
        free (req);

        req->client->prod_count -= real_sell_prod_count;
        req->client->money += (real_sell_prod_count * cost);
        market_prod_count -= real_sell_prod_count;
    }

    while (group != NULL) {
        group = free_and_get_next_group (group);
    }

    sinfo->sell_prod = NULL;
}

/* Make production and build factories. */
void grant_make_prod_request (client_info *client)
{
    /* TODO: messages for clients. */

    /* TODO: allow factories to not make productions,
     * if clients have little money (nessessary?). */

    /* Make production. */
    client->raw_count -= client->make_prod_count;
    client->prod_count += client->make_prod_count;
    client->money -= client->make_prod_count * MAKE_PROD_COST;
}

void grant_build_factories_request (client_info *client)
{
    /* TODO: messages for clients. */

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

/* Change sinfo->level depending on level_change_probability . */
void change_level (server_info *sinfo)
{
    /* Why 11? See comment to level_change_probability . */
    int random = 1 + get_random (11); /* [1-12] */
    int sum = 0;
    int i;

    for (i = 0; i < 5; ++i) {
        sum += level_change_probability[sinfo->level][i];
        if (sum >= random) {
            sinfo->level = i;
            return;
        }
    }
}

void game_process_next_step (server_info *sinfo)
{
    client_info *cur_client;
    unsigned int market_raw_count = get_market_raw_count (sinfo);
    unsigned int market_prod_count = get_market_prod_count (sinfo);

    buy_raw_auction (sinfo, market_raw_count);
    sell_prod_auction (sinfo, market_prod_count);

    for (cur_client = sinfo->first_client;
        cur_client != NULL;
        cur_client = cur_client->next)
    {
        grant_make_prod_request (cur_client);
        grant_build_factories_request (cur_client);
        after_step_expenses (cur_client);

        if (cur_client->money < 0) {
            process_client_bankrupt (sinfo, cur_client);
        }

        cur_client->step_completed = 0;
    } /* for */

    change_level (sinfo);
    ++(sinfo->step);
}
