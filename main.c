#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "main.h"
#include "arguments.h"
#include "commands.h"
#include "end_month.h"
#include "messages.h"
#include "utils.h"
#include "game.h"
#include "expire.h"
#include "notifications.h"

/* On success return listening socket. Exit on failure. */
static int get_listening_socket(const char *server_ip, uint server_port)
{
    struct sockaddr_in addr;
    int so_reuseaddr_value = 1;
    int listening_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (SOCKET_ERROR(listening_socket)) {
        perror("socket()");
        exit(ES_SYSCALL_FAILED);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(server_port);
    /* addr.sin_addr.s_addr = INADDR_ANY; */
    if (INET_ATON_ERROR(
            inet_aton(server_ip, &(addr.sin_addr))))
    {
        perror("inet_aton()");
        exit(ES_SYSCALL_FAILED);
    }

    if (SETSOCKOPT(setsockopt(listening_socket,
            SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr_value,
            sizeof(so_reuseaddr_value))))
    {
        perror("setsockopt()");
        exit(ES_SYSCALL_FAILED);
    }

    if (BIND_ERROR(bind(listening_socket,
            (struct sockaddr *) &addr, sizeof(addr))))
    {
        perror("bind()");
        exit(ES_SYSCALL_FAILED);
    }

    if (LISTEN_ERROR(listen(listening_socket, 5))) {
        perror("listen()");
        exit(ES_SYSCALL_FAILED);
    }

    return listening_socket;
}

static void update_max_fd(server_t *server)
{
    client_t *cur_client;

    server->max_fd = server->listening_socket;

    for (cur_client = server->first_client;
        cur_client != NULL;
        cur_client = cur_client->next)
    {
        if (server->max_fd < cur_client->fd) {
            server->max_fd = cur_client->fd;
        }
    }
}

static void game_over(server_t *server)
{
    client_t *cur_c;

    /* Flush game */
    free(server->game->players);
    free_bankrupts(server->game->first_bankrupt);
    free(server->game);
    server->game = NULL;

    /* Flush players array entries and NULLed pointers to it. */
    for (cur_c = server->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        if (cur_c->player) {
            free(cur_c->player);
            cur_c->player = NULL;
        }
    }
}

static client_t *new_client(int client_socket)
{
    client_t *client = (client_t *) malloc(sizeof(client_t));

    client->next = NULL;
    client->nick = NULL;
    client->fd = client_socket;
    client->connected = 0;

    /* client->read_buffer now exist, it is okay */
    client->read_available = 0;
    new_parser(&(client->parser));

    msg_buffer_new(&(client->write_buf));

    client->player = NULL;

    client->to_disconnect = 0;
    /* client->reason is undefined when (client->to_disconnect == 0) */
    client->want_to_next_round = 0;

    return client;
}

/* Get listening socket and put it to read_fds.
 * Note: no malloc */
static void new_server(server_t *server, const char *server_ip,
    uint server_port)
{
    server->state = ST_SERVER_START;

    server->first_client = NULL;
    server->last_client = NULL;
    server->clients_count = 0;

    /* Set listening_socket. */
    server->listening_socket =
        get_listening_socket(server_ip, server_port);

    /* Set read_fds. */
    FD_ZERO(&(server->read_fds));
    FD_SET(server->listening_socket, &(server->read_fds));

    /* Set max_fd. */
    update_max_fd(server);

    server->game = NULL;
}

/* Remove client from our structures (if it contain this client). */
static void try_to_unregister_client(server_t *server, client_t *client)
{
    client_t *prev_c = NULL;
    client_t *cur_c = server->first_client;
    client_t *next_c = NULL;

    while (cur_c != NULL) {
        next_c = cur_c->next;

        if (cur_c != client) {
            prev_c = cur_c;
            cur_c = next_c;
            continue;
        }

        FD_CLR(client->fd, &(server->read_fds));
        if (server->max_fd == client->fd) {
            update_max_fd(server);
        }

        /* if (cur_c == server->first_client) */
        if (prev_c == NULL)
            server->first_client = next_c;
        else
            prev_c->next = next_c;

        if (cur_c == server->last_client)
            server->last_client = prev_c;

        --(server->clients_count);
        break;
        /* Game related code see in 'disconnect' func. */
    }
}

/* TODO: use select */
static void write_buffers_all_clients(server_t *server)
{
    client_t *cur_c;

    for (cur_c = server->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        if (! cur_c->connected)
            continue;

        msg_buffer_write(&(cur_c->write_buf), cur_c->fd);
    }
}

/* Disconnect client. */
static void disconnect(server_t *server, client_t *client)
{
    if (! client->to_disconnect)
        return;

    if (client->connected) {
        msg_buffer_write(&(client->write_buf), client->fd);
    } else {
        msg_buffer_clear(&(client->write_buf));
    }

    try_to_unregister_client(server, client);

    /* ==== Game related code ==== */
    if (client->player) {
        player_to_client(server->game, client);
    }
    /* =========================== */

    /* shutdown(filedes, SHUT_RDWR) returns error,
     * if socket has data, that not readed by client,
     * but client is do reset connection (and
     * possibly in other case). Therefore, shutdown
     * fail must be ignored. */
    shutdown(client->fd, SHUT_RDWR);

    client->connected = 0;

    if (CLOSE_ERROR(close(client->fd))) {
        perror("close()");
        exit(ES_SYSCALL_FAILED);
    }

    free(client->nick);
    free(client);
}

static void process_disconnected(server_t *server)
{
    client_t *cur_c = server->first_client;
    client_t *next_c = NULL;

    while (cur_c != NULL) {
        next_c = cur_c->next;

        if (cur_c->to_disconnect) {
            msg_disconnecting_clients(server, cur_c->nick,
                get_reason_string(cur_c->reason));
            disconnect(server, cur_c);
        }

        cur_c = next_c;
    }
}

/* Add new client to our structures. Exit on failure. */
static void process_new_client(server_t *server, uint max_clients)
{
    /* We not save information about client IP and port. */
    int client_socket = accept(server->listening_socket, NULL, NULL);
    client_t *new_c;

    if (ACCEPT_ERROR(client_socket)) {
        perror("accept()");
        exit(ES_SYSCALL_FAILED);
    }

    new_c = new_client(client_socket);
    new_c->connected = 1;

    if (server->clients_count == max_clients) {
        new_c->to_disconnect = 1;
        new_c->reason = REASON_SERVER_FULL;
        msg_first_message(&(new_c->write_buf));
        msg_early_disconnecting(&(new_c->write_buf),
            get_reason_string(new_c->reason));
        disconnect(server, new_c);
        return;
    }

    ++(server->clients_count);
    new_c->nick = first_vacant_nick(server->first_client);
    msg_info_clients(server, "New client: %s", new_c->nick);

    if (server->first_client == NULL) {
        server->last_client = server->first_client = new_c;
    } else {
        server->last_client = server->last_client->next = new_c;
    }

    FD_SET(client_socket, &(server->read_fds));
    if (server->max_fd < client_socket) {
        server->max_fd = client_socket;
    }

    msg_first_message(&(new_c->write_buf));
    msg_prompt(&(new_c->write_buf));
}

static void process_readed_data(server_t *server, client_t *client)
{
    command_t *cmd;

    put_new_data_to_parser(&(client->parser),
        client->read_buffer,
        client->read_available);

    do {
        cmd = get_command(&(client->parser));

        if (cmd == NULL)
            break;

        if (cmd->type == CMD_PROTOCOL_PARSE_ERROR) {
            client->to_disconnect = 1;
            client->reason = REASON_PROTOCOL_PARSE_ERROR;
            break;
        }

        execute_command(server, client, cmd);
        destroy_command(cmd);
        msg_prompt(&(client->write_buf));
    } while (1);
}

static void read_ready_data(server_t *server, fd_set *ready_fds)
{
    client_t *cur_c = server->first_client;
    client_t *next_c;
    int read_value;

    while (cur_c != NULL) {
        next_c = cur_c->next;

        if (! FD_ISSET(cur_c->fd, ready_fds)) {
            cur_c = next_c;
            continue;
        }

        read_value = read(cur_c->fd, cur_c->read_buffer,
            sizeof(char) * READ_BUFFER_SIZE);

        if (READ_ERROR(read_value)) {
            /* Read error is not fatal for server.
             * For example read returns error, if
             * client was unexpectedly terminated
             * (sent a RST packet). */
#ifndef DAEMON
            perror("read()");
#endif
            cur_c->connected = 0;
            cur_c->to_disconnect = 1;
            cur_c->reason = REASON_BY_CLIENT;
        } else if (READ_EOF(read_value)) {
            cur_c->connected = 0;
            cur_c->to_disconnect = 1;
            cur_c->reason = REASON_BY_CLIENT;
        } else {
            cur_c->read_available = read_value;
            process_readed_data(server, cur_c);
        }

        cur_c = next_c;
    } /* while */
}

static void new_round(server_t *server, uint players_count)
{
    player_t **players = NULL;
    client_t *cur_c;
    uint i = 0;

    players = (player_t **) malloc(sizeof(player_t *) * players_count);

    for (cur_c = server->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        if (cur_c->want_to_next_round) {
            cur_c->want_to_next_round = 0;
            players[i] = cur_c->player = new_player(cur_c->nick);
            ++i;
        }
    }

    server->game = new_game(players, players_count);
    server->state = ST_SERVER_GAME;
}

static void process_time_events(server_t *server, expire_t *expire,
    uint time_before_round)
{
    uint players_count = 0;
    client_t *cur_c;

    for (cur_c = server->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        players_count += cur_c->want_to_next_round;
    }

    if (expire->all > 0) {
        msg_info_clients(server, "Round starts via %d sec.", expire->all);
    } else if (players_count > 1) {
        new_round(server, players_count);
        msg_round_new_clients(server);
    } else {
        expire_set(expire, time_before_round);
        server->state = ST_SERVER_COUNTER;
        msg_info_clients(server,
            "New round starts via %d sec.", expire->all);
    }
}

/* Wait new client, data from exist clients or expire of time period. */
static int wait_for_events(fd_set *ready_fds, int max_fd, expire_t *expire)
{
    struct timeval tv;
    time_t delta_time;
    int select_value;

    if (expire->cur < 0) {
        return select(max_fd + 1, ready_fds, NULL, NULL, NULL);
    }

    tv.tv_sec = expire->cur;

    if (tv.tv_sec == 0) {
        /* For avoid high network loading. */
        tv.tv_usec = 100000; /* 0.1 seconds */
    } else {
        tv.tv_usec = 0;
    }

    time(&delta_time); /* Save old time. */

    select_value = select(max_fd + 1, ready_fds, NULL, NULL, &tv);

    delta_time = time(NULL) - delta_time; /* Calculate delta time. */

    if (SELECT_TIMEOUT(select_value)) {
        if (expire->all <= delta_time) {
            expire_stop(expire);
        } else {
            expire_dec(expire, delta_time);
        }
    } else {
        expire_dec(expire, delta_time);
    }

    return select_value;
}

/* Must be called, if clients count may be increased. */
static void maybe_increased_callback(server_t *server,
    expire_t *expire, arguments_t *arguments)
{
    if (server->state == ST_SERVER_START
        && server->clients_count > 1)
    {
        expire_set(expire, arguments->time_before_round);
        server->state = ST_SERVER_COUNTER;
        msg_info_clients(server,
            "New round starts via %d sec.", expire->all);
    }
}

/* Must be called, if clients or players count may be decreased. */
static void maybe_decreased_callback(server_t *server,
    expire_t *expire, arguments_t *arguments)
{
    if (server->state == ST_SERVER_COUNTER
        && server->clients_count <= 1)
    {
        expire_stop(expire);
        server->state = ST_SERVER_START;
        msg_info_clients(server,
            "New-round backward counter are stopped.");
    } else if (server->state == ST_SERVER_GAME
        && server->game->players_count <= 1)
    {
        /* Add winner to bankrupts for clients notifications. */
        if (server->game->players_count == 1) {
            add_winner_to_bankrupts(server->game);
        }

        msg_round_over_clients(server);
        game_over(server);

        if (server->clients_count <= 1) {
            server->state = ST_SERVER_START;
            msg_info_clients(server, "Game over!");
        } else {
            expire_set(expire, arguments->time_before_round);
            server->state = ST_SERVER_COUNTER;
            msg_info_clients(server,
                "Game over! New round starts via %d sec.", expire->all);
        }
    }
}

int main(int argc, const char **argv)
{
    arguments_t arguments;
    expire_t expire;
    server_t server;
    fd_set ready_fds;
    int select_value;

    process_arguments(&arguments, argv + 1);

    if (arguments.server_ip == NULL) {
        fprintf(stderr, "Server IP omited. Exiting...\n");
        exit(ES_WRONG_ARGS);
    }

#ifdef DAEMON
    if (isatty(STDOUT_FILENO)) {
        printf("Fork to background...\n");
    }
    daemon(0, 0);
#endif

    expire_stop(&expire);
    new_server(&server, arguments.server_ip, arguments.server_port);

    do {
        /* Copy all available file descriptors. */
        ready_fds = server.read_fds;

        select_value =
            wait_for_events(&ready_fds, server.max_fd, &expire);

        /* Error */
        if (SELECT_ERROR(select_value)) {
            perror("select()");
            exit(ES_SYSCALL_FAILED);
        }

        /* Timeout */
        if (SELECT_TIMEOUT(select_value)) {
            process_time_events(&server, &expire, arguments.time_before_round);
        }

        /* New client */
        if (FD_ISSET(server.listening_socket, &ready_fds)) {
            process_new_client(&server, arguments.max_clients);
            maybe_increased_callback(&server, &expire, &arguments);
        }

        /* Data from client */
        read_ready_data(&server, &ready_fds);

        /* Process disconnected clients */
        process_disconnected(&server);

        maybe_decreased_callback(&server, &expire, &arguments);

        /* Process requests */
        if (ready_to_new_month(server.game)) {
            end_month(server.game);
            msg_month_over_clients(&server);
            bankrupts_to_clients(&server);
            new_month(server.game);
        }

        maybe_decreased_callback(&server, &expire, &arguments);

        write_buffers_all_clients(&server);
    } while (1);

    return 0;
}
