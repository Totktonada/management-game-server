/*#include "messages.h"*/
#include "msg_buffer.h"
#include "utils.h"
#include "market.h"
#include "main.h"
#include "game.h"

#ifndef DAEMON
#include <stdio.h>
#endif

#define PROTOCOL_VERSION 1

static void number_overflow(const char *file, uint line)
{
#ifndef DAEMON
    fprintf(stderr, "%s:%d Number overflow.\n", file, line);
#endif
}

/* ==== Common message objects ==== */

static void spaces(msg_buffer *buf, uint n)
{
    for (; n > 0; --n) {
        add_char(buf, ' ');
    }
}

static void number(msg_buffer *buf, uint n)
{
    static char str[11];

    number_to_str(str, n);
    add_str(buf, str);
}

static void number_short(msg_buffer *buf, uint n)
{
    if (n > 9999999) {
        number_overflow(__FILE__, __LINE__);
        n = 9999999;
    }

    number(buf, n);
}

#if 0
/* Length: 10. */
static void number_or_spaces(msg_buffer *buf, uint n)
{
    if (n == 0) {
        spaces(buf, 10);
        return;
    }

    spaces(buf, 10 - number_strlen(n));
    number(buf, n);
}

/* Length: 7. */
static void number_or_spaces_short(msg_buffer *buf, uint n)
{
    if (n == 0) {
        spaces(buf, 7);
        return;
    }

    spaces(buf, 7 - number_strlen(n));
    number_short(buf, n);
}
#endif

/* Length: 10. */
static void number_explicit(msg_buffer *buf, uint n)
{
    spaces(buf, 10 - number_strlen(n));
    number(buf, n);
}

/* Length: 7. */
static void number_explicit_short(msg_buffer *buf, uint n)
{
    spaces(buf, 7 - number_strlen(n));
    number_short(buf, n);
}

static void is(msg_buffer *buf, uint yes, const char *str)
{
    if (yes) {
        add_str(buf, str);
    } else {
        spaces(buf, strlen(str));
    }
}

/* Length: 8. */
static void positive(msg_buffer *buf, uint n)
{
    if (n == 0) {
        spaces(buf, 8);
    } else {
        spaces(buf, 7 - number_strlen(n));
        add_char(buf, '+');
        number_short(buf, n);
    }
}

/* Length: 8. */
static void negative(msg_buffer *buf, uint n)
{
    if (n == 0) {
        spaces(buf, 8);
    } else {
        spaces(buf, 7 - number_strlen(n));
        add_char(buf, '-');
        number_short(buf, n);
    }
}

/* Length: 12. */
static void rise(msg_buffer *buf, uint n)
{
    if (n == 0) {
        spaces(buf, 12);
    } else {
        spaces(buf, 10 - number_strlen(n));
        add_char(buf, '+');
        add_char(buf, '$');
        number(buf, n);
    }
}

/* Length: 12. */
static void expense(msg_buffer *buf, uint n)
{
    if (n == 0) {
        spaces(buf, 12);
    } else {
        spaces(buf, 10 - number_strlen(n));
        add_char(buf, '-');
        add_char(buf, '$');
        number(buf, n);
    }
}

/* Length: 12. */
static void money(msg_buffer *buf, sint n)
{
    if (n < 0) {
        spaces(buf, 10 - number_strlen(-n));
        add_char(buf, '-');
        add_char(buf, '$');
        number(buf, -n);
    } else {
        spaces(buf, 11 - number_strlen(n));
        add_char(buf, '$');
        number(buf, n);
    }
}

/* Length: 3. */
static void number_small(msg_buffer *buf, uint n)
{
    if (n > 100) {
        number_overflow(__FILE__, __LINE__);
        n = 100;
    }

    if (n < 10) {
        add_char(buf, ' ');
        add_char(buf, ' ');
        add_char(buf, '0' + (n % 10));
    } else if (n < 100) {
        add_char(buf, ' ');
        add_char(buf, '0' + (n / 10));
        add_char(buf, '0' + (n % 10));
    } else {
        /* n == 100 */
        add_str(buf, "100");
    }
}

/* Length: 4. */
static void procent(msg_buffer *buf, uint n)
{
    number_small(buf, n);
    add_char(buf, '%');
}

