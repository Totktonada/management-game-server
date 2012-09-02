#include <string.h>

#include "commands.h"
#include "messages.h"
#include "notifications.h"
#include "market.h"
#include "parser.h"
#include "utils.h"

static void do_cmd_help(msg_buffer *buf, const char *cmd)
{
    if (cmd == NULL) {
        msg_cmd_help_0(buf);
    } else if (get_command_kind(cmd) == CMD_WRONG) {
        msg_cmd_fail(buf, "Unknown command name.");
    } else {
        msg_cmd_help_1(buf, get_command_kind(cmd));
    }
}

static void do_cmd_nick(msg_buffer *buf, server_t *server,
    client_t *client, char *nick)
{
    uint nick_size;

    if (nick == NULL) {
        msg_cmd_nick_0(buf, client->nick);
    } else if (STR_EQUAL(client->nick, nick)) {
        msg_cmd_fail(buf, "You already has this nickname..");
    } else if (get_client_by_nick(server->first_client, nick) != NULL) {
        msg_cmd_fail(buf, "Nickname is holding by other client.");
    } else if (is_valid_nick(nick) != NULL) {
        msg_cmd_fail(buf, is_valid_nick(nick));
    } else {
        msg_cmd_ok(buf, "Your new nickname will be accepted now.");
        msg_nick_changed_clients(server, client->nick, nick);

        free(client->nick);

        nick_size = strlen(nick) + 1;
        client->nick = (char *) malloc(sizeof(char) * nick_size);
        memcpy(client->nick, nick, nick_size);

        if (client->player) {
            client->player->nick = client->nick;
        }
    }
}

static void do_cmd_players(msg_buffer *buf, const game_t *game)
{
    if (game == NULL) {
        msg_cmd_fail(buf, "Game is over.");
    } else {
        msg_cmd_players(buf, game);
    }
}

static void do_cmd_requests(msg_buffer *buf, client_t *client)
{
    if (client->player == NULL) {
        msg_cmd_fail(buf, "This command allowed only for players.");
    } else {
        msg_cmd_requests(buf, client->player);
    }
}

static void do_cmd_market(msg_buffer *buf, const game_t *game)
{
    if (game == NULL) {
        msg_cmd_fail(buf, "Game is over.");
    } else {
        msg_cmd_market(buf, game);
    }
}

static void do_cmd_build(msg_buffer *buf, client_t *client, uint count)
{
    if (client->player == NULL) {
        msg_cmd_fail(buf, "This command allowed only for players.");
    } else if (client->player->turn) {
        msg_cmd_fail(buf, "You already close this month.");
    } else {
        client->player->req_build = count;
        msg_cmd_ok(buf, "Okay, request stored.");
    }
}

static void do_cmd_make(msg_buffer *buf, client_t *client, uint count)
{
    if (client->player == NULL) {
        msg_cmd_fail(buf, "This command allowed only for players.");
    } else if (client->player->turn) {
        msg_cmd_fail(buf, "You already close this month.");
    } else if (count > client->player->fact) {
        msg_cmd_fail(buf, "You have too few *factories*.");
    } else if (count > client->player->raw) {
        msg_cmd_fail(buf, "You have too few *raw*.");
    } else {
        client->player->req_make = count;
        msg_cmd_ok(buf, "Okay, request stored.");
    }
}

static void do_cmd_buy(msg_buffer *buf, game_t *game, client_t *client,
    uint count, uint cost)
{
    if (client->player == NULL) {
        msg_cmd_fail(buf, "This command allowed only for players.");
    } else if (client->player->turn) {
        msg_cmd_fail(buf, "You already close this month.");
    } else if (count > market_raw(game->level, game->players_count)) {
        msg_cmd_fail(buf, "Your *count* is out of range.");
    } else if (cost < market_raw_cost(game->level)) {
        msg_cmd_fail(buf, "Your *cost* is out of range.");
    } else {
        client->player->req_raw = count;
        client->player->req_raw_cost = cost;
        msg_cmd_ok(buf, "Okay, request stored.");
    }
}

static void do_cmd_sell(msg_buffer *buf, game_t *game, client_t *client,
    uint count, uint cost)
{
    if (client->player == NULL) {
        msg_cmd_fail(buf, "This command allowed only for players.");
    } else if (client->player->turn) {
        msg_cmd_fail(buf, "You already close this month.");
    } else if (count > market_prod(game->level, game->players_count)) {
        msg_cmd_fail(buf, "Your *count* is out of range.");
    } else if (count > client->player->prod) {
        msg_cmd_fail(buf, "You have not so much productions.");
    } else if (cost > market_prod_cost(game->level)) {
        msg_cmd_fail(buf, "Your *cost* is out of range.");
    } else {
        client->player->req_prod = count;
        client->player->req_prod_cost = cost;
        msg_cmd_ok(buf, "Okay, request stored.");
    }
}

static void do_cmd_turn(msg_buffer *buf, client_t *client)
{
    if (client->player == NULL) {
        msg_cmd_fail(buf, "This command allowed only for players.");
    } else if (client->player->turn) {
        msg_cmd_fail(buf, "Your request already stored.");
    } else {
        client->player->turn = 1;
        msg_cmd_ok(buf, "Okay, you are close this month.");
    }
}

static void do_cmd_join(msg_buffer *buf, client_t *client)
{
    if (client->want_to_next_round) {
        msg_cmd_fail(buf, "Your request already stored.");
    } else if (client->player) {
        msg_cmd_fail(buf, "This command *not* allowed for players.");
    } else {
        client->want_to_next_round = 1;
        msg_cmd_ok(buf, "Okay, you will be player of next round.");
    }
}

void execute_command(server_t *server, client_t *client, command_t *cmd)
{
    switch (cmd->type) {
    case CMD_EMPTY:
        /* Do nothing. */
        break;
    case CMD_HELP:
        do_cmd_help(&(client->write_buf), cmd->value.str);
        break;
    case CMD_NICK:
        do_cmd_nick(&(client->write_buf), server, client, cmd->value.str);
        break;
    case CMD_CLIENTS:
        msg_cmd_clients(&(client->write_buf), server->first_client);
        break;
    case CMD_PLAYERS:
        do_cmd_players(&(client->write_buf), server->game);
        break;
    case CMD_REQUESTS:
        do_cmd_requests(&(client->write_buf), client);
        break;
    case CMD_MARKET:
        do_cmd_market(&(client->write_buf), server->game);
        break;
    case CMD_BUILD:
        do_cmd_build(&(client->write_buf), client,
            cmd->value.number);
        break;
    case CMD_MAKE:
        do_cmd_make(&(client->write_buf), client,
            cmd->value.number);
        break;
    case CMD_BUY:
        do_cmd_buy(&(client->write_buf), server->game, client,
            cmd->value.number, cmd->value2.number);
        break;
    case CMD_SELL:
        do_cmd_sell(&(client->write_buf), server->game, client,
            cmd->value.number, cmd->value2.number);
       break;
    case CMD_TURN:
        do_cmd_turn(&(client->write_buf), client);
        break;
    case CMD_JOIN:
        do_cmd_join(&(client->write_buf), client);
        break;
    case CMD_WRONG:
        msg_cmd_wrong(&(client->write_buf));
        break;
    case CMD_PROTOCOL_PARSE_ERROR:
        /* Not possible */
        break;
    }
}


