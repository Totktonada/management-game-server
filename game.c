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

/*  Messages   */
/* =========== */

/* Command help */
/* String splitted to two part, because gcc
 * makes warning on strings longer 509 symbols. */
const char msg_help_common_part1[] = "\
\n\
Available commands:\n\
* help [command]\n\
* nick [string]\n\
* status [username | parameter]\n\
* build count\n\
* make count\n\
* buy count cost\n\
* sell count cost\n\
* turn\n\
* join\n\
\n\
Command parser is case insensitive and have fun,\n\
when you type commands in uppercase.\n\n";

const char msg_help_common_part2[] = "\
Rules you can see in book [1] or [2]. Also, very\n\
short and not full instructions you can find in\n\
\"on command\" help.\n\
\n\
Books:\n\
[1] Chales Wetherell \"Etudes for programmers\"\n\
    (the book has english and russian releases).\n\
[2] http://www.stolyarov.info/books/gameserv\n\
    (only in russian).\n";

const char msg_help_cmd_help[] = "\
help [command]\n\
\n\
If have not argument, print available commands.\n\
If argument is command name, print short help\n\
about this command.\n";

const char msg_help_cmd_nick[] = "\
nick [string]\n\
\n\
If have not argument, simply print your username.\n\
Otherwise, change your username to \"string\".\n\
Usernames starts with '-' are forbidden.\n";

const char msg_help_cmd_status_part1[] = "\
status [username\n\
        | --all | -a\n\
        | --market | -m\n\
        | --players | -p\n\
        | --server | -s]\n\
\n\
Command have few forms:\n\
1. status\n\
   Without arguments command print information\n\
   about your current state.\n\
2. status username\n\
   Print information about pointed player.\n\
3. status [--all | -a]\n\
   Print server information, market information\n\
   and information about all players.\n";

const char msg_help_cmd_status_part2[] = "\
4. status [--market | -m]\n\
   Print market information (current month\n\
   and values dependent on market level).\n\
5. status [--players | -p]\n\
   Print information about all players.\n\
6. status [--server | -s]\n\
   Print server information (connected clients).\n";

/* TODO: use MAKE_FACTORY_FIRST_HALF
 * and MAKE_FACTORY_SECOND_HALF macro. */
const char msg_help_cmd_build[] = "\
build count\n\
\n\
Start process of building count factories.\n\
One factory cost:\n\
* 2500 after current step\n\
* and 2500 after (current + 5) step.\n\
After second payment, count factories is ready to work.\n";

/* TODO: use MAKE_PROD_COST macro. */
const char msg_help_cmd_make[] = "\
make count\n\
\n\
Make production on your factories.\n\
One factory can make one production in one step.\n\
One production cost: 2000.\n";

const char msg_help_cmd_buy[] = "\
buy count cost\n\
\n\
Add request to buy raw auction. Satisfied limited count\n\
of requests (1-3 per client depending on market level).\n\
In the first place satisfy requests with *maximum* cost.\n\
Minimum cost of requested raw limited by some value\n\
(300-800), which depended on market level.\n\
\n\
For information about current market level, see \"status\"\n\
command.\n";

const char msg_help_cmd_sell[] = "\
sell count cost\n\
\n\
Add offer to sell production auction. Satisfied limited\n\
count of offers (1-3 per client depending on market level).\n\
In the first place satisfy offers with *minimum* cost.\n\
Maximum cost of offered production limited by some value\n\
(4500-6500), which depended on market level.\n\
\n\
For information about current market level, see \"status\"\n\
command.\n";

const char msg_help_cmd_turn[] = "\
turn\n\
\n\
To finish step. After this your requests (and offers) really\n\
satisfied (or, on auctions, possibly not satisfied), from\n\
your money substracted some expenses.\n\
\n\
Expenses:\n\
* 350 for one raw;\n\
* 500 for one production;\n\
* 1000 for one factory;\n\
Raws, productions and factories counts used in expenses\n\
calculation after processing all requests and offers.\n";

const char msg_help_cmd_join[] = "\
join\n\
\n\
Make request for particiation in next game round.\n\
Forbidden if you currently in the round.\n";

const char msg_help_cmd_wrong[] = "\
Unknown command. See \"help\" (without argumenst)\n\
for available commands list.\n";