static void multiline_text_end(msg_buffer *buf)
{
    add_str(buf, "\n----\n");
}

/* Multiline text can consist of two part for avoid very big constant strings. */
static void multiline_text(msg_buffer *buf, const char *part1, const char *part2)
{
    if (part1 != NULL) {
        add_str(buf, part1);
    }

    if (part2 != NULL) {
        add_str(buf, part2);
    }

    multiline_text_end(buf);
}

static void header(msg_buffer *buf, const char *str)
{
    add_str(buf, str);
    add_char(buf, '\n');
}

static void list_end(msg_buffer *buf)
{
    add_str(buf, "----\n");
}

/* Length: 10. */
static void nickname(msg_buffer *buf, const char *nick)
{
    spaces(buf, 10 - strlen(nick));
    add_str(buf, nick);
}

/* ==== Prefixes ==== */

static void ok(msg_buffer *buf)
{
    add_char(buf, '+');
    add_char(buf, ' ');
}

static void fail(msg_buffer *buf)
{
    add_char(buf, '-');
    add_char(buf, ' ');
}

static void prefix_proto(msg_buffer *buf)
{
    add_char(buf, 'p');
    add_char(buf, ' ');
}

static void prefix_early_disconnecting(msg_buffer *buf)
{
    add_char(buf, 'e');
    add_char(buf, ' ');
}

static void prefix_round_new(msg_buffer *buf)
{
    add_char(buf, 'r');
    add_char(buf, ' ');
}

static void prefix_month_over(msg_buffer *buf)
{
    add_char(buf, 'm');
    add_char(buf, ' ');
}

static void prefix_round_over(msg_buffer *buf)
{
    add_char(buf, 'o');
    add_char(buf, ' ');
}

static void prefix_nick_changed(msg_buffer *buf)
{
    add_char(buf, 'n');
    add_char(buf, ' ');
}

static void prefix_disconnecting(msg_buffer *buf)
{
    add_char(buf, 'd');
    add_char(buf, ' ');
}

static void prefix_info(msg_buffer *buf)
{
    add_char(buf, 'i');
    add_char(buf, ' ');
}

/* ==== First message ==== */

void msg_first_message(msg_buffer *buf)
{
    prefix_proto(buf);
    add_str(buf, "Protocol version");
    add_char(buf, ':');
    add_char(buf, ' ');
    number(buf, PROTOCOL_VERSION);
    add_char(buf, '\n');
}

/* ==== Prompt ==== */

/* Length: 10. */
static void time(msg_buffer *buf)
{
    static char time[TIME_BUFFER_SIZE];

    if (update_time_buf(time, sizeof(time))) {
        add_str(buf, time);
    } else {
        spaces(buf, 10);
    }
}

/* Length: 13. */
void msg_prompt(msg_buffer *buf)
{
    time(buf);
    add_str(buf, " $ ");
}

/* ==== Common commands stuff ==== */

void msg_cmd_ok(msg_buffer *buf, const char *str)
{
    ok(buf);
    add_str(buf, str);
    add_char(buf, '\n');
}

void msg_cmd_fail(msg_buffer *buf, const char *reason)
{
    fail(buf);
    add_str(buf, reason);
    add_char(buf, '\n');
}

void msg_cmd_wrong(msg_buffer *buf)
{
    static const char error_text[] =
        "Wrong command or argument(s). See \"help\".";

    fail(buf);
    add_str(buf, error_text);
    add_char(buf, '\n');
}

/* ==== help, nick, clients ==== */

void msg_cmd_help_0(msg_buffer *buf)
{
    static const char help_text_1[] = "\
Available commands:\n\
* help [command]\n\
* nick [nick]\n\
* clients\n\
* players\n\
* requests\n\
* market\n\
* build count\n\
* make count\n\
* buy count cost\n\
* sell count cost\n\
* turn\n\
* join\n\
\n";

    static const char help_text_2[] = "\
Command parser is case insensitive and have fun,\n\
when you type commands in uppercase.\n\
\n\
Rules you can see in book [1] or [2]. Also, very\n\
short and not full instructions you can find in\n\
\"on command\" help.\n\
\n\
Books:\n\
[1] Chales Wetherell \"Etudes for programmers\"\n\
    (the book has english and russian releases).\n\
[2] http://www.stolyarov.info/books/gameserv\n\
    (only in russian).";

    ok(buf);
    multiline_text(buf, help_text_1, help_text_2);
}

