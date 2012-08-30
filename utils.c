#include <time.h>

#include "utils.h"
#include "parser.h"
#include "main.h"

#ifndef DAEMON
#include <stdio.h>
#endif

#define LOCALTIME_ERROR(localtime_value) ((localtime_value) == NULL)
#define STRFTIME_ERROR(strftime_value) ((strftime_value) == 0)

command_kind get_command_kind(const char *str)
{
    static const char *command_strings[] = {
        "\n", "help", "nick", "clients",
        "players", "requests", "market", "build",
        "make", "buy", "sell", "turn",
        "join", "", ""
    };

    uint i;

    for (i = 0; i < sizeof(command_strings) / sizeof(char *); ++i) {
        if (STR_EQUAL_CASE_INS(str, command_strings[i]))
            return (command_kind) i;
    }

    return CMD_WRONG;
}

uint get_random(uint max_value)
{
    return (uint) (max_value *
        (rand() / (RAND_MAX + 1.0)));
}

uint log10i(uint number)
{
    uint i = 0;
    uint del = 1;

    while (number / 10 >= del) {
        ++i;
        del *= 10;
    }

    return i;
}

uint number_to_str(char *buf, uint number)
{
    uint i = 0;
    uint del = 1;

    while (number / 10 >= del) {
        del *= 10;
    }

    do {
        buf[i] = '0' + ((number / del) % 10);
        del /= 10;
        ++i;
    } while (del > 0);

    buf[i] = '\0';
    return i;
}

char *first_vacant_nick(const client_t *first_client)
{
    /* Why 12? See comment to 'number_to_str' func.
     * First position reserved for 'p' symbol. */
    char *buf = (char *) malloc(12 * sizeof(char));
    const client_t *cur_c;
    int nick_number = 0;
    int nick_found;

    *buf = 'p';

    do {
        nick_found = 0;
        cur_c = first_client;
        number_to_str(buf + 1, nick_number);

        while (cur_c != NULL && !nick_found) {
            if (cur_c->nick == NULL) {
                nick_found = 0;
            } else {
                nick_found = STR_EQUAL(buf,
                    cur_c->nick);
            }
            cur_c = cur_c->next;
        }

        ++nick_number;
    } while (nick_found);

    return buf;
}

uint update_time_buf(char *time_buf, uint buf_size)
{
    time_t unix_time;
    struct tm *localtime_var;
    int ok = 1;
    int strftime_value;

    if (time_buf == NULL || buf_size == 0) {
        return 0;
    }

    time(&unix_time);
    localtime_var = localtime(&unix_time);

    if (LOCALTIME_ERROR(localtime_var)) {
        perror("localtime()");
        ok = 0;
    }

    if (ok) {
        strftime_value = strftime(time_buf, buf_size,
            "[%H:%M:%S] ", localtime_var);
    }

    if (ok && STRFTIME_ERROR(strftime_value)) {
#ifndef DAEMON
        fprintf(stderr, "strftime() failed. See %s:%d.\n",
            __FILE__, __LINE__);
#endif
        ok = 0;
    }

    if (!ok) {
        time_buf[0] = '\0';
    }

    return ok;
}

client_t *get_client_by_nick(client_t *first_client,
    const char *nick)
{
    client_t *cur_c;

    for (cur_c = first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        if (STR_EQUAL(nick, cur_c->nick)) {
            return cur_c;
        }
    }

    return NULL;
}

const char *is_valid_nick(const char *nick)
{
    static const char error1_text[] =
        "Nickname too long (more than 10 symbols).";
    static const char error2_text[] =
        "Nickname contain forbidden symbol ':'.";

    uint i;

    for (i = 0; nick[i] != '\0'; ++i) {
        if (i == 10) {
            return error1_text;
        }
        if (nick[i] == ':') {
            return error2_text;
        }
    }

    return NULL;
}

const char *get_reason_string(disconnect_reasons reason)
{
    switch (reason) {
    case REASON_SERVER_FULL:
        return "server full";
    case REASON_BY_CLIENT:
        return "closed by client";
    case REASON_PROTOCOL_PARSE_ERROR:
        return "protocol parse error";
    }

    /* Impossible */
    return NULL;
}
