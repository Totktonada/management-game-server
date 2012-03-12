#ifndef MAIN_H_SENTRY
#define MAIN_H_SENTRY

/* For inet_aton() and daemon(). */
#define _BSD_SOURCE

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

#define DEFAULT_LISTENING_PORT 37187
#define READ_BUFFER_SIZE 1024
#define TIME_BETWEEN_TIME_EVENTS 10
#define WARNINGS_BEFORE_ROUND 6

#include "parser.h"
#include "utils.h"
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
#define SHUTDOWN_ERROR(shutdown_value) ((shutdown_value) == -1)
#define CLOSE_ERROR(close_value) ((close_value) == -1)
#define CHDIR_ERROR(chdir_value) ((chdir_value) == -1)
#define SETSID_ERROR(setsid_value) ((setsid_value) == -1)

typedef enum disconnect_reasons {
    REASON_SERVER_FULL,
    REASON_BY_CLIENT,
    REASON_PROTOCOL_PARSE_ERROR
} disconnect_reasons;

typedef struct client_info {
    /* Common */
    struct client_info *next;
    int fd;
    int connected;
    int to_disconnect;
    disconnect_reasons reason;
    unsigned int in_round:1;
    unsigned int want_to_next_round:1;
    /* Read */
    char read_buffer[READ_BUFFER_SIZE];
    int read_available;
    parser_info pinfo;
    /* Write */
    msg_buffer write_buf;
    /* Client game data */
    char *nick;
    int money;
    unsigned int raw_count;
    unsigned int prod_count;
    unsigned int factory_count;
    unsigned int step_completed:1;
    /* Requests */
    unsigned int build_factory_count;
    unsigned int make_prod_count;
    /* Building factories. Count of
     * 1, 2, 3, 4 month old factories. */
    unsigned int building_factory_1;
    unsigned int building_factory_2;
    unsigned int building_factory_3;
    unsigned int building_factory_4;
    /* For write_client_information() */
    unsigned int buy_raw_count;
    unsigned int buy_raw_cost;
    unsigned int sell_prod_count;
    unsigned int sell_prod_cost;
} client_info;

typedef enum request_type {
    REQUEST_RAW,
    REQUEST_PROD
} request_type;

typedef struct request {
    struct request *next;
    client_info *client;
    unsigned int count;
} request;

typedef struct request_group {
    struct request_group *next;
    unsigned int cost;
    unsigned int req_count;
    request *first_req;
} request_group;

typedef struct server_info {
    /* Connecting data */
    const char *server_ip;
    int listening_port;
    int listening_socket;
    fd_set read_fds;
    int max_fd;
    client_info *first_client;
    client_info *last_client;
    int clients_count;
    int max_clients; /* -1 is without limitation */
    time_t time_to_next_event;
    unsigned int backward_warnings_counter;
    /* Prompt and time data.
     * Prompt format:     "\n[%H:%M:%S] "
     * Async. msg format: "\n<%H:%M:%S> "
     * See update_time_buf() in utils.c
     * for more information. */
    char prefix_prompt[16];
    char prefix_async_msg[16];
    /* Game data */
    int players_count;
    unsigned int in_round:1;
    unsigned int step;
    unsigned int level; /* [0-4] */
    request_group *buy_raw;
    request_group *sell_prod;
} server_info;

void mark_client_to_disconnect(client_info *client,
    disconnect_reasons reason);
void process_end_round(server_info *sinfo);
void try_to_deferred_start_round(server_info *sinfo);

#include "parameters.h"
#include "game.h"

#endif