void msg_cmd_help_1(msg_buffer *buf, command_kind cmd)
{
    static const char ok_text_1[] =
        "TODO: some cmd help.";

    ok(buf);

    switch (cmd) {
    case CMD_EMPTY:
        /* Not possible */
        break;

    case CMD_HELP:
        multiline_text(buf, ok_text_1, NULL);
        break;
    case CMD_NICK:
        multiline_text(buf, ok_text_1, NULL);
        break;
    case CMD_CLIENTS:
        multiline_text(buf, ok_text_1, NULL);
        break;
    case CMD_PLAYERS:
        multiline_text(buf, ok_text_1, NULL);
        break;
    case CMD_REQUESTS:
        multiline_text(buf, ok_text_1, NULL);
        break;
    case CMD_MARKET:
        multiline_text(buf, ok_text_1, NULL);
        break;
    case CMD_BUILD:
        multiline_text(buf, ok_text_1, NULL);
        break;
    case CMD_MAKE:
        multiline_text(buf, ok_text_1, NULL);
        break;
    case CMD_BUY:
        multiline_text(buf, ok_text_1, NULL);
        break;
    case CMD_SELL:
        multiline_text(buf, ok_text_1, NULL);
        break;
    case CMD_TURN:
        multiline_text(buf, ok_text_1, NULL);
        break;
    case CMD_JOIN:
        multiline_text(buf, ok_text_1, NULL);
        break;

    case CMD_WRONG:
    case CMD_PROTOCOL_PARSE_ERROR:
        /* Not possible */
        break;
    }
}

void msg_cmd_nick_0(msg_buffer *buf, const char *nick)
{
    static const char ok_text[] =
        "Your nickname";

    ok(buf);
    add_str(buf, ok_text);
    add_char(buf, ':');
    add_char(buf, ' ');
    nickname(buf, nick);
    add_char(buf, '\n');
}

static void client_state(msg_buffer *buf, const client_t *client)
{
    if (client->player != NULL) {
        add_str(buf, " player");
    } else if (client->want_to_next_round) {
        add_str(buf, "wishful");
    } else {
        spaces(buf, sizeof("wishful") - 1);
    }
}

void msg_cmd_clients(msg_buffer *buf, const client_t *client)
{
    static const char header_text[] =
        "Nickname  State";

    ok(buf);
    header(buf, header_text);

    while (client) {
        nickname(buf, client->nick);
        add_char(buf, ' ');
        client_state(buf, client);
        add_char(buf, '\n');
        client = client->next;
    }

    list_end(buf);
}

/* ==== players ==== */

static void players_t_entry(msg_buffer *buf, const player_t *player)
{
    nickname(buf, player->nick);
    add_char(buf, ' ');
    money(buf, player->money);
    add_char(buf, ' ');
    number_explicit(buf, player->raw);
    add_char(buf, ' ');
    number_explicit(buf, player->prod);
    add_char(buf, ' ');
    number_explicit(buf, player->fact);
    add_char(buf, ' ');
    is(buf, player->turn, "turn");
    add_char(buf, '\n');
}

void msg_cmd_players(msg_buffer *buf, const game_t *game)
{
    static const char header_text[] =
        "Nickname        Money        Raw       Prod       Fact M.turn?";

    uint i;

    ok(buf);
    header(buf, header_text);

    for (i = 0; i < game->players_count; ++i) {
        players_t_entry(buf, game->players[i]);
    }

    list_end(buf);
}

/* ==== requests ==== */

static void req_t1(msg_buffer *buf, const player_t *player)
{
    static const char header_text[] =
"   Raw     Raw cost     Prod    Prod cost  To make    Make cost";

    header(buf, header_text);

    positive(buf, player->req_raw);
    add_char(buf, ' ');
    expense(buf, player->req_raw * player->req_raw_cost);
    add_char(buf, ' ');

    negative(buf, player->req_prod);
    add_char(buf, ' ');
    rise(buf, player->req_prod * player->req_prod_cost);
    add_char(buf, ' ');

    positive(buf, player->req_make);
    add_char(buf, ' ');
    expense(buf, player->req_make * MAKE_COST);
    add_char(buf, '\n');
}