/* "OK" response, for commands with request semantic. */
const char msg_request_stored[] = "\
Okay! Your request stored to after-step processing.\n";

/* For do_cmd_buy () and do_cmd_sell (). */
const char msg_cost_out_of_range[] = "\
Your cost is out of range. Request rejected.\n\
See information by \"status\" command.\n";

const char msg_count_out_of_range[] = "\
Your count is out of range. Request rejected.\n\
See information by \"status\" command.\n";

/* =========== */

void new_game_data (server_info *sinfo)
{
    sinfo->step = 0;
    sinfo->level = 2;
    sinfo->buy_raw = NULL;
    sinfo->sell_prod = NULL;
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

    client->build_factory_count = 0;
    client->make_prod_count = 0;
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
 * This function used sinfo->players_count for calculation,
 * therefore it returns correct value for current step
 * only before or after clients to go bankrupt.
 * Rounding to lower number. */
unsigned int get_market_raw_count (server_info *sinfo)
{
    return (unsigned int) ((raw_count_per_player[sinfo->level]
        * sinfo->players_count) / 2);
}

/* Calculate production need count for current market level.
 * This function used sinfo->players_count for calculation,
 * therefore it returns correct value for current step
 * only before or after clients to go bankrupt.
 * Rounding to lower number. */
unsigned int get_market_prod_count (server_info *sinfo)
{
    return (unsigned int) ((prod_count_per_player[sinfo->level]
        * sinfo->players_count) / 2);
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
    case CMD_MAKE:
        printf ("[CMD_MAKE]\n");
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
    case CMD_JOIN:
        printf ("[CMD_JOIN]\n");
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

/* See msg_help_* strings in top of this file. */
void do_cmd_help (client_info *client, char *cmd_name)
{
    if (cmd_name == NULL) {
        ADD_S (&(client->write_buf), msg_help_common_part1);
        ADD_S (&(client->write_buf), msg_help_common_part2);
        return;
    }

    switch (get_cmd_type (cmd_name)) {
    case CMD_HELP:
        ADD_S (&(client->write_buf), msg_help_cmd_help);
        break;
    case CMD_NICK:
        ADD_S (&(client->write_buf), msg_help_cmd_nick);
        break;
    case CMD_STATUS:
        ADD_S (&(client->write_buf), msg_help_cmd_status_part1);
        ADD_S (&(client->write_buf), msg_help_cmd_status_part2);
        break;
    case CMD_BUILD:
        ADD_S (&(client->write_buf), msg_help_cmd_build);
        break;
    case CMD_MAKE:
        ADD_S (&(client->write_buf), msg_help_cmd_make);
        break;
    case CMD_BUY:
        ADD_S (&(client->write_buf), msg_help_cmd_buy);
        break;
    case CMD_SELL:
        ADD_S (&(client->write_buf), msg_help_cmd_sell);
        break;
    case CMD_TURN:
        ADD_S (&(client->write_buf), msg_help_cmd_turn);
        break;
    case CMD_JOIN:
        ADD_S (&(client->write_buf), msg_help_cmd_join);
        break;
    case CMD_WRONG:
        /* Unknown command. */
        ADD_S (&(client->write_buf), msg_help_cmd_wrong);
        break;
    case CMD_EMPTY:
    case CMD_PROTOCOL_PARSE_ERROR:
        /* Not possible. */
        break;
    }
}

/* Returns:
 * client with nick equal to nick argument;
 * NULL, if same client not found. */
client_info *get_client_by_nick (client_info *first_client,
    const char *nick)
{
    client_info *cur_c;

    for (cur_c = first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        if (STR_EQUAL (nick, cur_c->nick)) {
            return cur_c;
        }
    }

    return NULL;
}

void do_cmd_nick (server_info *sinfo, client_info *client, char *nick)
{
    client_info *cur_c;

    if (nick != NULL) {
        /* If starts with '-'. */
        if (*nick == '-') {
            free (nick);
            ADD_S (&(client->write_buf),
"Usernames starts with '-' are forbidden. Request rejected.\n");
            return;
        }

        /* If already exists. */
        if (get_client_by_nick (sinfo->first_client, nick) != NULL) {
            free (nick);
            ADD_S (&(client->write_buf),
"Client with same nick already exists. Request rejected.\n");
            return;
        }

        /* Messages for all clients. */
        for (cur_c = sinfo->first_client;
            cur_c != NULL;
            cur_c = cur_c->next)
        {
            if (cur_c == client)
                continue;

            ADD_S (&(cur_c->write_buf), "Username change: ");
            ADD_S_STRLEN_MAKE_COPY (&(cur_c->write_buf), client->nick);
            ADD_S (&(cur_c->write_buf), " -> ");
            ADD_S_STRLEN (&(cur_c->write_buf), nick);
            ADD_S (&(cur_c->write_buf), "\n");
        }

        free (client->nick);
        client->nick = nick;
    }

    ADD_S (&(client->write_buf), "Your username: ");
    ADD_S_STRLEN (&(client->write_buf), client->nick);
    ADD_S (&(client->write_buf), "\n");
}

void write_server_information (server_info *sinfo, msg_buffer *write_buf)
{
    ADD_S (write_buf,
"\n==== Server info ====\n");

    ADD_SNS (write_buf,
"Connected (clients): ", sinfo->clients_count, "\n");

    ADD_SNS (write_buf,
"In round (players):  ", sinfo->players_count, "\n");
}

void write_market_information (server_info *sinfo, msg_buffer *write_buf)
{
    int i;

    ADD_S (write_buf,
"\n==== Market info ====\n");

    ADD_SNS (write_buf,
"Current month:   ", sinfo->step, "\n");

    /* sinfo->level is index. */
    ADD_SNS (write_buf,
"Market level:    ", sinfo->level + 1, "\n");

    ADD_SNSHS (write_buf,
"Raws on market:  ",
get_market_raw_count (sinfo), " / ",
raw_count_per_player[sinfo->level],
"          (in all / per player)\n");

    ADD_SNS (write_buf,
"Min raw price:   ", min_raw_price[sinfo->level], "\n");

    ADD_SNSHS (write_buf,
"Productions:     ",
get_market_prod_count (sinfo), " / ",
prod_count_per_player[sinfo->level],
"          (in all / per player)\n");

    ADD_SNS (write_buf,
"Max prod. price: ", max_prod_price[sinfo->level], "\n");

    ADD_S (write_buf,
"Next level:      ");

    for (i = 0; i < 5; ++i) {
        ADD_N (write_buf,
            level_change_probability[sinfo->level][i]);
        if (i < 4) {
            ADD_S (write_buf, ", ");
        } else {
            ADD_S (write_buf, "  (probability * 12)\n");
        }
    }
}

void write_player_information (client_info *client,
    msg_buffer *write_buf, int write_requests)
{
    if (! client->in_round) {
        ADD_S (write_buf, "Client ");
        ADD_S_STRLEN (write_buf, client->nick);
        ADD_S (write_buf, " is not player.");
        return;
    }

    ADD_S (write_buf, "\n==== ");
    ADD_S_STRLEN (write_buf, client->nick);
    ADD_S (write_buf, " ====\n");

    ADD_SNS (write_buf,
"Money:           ", client->money, "\n");

    ADD_SNS (write_buf,
"Raws:            ", client->raw_count, "\n");

    ADD_SNS (write_buf,
"Productions:     ", client->prod_count, "\n");

    ADD_SNS (write_buf,
"Factories:       ", client->factory_count, "\n");

    ADD_S (write_buf,
"State:           ");
    if (client->step_completed) {
        ADD_S (write_buf, "month completed.\n");
    } else {
        ADD_S (write_buf, "month *not* completed.\n");
    }

    if (! write_requests)
        return;

/* TODO: maybe write cost for each request. */

    ADD_S (write_buf,
"\n--- Requests info ---\n");

    ADD_S (write_buf,
"Requested raw:      ");
    ADD_N (write_buf, client->buy_raw_count);
    ADD_S (write_buf, " / ");
    ADD_N (write_buf, client->buy_raw_cost);
    ADD_S (write_buf, "    (count / cost of one)\n");

    ADD_S (write_buf,
"Offered production: ");
    ADD_N (write_buf, client->sell_prod_count);
    ADD_S (write_buf, " / ");
    ADD_N (write_buf, client->sell_prod_cost);
    ADD_S (write_buf, "    (count / cost of one)\n");

    ADD_SNS (write_buf,
"Productions:        ", client->make_prod_count, "\n");

    ADD_SNS (write_buf,
"Factories:          ", client->build_factory_count, "\n");

    ADD_S (write_buf,
"\n--- Building factories ---\n");

    ADD_S (write_buf,
"1/2/3/4 month old:  ");
    ADD_N (write_buf, client->building_factory_1);
    ADD_S (write_buf, " / ");
    ADD_N (write_buf, client->building_factory_2);
    ADD_S (write_buf, " / ");
    ADD_N (write_buf, client->building_factory_3);
    ADD_S (write_buf, " / ");
    ADD_N (write_buf, client->building_factory_4);
    ADD_S (write_buf, "\n");
}

void write_all_players_information (server_info *sinfo,
    client_info *to_client)
{
    client_info *cur_c;

    for (cur_c = sinfo->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        if (! cur_c->in_round)
            continue;

        write_player_information (cur_c,
            &(to_client->write_buf), to_client == cur_c);
    }
}

void do_cmd_status (server_info *sinfo, client_info *client,
    char *nick)
{
    client_info *pointed_client = NULL;

    if (nick == NULL && client->in_round) {
        write_player_information (client, &(client->write_buf), 1);
    } else if (nick == NULL && ! client->in_round) {
        ADD_S (&(client->write_buf), "You are not player.\n");
    } else if (STR_EQUAL_CASE_INS (nick, "--all")
        || STR_EQUAL_CASE_INS (nick, "-a"))
    {
        write_server_information (sinfo, &(client->write_buf));
        write_market_information (sinfo, &(client->write_buf));
        write_all_players_information (sinfo, client);
    } else if (STR_EQUAL_CASE_INS (nick, "--market")
        || STR_EQUAL_CASE_INS (nick, "-m"))
    {
        write_market_information (sinfo, &(client->write_buf));
    } else if (STR_EQUAL_CASE_INS (nick, "--players")
        || STR_EQUAL_CASE_INS (nick, "-p"))
    {
        write_all_players_information (sinfo, client);
    } else if (STR_EQUAL_CASE_INS (nick, "--server")
        || STR_EQUAL_CASE_INS (nick, "-s"))
    {
        write_server_information (sinfo, &(client->write_buf));
    } else {
        pointed_client = get_client_by_nick (sinfo->first_client, nick);
        if (pointed_client == NULL) {
            ADD_S (&(client->write_buf),
"Client with same username not found, try \"status --players\".\n");
        } else {
            write_player_information (pointed_client,
                &(client->write_buf), client == pointed_client);
        }
    }
}

void do_cmd_build (client_info *client, int count)
{
    /* TODO: check money (nessessary?). */
    client->build_factory_count = count;

    ADD_S (&(client->write_buf), msg_request_stored);
}

void do_cmd_make (client_info *client, int count)
{
    /* TODO: check money (nessessary?). */
    if (count > client->factory_count) {
        ADD_S (&(client->write_buf),
"You have too few factories. Request rejected.\n\
See information by \"status\" command.\n");
        return;
    }

    client->make_prod_count = count;

    ADD_S (&(client->write_buf), msg_request_stored);
}

void do_cmd_buy (server_info *sinfo, client_info *client,
    int count, int cost)
{
    request *req;

    if (count > get_market_raw_count (sinfo)) {
        ADD_S (&(client->write_buf), msg_count_out_of_range);
        return;
    }

    if (cost < min_raw_price[sinfo->level]) {
        ADD_S (&(client->write_buf), msg_cost_out_of_range);
        return;
    }

    req = new_request (client, count);
    add_buy_raw_request (sinfo, cost, req);

    client->buy_raw_count = count;
    client->buy_raw_cost = cost;

    ADD_S (&(client->write_buf), msg_request_stored);
}

void do_cmd_sell (server_info *sinfo, client_info *client,
    int count, int cost)
{
    request *req;

    if (count > get_market_prod_count (sinfo)) {
        ADD_S (&(client->write_buf), msg_count_out_of_range);
        return;
    }

    if (count > client->prod_count) {
        ADD_S (&(client->write_buf),
"Your have not so much productions. Request rejected.\n\
See information by \"status\" command.\n");
        return;
    }

    if (cost > max_prod_price[sinfo->level]) {
        ADD_S (&(client->write_buf), msg_cost_out_of_range);
        return;
    }

    req = new_request (client, count);
    add_sell_prod_request (sinfo, cost, req);

    client->sell_prod_count = count;
    client->sell_prod_cost = cost;

    ADD_S (&(client->write_buf), msg_request_stored);
}

void write_not_completed_step_clients (server_info *sinfo,
    msg_buffer *write_buf)
{
    client_info *cur_c;
    int first_nick = 1;

    for (cur_c = sinfo->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        if (!cur_c->in_round || cur_c->step_completed)
            continue;

        if (first_nick) {
            ADD_S (write_buf, "Expected: ");
        } else {
            ADD_S (write_buf, ", ");
        }

        ADD_S_STRLEN (write_buf, cur_c->nick);
        first_nick = 0;
    }

    if (!first_nick)
        ADD_S (write_buf, "\n");
}

void do_cmd_turn (server_info *sinfo, client_info *client)
{
    client_info *cur_c;
    int all_compl;

    if (client->step_completed) {
        ADD_S (&(client->write_buf),
"This month already completed, wait for other players.\n");
        return;
    }

    client->step_completed = 1;

    all_compl = 1;
    for (cur_c = sinfo->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        all_compl = all_compl &&
            (!cur_c->in_round || cur_c->step_completed);

        if ((! cur_c->step_completed)
            || cur_c == client)
        {
            continue;
        }

        ADD_S (&(cur_c->write_buf), "Client ");
        ADD_S_STRLEN (&(cur_c->write_buf), client->nick);
        ADD_S (&(cur_c->write_buf), " completed this month.\n");
        /* TODO: make it more optimal. */
        write_not_completed_step_clients (sinfo, &(cur_c->write_buf));
    }

    ADD_S (&(client->write_buf), "This month completed.\n");
    write_not_completed_step_clients (sinfo, &(client->write_buf));

    if (all_compl) {
        game_process_next_step (sinfo);
    }
}

void do_cmd_join (server_info *sinfo, client_info *client)
{
    if (client->want_to_next_round) {
        ADD_S (&(client->write_buf),
"You already make request to participation\n\
in next round.\n");
        return;
    }

    client->want_to_next_round = 1;
    ++(sinfo->players_count);
    ADD_S (&(client->write_buf),
"Okay! Your request to participating in next game round\n\
is stored.\n");
    /* TODO: maybe send notification for other users. */
}

void do_cmd_wrong (client_info *client)
{
    ADD_S (&(client->write_buf),
"Wrong command or argument(s). See \"help\"\n\
for information about available commands.\n");
}

/* Returns:
 * 0, if command forbidden in dependent with current
 * value of client->in_round. Also, write reason of
 * rejection to the client.
 * 1, otherwise (command allowed). */
int allow_command (client_info *client, command *cmd)
{
    switch (cmd->type) {
    case CMD_EMPTY:
    case CMD_HELP:
    case CMD_STATUS:
    case CMD_NICK:
        return 1;
#if 0
    case CMD_NICK:
        /* Players can not change nick. */
        return (! sinfo->in_round);
#endif

    case CMD_BUILD:
    case CMD_MAKE:
    case CMD_BUY:
    case CMD_SELL:
    case CMD_TURN:
        if (! client->in_round) {
            ADD_S (&(client->write_buf),
"This command is forbidden before start the round.\n");
        }
        /* sinfo->in_round is 1 if client->in_round is 1. */
        return client->in_round;
    case CMD_JOIN:
        if (client->in_round) {
            ADD_S (&(client->write_buf),
"This command is forbidden for players,\n\
which currently in the round.\n");
        }
        return (! client->in_round);

    case CMD_WRONG:
    case CMD_PROTOCOL_PARSE_ERROR:
        /* Continue processing in execute_cmd (). */
        return 1;
    }

    return 1;
}

/* Returns:
 * 1, if pointer cmd->value.str not used.
 * 0, otherwise. */
int execute_cmd (server_info *sinfo,
    client_info *client, command *cmd)
{
    if (! allow_command (client, cmd)) {
        return 1;
    }

    switch (cmd->type) {
    case CMD_EMPTY:
        /* Do nothing. */
        break;
    case CMD_HELP:
        do_cmd_help (client, cmd->value.str);
        break;
    case CMD_NICK:
        do_cmd_nick (sinfo, client, cmd->value.str);
        break;
    case CMD_STATUS:
        do_cmd_status (sinfo, client, cmd->value.str);
        break;
    case CMD_BUILD:
        do_cmd_build (client, cmd->value.number);
        break;
    case CMD_MAKE:
        do_cmd_make (client, cmd->value.number);
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
    case CMD_JOIN:
        do_cmd_join (sinfo, client);
        break;
    case CMD_WRONG:
        do_cmd_wrong (client);
        break;
    case CMD_PROTOCOL_PARSE_ERROR:
        /* Not possible */
        break;
    }

    return (cmd->type != CMD_NICK);
}

void after_step_expenses (client_info *client)
{
    if (client->raw_count > 0) {
        VAR_CHANGE_MULT (&(client->write_buf),
            "[Raw expenses] Money: ", &(client->money),
            client->raw_count, -((int) RAW_EXPENSES), "\n");
    }

    if (client->prod_count > 0) {
        VAR_CHANGE_MULT (&(client->write_buf),
            "[Prod. expenses] Money: ", &(client->money),
            client->prod_count, -((int) PROD_EXPENSES), "\n");
    }

    if (client->factory_count > 0) {
        VAR_CHANGE_MULT (&(client->write_buf),
            "[Factory expenses] Money: ", &(client->money),
            client->factory_count, -((int) FACTORY_EXPENSES), "\n");
    }
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

void make_auction_request (request_type type, request *req,
    unsigned int cost)
{
    switch (type) {
    case REQUEST_RAW:
        VAR_CHANGE (&(req->client->write_buf),
            "[Raw auction] Raw: ", &(req->client->raw_count),
            req->count, "\n");
        VAR_CHANGE_MULT (&(req->client->write_buf),
            "[Raw auction] Money: ", &(req->client->money),
            req->count, -((int) cost), "\n");
        break;
    case REQUEST_PROD:
        VAR_CHANGE (&(req->client->write_buf),
            "[Prod. auction] Prod.: ", &(req->client->prod_count),
            -((int) req->count), "\n");
        VAR_CHANGE_MULT (&(req->client->write_buf),
            "[Prod. auction] Money: ", &(req->client->money),
            req->count, cost, "\n");
    }
}

void make_auction (request_group **group_pointer,
    request_type type, unsigned int market_count)
{
    request *req;
    unsigned int cost;

    while (market_count != 0 && *group_pointer != NULL) {
        req = get_request (group_pointer, &cost);
        if (market_count < req->count)
            req->count = market_count;
        make_auction_request (type, req, cost);
        market_count -= req->count;
        free (req);
    }

    while (*group_pointer != NULL) {
        *group_pointer = free_and_get_next_group (*group_pointer);
    } /* after while (*group_pointer == NULL) */
}

/* Make production and build factories. */
void grant_make_prod_request (client_info *client)
{
    /* TODO: allow factories to not make productions,
     * if clients have little money (nessessary?). */

    if (client->make_prod_count == 0)
        return;

    /* Make production. */
    VAR_CHANGE (&(client->write_buf),
        "[Make production] Raw: ", &(client->raw_count),
        -((int) client->make_prod_count), "\n");
    VAR_CHANGE (&(client->write_buf),
        "[Make production] Prod.: ", &(client->prod_count),
        client->make_prod_count, "\n");
    VAR_CHANGE_MULT (&(client->write_buf),
        "[Make production] Money: ", &(client->money),
        client->make_prod_count, -((int) MAKE_PROD_COST), "\n");
    client->make_prod_count = 0;
}

void grant_build_factories_request (client_info *client)
{
    /* Build factories. */
    if (client->building_factory_4 > 0) {
        VAR_CHANGE (&(client->write_buf),
            "[Building] Factories: ", &(client->factory_count),
            client->building_factory_4, "\n");
    }

    client->building_factory_4 = client->building_factory_3;
    client->building_factory_3 = client->building_factory_2;
    client->building_factory_2 = client->building_factory_1;
    client->building_factory_1 = client->build_factory_count;
    client->build_factory_count = 0;

    if (client->building_factory_1 > 0) {
        VAR_CHANGE_MULT (&(client->write_buf),
            "[Building, payment 1] Money: ", &(client->money),
            client->building_factory_1,
            -((int) MAKE_FACTORY_FIRST_HALF), "\n");
    }

    if (client->building_factory_4 > 0) {
        VAR_CHANGE_MULT (&(client->write_buf),
            "[Building, payment 2] Money: ", &(client->money),
            client->building_factory_4,
            -((int) MAKE_FACTORY_SECOND_HALF), "\n");
    }
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

void flush_auction_auxiliary_info (client_info *client)
{
    client->buy_raw_count = 0;
    client->buy_raw_cost = 0;
    client->sell_prod_count = 0;
    client->sell_prod_cost = 0;
}

void bankrupt_notify_all_clients (server_info *sinfo,
    client_info *client)
{
    client_info *cur_c;

    /* Messages for all clients. */
    for (cur_c = sinfo->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        if (cur_c == client) {
            ADD_S (&(cur_c->write_buf), "You are bankrupt!\n");
        } else {
            ADD_S (&(cur_c->write_buf), "Player ");
            ADD_S_STRLEN (&(cur_c->write_buf), client->nick);
            ADD_S (&(cur_c->write_buf), " is bankrupt.\n");
        }
    }
}

void write_all_winners (server_info *sinfo, msg_buffer *write_buf)
{
    client_info *cur_c;
    int first_nick = 1;

    for (cur_c = sinfo->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        if (! cur_c->in_round)
            continue;

        if (first_nick) {
            ADD_S (write_buf, "Winners: ");
        } else {
            ADD_S (write_buf, ", ");
        }

        ADD_S_STRLEN (write_buf, cur_c->nick);
        first_nick = 0;
    }

    if (!first_nick)
        ADD_S (write_buf, "\n");
}

/* Winner must be existent and single. */
void write_winner (server_info *sinfo)
{
    client_info *cur_c;
    client_info *winner;

    for (cur_c = sinfo->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        if (cur_c->money >= 0) {
            winner = cur_c;
            break;
        }
    }

    for (cur_c = sinfo->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        ADD_S (&(cur_c->write_buf), "Winner: ");
        ADD_S_STRLEN (&(cur_c->write_buf), winner->nick);
        ADD_S (&(cur_c->write_buf), "\n");
    }
}

/* if (players_count > 1) continue game;
 * if (players_count == 1) winner has (money >=0);
 * if (players_count == 0) winners have (in_round == 1). */
void try_to_process_win (server_info *sinfo)
{
    client_info *cur_c;

    if (sinfo->players_count > 1)
        return;

    if (sinfo->players_count == 1) {
        write_winner (sinfo);
    } else {
        /* sinfo->players_count == 0 */
        for (cur_c = sinfo->first_client;
            cur_c != NULL;
            cur_c = cur_c->next)
        {
            write_all_winners (sinfo, &(cur_c->write_buf));
        }
    }

    for (cur_c = sinfo->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        cur_c->in_round = 0;
    }

    process_end_round (sinfo);
    try_to_deferred_start_round (sinfo);
}

void game_process_next_step (server_info *sinfo)
{
    client_info *cur_c;
    unsigned int market_raw_count = get_market_raw_count (sinfo);
    unsigned int market_prod_count = get_market_prod_count (sinfo);

    for (cur_c = sinfo->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        ADD_S (&(cur_c->write_buf),
            "Yeah! All players completed this month!\n");
    }

    make_auction (&(sinfo->buy_raw), REQUEST_RAW, market_raw_count);
    make_auction (&(sinfo->sell_prod), REQUEST_PROD, market_prod_count);

    for (cur_c = sinfo->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        if (! cur_c->in_round)
            continue;

        flush_auction_auxiliary_info (cur_c);
        grant_make_prod_request (cur_c);
        grant_build_factories_request (cur_c);
        after_step_expenses (cur_c);

        if (cur_c->money < 0) {
            bankrupt_notify_all_clients (sinfo, cur_c);
            --(sinfo->players_count);
        }

        cur_c->step_completed = 0;
    } /* for */

    change_level (sinfo);
    ++(sinfo->step);
    try_to_process_win (sinfo);
}
