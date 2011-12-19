#ifndef MAIN_H_SENTRY
#define MAIN_H_SENTRY

/* For inet_aton () and daemon (). */
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

#define DEFAULT_LISTENING_PORT 37187
#define READ_BUFFER_SIZE 1024

#include "parser.h"
#include "runner.h"
#include "utils.h"

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

typedef struct client_info {
    int fd;
    char read_buffer[READ_BUFFER_SIZE];
    int read_available;
    parser_info pinfo;
    user_game_data user_gdata;
    struct client_info *next;
} client_info;

typedef struct server_info {
    int listening_port;
    int listening_socket;
    fd_set read_fds;
    int max_fd;
    client_info *first_client;
    client_info *last_client;
    game_data gdata;
} server_info;

#include "parameters.h"

#endif