static void req_t2(msg_buffer *buf, const player_t *player)
{
    static const char header_text[] =
"To build   Fact pay 1   Fact pay 2  Raw expense Prod expense Fact expense";

    header(buf, header_text);

    positive(buf, player->req_build);
    add_char(buf, ' ');
    expense(buf, player->req_build * BUILD_COST_1);
    add_char(buf, ' ');
    expense(buf, player->building_fact_3 * BUILD_COST_2);
    add_char(buf, ' ');

    expense(buf, player->raw * RAW_EXPENSE);
    add_char(buf, ' ');
    expense(buf, player->prod * PROD_EXPENSE);
    add_char(buf, ' ');
    expense(buf, player->fact * FACT_EXPENSE);
    add_char(buf, '\n');
}

void msg_cmd_requests(msg_buffer *buf, const player_t *player)
{
    ok(buf);
    req_t1(buf, player);
    req_t2(buf, player);
}

/* ==== market ==== */

static void market_t(msg_buffer *buf, const game_t *game)
{
    static const char header_text[] =
"   Month   Level        Raw     Raw cost       Prod    Prod cost";

    header(buf, header_text);

    number_explicit(buf, game->month);
    add_char(buf, ' ');
    number_explicit_short(buf, game->level + 1); /* level field is index */
    add_char(buf, ' ');
    number_explicit(buf, market_raw(game->level, game->players_count));
    add_char(buf, ' ');
    money(buf, market_raw_cost(game->level));
    add_char(buf, ' ');
    number_explicit(buf, market_prod(game->level, game->players_count));
    add_char(buf, ' ');
    money(buf, market_prod_cost(game->level));
    add_char(buf, '\n');
}

static void next_lvl_t(msg_buffer *buf, const game_t *game)
{
    static const char header_text[] = "Levels probability";

    header(buf, header_text);

    procent(buf, market_level_probability(game->level, 0));
    add_char(buf, ' ');
    procent(buf, market_level_probability(game->level, 1));
    add_char(buf, ' ');
    procent(buf, market_level_probability(game->level, 2));
    add_char(buf, ' ');
    procent(buf, market_level_probability(game->level, 3));
    add_char(buf, ' ');
    procent(buf, market_level_probability(game->level, 4));
    add_char(buf, '\n');
}

void msg_cmd_market(msg_buffer *buf, const game_t *game)
{
    ok(buf);
    market_t(buf, game);
    next_lvl_t(buf, game);
}

/* ===== build, make, buy, sell, turn, join ==== */

/* See common commands stuff upper in this file and PROTO file. */

/* ==== Asynchronous messages ==== */

static void players_list(msg_buffer *buf, const game_t *game)
{
    uint i;

    for (i = 0; i < game->players_count; ++i) {
        nickname(buf, game->players[i]->nick);
        add_char(buf, '\n');
    }

    list_end(buf);
}

static void month_over_t1_entry(msg_buffer *buf, const player_t *player)
{
    nickname(buf, player->nick);
    add_char(buf, ' ');

    positive(buf, player->req_raw);
    add_char(buf, ' ');
    expense(buf, player->req_raw * player->req_raw_cost);
    add_char(buf, ' ');

    negative(buf, player->req_prod);
    add_char(buf, ' ');
    rise(buf, player->req_prod * player->req_prod_cost);
    add_char(buf, '\n');
}

static void month_over_t2_entry(msg_buffer *buf, const player_t *player)
{
    nickname(buf, player->nick);
    add_char(buf, ' ');

    positive(buf, player->req_make);
    add_char(buf, ' ');
    expense(buf, player->req_make * MAKE_COST);
    add_char(buf, ' ');

    positive(buf, player->building_fact_1);
    add_char(buf, ' ');
    expense(buf, player->building_fact_1 * BUILD_COST_1);
    add_char(buf, ' ');
    expense(buf, player->building_fact_4 * BUILD_COST_2);
    add_char(buf, '\n');
}

