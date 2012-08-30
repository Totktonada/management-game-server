#ifndef MAIN_H_SENTRY
#define MAIN_H_SENTRY

#include <sys/time.h>

#include "parser.h"
#include "game.h"
#include "msg_buffer.h"

/* Exit status, if one of next system calls failed:
 * fork/socket/inet_aton/bind/listen/select/accept. */
#define ES_SYSCALL_FAILED 1

#define FORK_ERROR(fork_value) ((fork_value) == -1)
#define FORK_PARENT(fork_value) ((fork_value) > 0)
#define SOCKET_ERROR(socket_value) ((socket_value) == -1)
#define SETSOCKOPT(setsockopt_value) ((setsockopt_value) == -1)
#define BIND_ERROR(bind_value) ((bind_value) == -1)
#define INET_ATON_ERROR(inet_aton_value) ((inet_aton_value) == 0)
#define LISTEN_ERROR(listen_value) ((listen_value) == -1)
#define ACCEPT_ERROR(accept_value) ((accept_value) == -1)
#define SELECT_ERROR(select_value) ((select_value) == -1)
#define SELECT_TIMEOUT(select_value) ((select_value) == 0)
#define READ_ERROR(read_value) ((read_value) == -1)
#define READ_EOF(read_value) ((read_value) == 0)
#define CLOSE_ERROR(close_value) ((close_value) == -1)
#define CHDIR_ERROR(chdir_value) ((chdir_value) == -1)
#define SETSID_ERROR(setsid_value) ((setsid_value) == -1)

#define READ_BUFFER_SIZE 1024

typedef enum disconnect_reasons {
    REASON_SERVER_FULL,
    REASON_BY_CLIENT,
    REASON_PROTOCOL_PARSE_ERROR
} disconnect_reasons;

typedef struct client_t {
    /* Common */
    struct client_t *next;
    char *nick;
    int fd;
    int connected;
    /* Read */
    char read_buffer[READ_BUFFER_SIZE];
    int read_available;
    parser_t parser;
    /* Write */
    msg_buffer write_buf;
    /* Player */
    player_t *player;
    /* Requests */
    uint to_disconnect:1;
    disconnect_reasons reason;
    uint want_to_next_round:1;
} client_t;

typedef enum server_state_t {
    ST_SERVER_START,
    ST_SERVER_COUNTER,
    ST_SERVER_GAME
} server_state_t;

typedef struct server_t {
    /* State */
    server_state_t state;
    /* Clients */
    client_t *first_client;
    client_t *last_client;
    uint clients_count;
    /* Connecting data */
    int listening_socket;
    fd_set read_fds;
    int max_fd;
    /* Game data */
    game_t *game;
} server_t;

typedef enum msg_type {
    MSG_RESPONCE,
    MSG_ASYNC
} msg_type;

#endif
