#ifndef MESSAGES_H_SENTRY
#define MESSAGES_H_SENTRY

#include "typedefs.h"
#include "main.h"
#include "msg_buffer.h"
#include "game.h"

void msg_first_message(msg_buffer *buf);

/* Prompt format: "[%H:%M:%S] $ ". See 'update_time_buf'
 * in utils.[ch] for more information. */
void msg_prompt(msg_buffer *buf);

void msg_cmd_ok(msg_buffer *buf, const char* str);

void msg_cmd_fail(msg_buffer *buf, const char* reason);

void msg_cmd_wrong(msg_buffer *buf);

void msg_cmd_help_0(msg_buffer *buf);

void msg_cmd_help_1(msg_buffer *buf, command_kind cmd);

void msg_cmd_nick_0(msg_buffer *buf, const char *nick);

void msg_cmd_clients(msg_buffer *buf, const client_t *client);

void msg_cmd_players(msg_buffer *buf, const game_t *game);

void msg_cmd_requests(msg_buffer *buf, const player_t *player);

void msg_cmd_market(msg_buffer *buf, const game_t *game);

void msg_early_disconnecting(msg_buffer *buf, const char *reason);

void msg_round_new(msg_buffer *buf, const game_t *game);

void msg_month_over(msg_buffer *buf, const game_t *game);

void msg_round_over(msg_buffer *buf, const game_t *game);

void msg_nick_changed(msg_buffer *buf, const char *old_nick,
    const char *new_nick);

void msg_disconnecting(msg_buffer *buf, const char *nick,
    const char *str);

void msg_info(msg_buffer *buf, const char *str);

#endif /* MESSAGES_H_SENTRY */