static void month_over_t3_entry(msg_buffer *buf, const player_t *player)
{
    nickname(buf, player->nick);
    add_char(buf, ' ');


    expense(buf, player->raw * RAW_EXPENSE);
    add_char(buf, ' ');
    expense(buf, player->prod * PROD_EXPENSE);
    add_char(buf, ' ');
    expense(buf, player->fact * FACT_EXPENSE);
    add_char(buf, ' ');

    money(buf, player->money);
    add_char(buf, ' ');

    is(buf, player->money < 0, "bankrupt");
    add_char(buf, '\n');
}

static void month_over_t1(msg_buffer *buf, const game_t *game)
{
    static const char header_text[] =
        "  Nickname      Raw     Raw cost     Prod    Prod cost";

    uint i;

    header(buf, header_text);

    for (i = 0; i < game->players_count; ++i) {
        month_over_t1_entry(buf, game->players[i]);
    }

    list_end(buf);
}

static void month_over_t2(msg_buffer *buf, const game_t *game)
{
    static const char header_text[] =
"  Nickname  To make    Make cost    Build   Fact pay 1   Fact pay 2";

    uint i;

    header(buf, header_text);

    for (i = 0; i < game->players_count; ++i) {
        month_over_t2_entry(buf, game->players[i]);
    }

    list_end(buf);
}

static void month_over_t3(msg_buffer *buf, const game_t *game)
{
    static const char header[] =
"  Nickname  Raw expense Prod expense Fact expense        Money Resolu-n";

    uint i;

    add_str(buf, header);
    add_char(buf, '\n');

    for (i = 0; i < game->players_count; ++i) {
        month_over_t3_entry(buf, game->players[i]);
    }

    list_end(buf);
}

void msg_early_disconnecting(msg_buffer *buf, const char *reason)
{
    static const char text[] = "Disconnecting: ";

    prefix_early_disconnecting(buf);

    add_str(buf, text);
    add_str(buf, reason);
    add_char(buf, '.');

    add_char(buf, '\n');
}

void msg_round_new(msg_buffer *buf, const game_t *game)
{
    static const char text_ok[] =
        "New round! Players:";

    prefix_round_new(buf);
    add_str(buf, text_ok);
    add_char(buf, '\n');

    players_list(buf, game);
}

void msg_month_over(msg_buffer *buf, const game_t *game)
{
    static const char text_ok[] =
        "Month over!";

    prefix_month_over(buf);
    add_str(buf, text_ok);
    add_char(buf, '\n');

    month_over_t1(buf, game);
    month_over_t2(buf, game);
    month_over_t3(buf, game);
}


/* Length: 10. */
static void number_or_winner(msg_buffer *buf, uint month,
    uint is_winner)
{
    if (is_winner) {
        add_str(buf, "   Winner!");
    } else {
        number_explicit(buf, month);
    }
}

/* Length: 21. */
static void players_month_list(msg_buffer *buf, const bankrupt_t *bankrupt)
{
    for ( ; bankrupt != NULL; bankrupt = bankrupt->next) {
        nickname(buf, bankrupt->nick);
        add_char(buf, ' ');
        number_or_winner(buf, bankrupt->month, bankrupt->is_winner);
        add_char(buf, '\n');
    }

    list_end(buf);
}

void msg_round_over(msg_buffer *buf, const bankrupt_t *first_bankrupt)
{
    static const char text_ok[] =
        "Round over! Players and his month of bankrupting:";

    prefix_round_over(buf);
    add_str(buf, text_ok);
    add_char(buf, '\n');

    players_month_list(buf, first_bankrupt);
}

void msg_nick_changed(msg_buffer *buf, const char *old_nick,
    const char *new_nick)
{
    prefix_nick_changed(buf);
    nickname(buf, old_nick);
    add_str(buf, " -> ");
    nickname(buf, new_nick);
    add_char(buf, '\n');
}

void msg_disconnecting(msg_buffer *buf, const char *nick, const char *str)
{
    prefix_disconnecting(buf);
    nickname(buf, nick);
    add_char(buf, ' ');
    add_str(buf, str);
    add_char(buf, '\n');
}

void msg_info(msg_buffer *buf, const char *str)
{
    prefix_info(buf);
    add_str(buf, str);
    add_char(buf, '\n');
}
