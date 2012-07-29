#include "main.h"

/* Prompt format: "\n[%H:%M:%S] "
 * See update_time_buf() in utils.c
 * for more information. */
void add_prompt(client_info *client)
{
    char prompt[TIME_BUFFER_SIZE]; /* TODO: static? */

    update_time_buf(prompt, sizeof(prompt),
        TIME_BUF_PROMPT_AND_RESPONCE);
    ADD_S_STRLEN_MAKE_COPY(&(client->write_buf), prompt);
    ADD_S_STRLEN(&(client->write_buf), client->nick);
    ADD_S(&(client->write_buf), " $ ");
}

/* Async. msg format: "\n<%H:%M:%S> "
 * See update_time_buf() in utils.c
 * for more information. */
void add_msg_head(msg_buffer *write_buf, char *head_str,
    msg_type type)
{
    char time_buf[TIME_BUFFER_SIZE]; /* TODO: static? */

    if (type == MSG_ASYNC) {
        update_time_buf(time_buf,
            sizeof(time_buf), TIME_BUF_ASYNC_MSG);
    } else { /* type == MSG_RESPONCE */
        update_time_buf(time_buf,
            sizeof(time_buf), TIME_BUF_PROMPT_AND_RESPONCE);
    }

    ADD_S_STRLEN_MAKE_COPY(write_buf, time_buf);
    /* TODO: remove strlen. */
    ADD_S_STRLEN(write_buf, head_str);
}

void notify_client_about_disconnect_reason(client_info *client)
{
    if (! client->connected)
        return;

    add_msg_head(&(client->write_buf),
        "[Disconnecting]\n", MSG_RESPONCE);
    ADD_S(&(client->write_buf), "Disconnecting... Reason: ");

    switch (client->reason) {
    case REASON_SERVER_FULL:
        ADD_S(&(client->write_buf), "server full.\n");
        break;
    case REASON_BY_CLIENT:
        /* Not possible. */
        break;
    case REASON_PROTOCOL_PARSE_ERROR:
        ADD_S(&(client->write_buf), "protocol parse error.\n");
        break;
    }
}

void notify_all_clients_about_disconnect(const server_info *sinfo,
    client_info *client)
{
    client_info *cur_c;

    if (client->reason == REASON_SERVER_FULL)
        return;

    for (cur_c = sinfo->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        if (cur_c == client)
            continue;

        add_msg_head(&(cur_c->write_buf),
            "[Disconnecting]\n", MSG_ASYNC);
        ADD_S(&(cur_c->write_buf), "Client ");
        ADD_S_STRLEN_MAKE_COPY(&(cur_c->write_buf), client->nick);
        ADD_S(&(cur_c->write_buf), " disconnected.\nReason: ");

        switch (client->reason) {
        case REASON_SERVER_FULL:
            /* Not possible. */
            break;
        case REASON_BY_CLIENT:
            ADD_S(&(cur_c->write_buf),
                "connection closed by client.\n");
            break;
        case REASON_PROTOCOL_PARSE_ERROR:
            ADD_S(&(cur_c->write_buf),
                "protocol parse error.\n");
            break;
        }
    }
}

/* On success set sinfo->listening_socket
 * to proper value.
 * Exit on failure. */
void get_listening_socket(server_info *sinfo)
{
    struct sockaddr_in addr;
    int so_reuseaddr_value = 1;

    sinfo->listening_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (SOCKET_ERROR(sinfo->listening_socket)) {
        perror("socket()");
        exit(ES_SYSCALL_FAILED);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(sinfo->listening_port);
    /* addr.sin_addr.s_addr = INADDR_ANY; */
    if (INET_ATON_ERROR(
            inet_aton(sinfo->server_ip, &(addr.sin_addr))))
    {
        perror("inet_aton()");
        exit(ES_SYSCALL_FAILED);
    }

    if (SETSOCKOPT(setsockopt(sinfo->listening_socket,
            SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr_value,
            sizeof(so_reuseaddr_value))))
    {
        perror("setsockopt()");
        exit(ES_SYSCALL_FAILED);
    }

    if (BIND_ERROR(bind(sinfo->listening_socket,
            (struct sockaddr *) &addr, sizeof(addr))))
    {
        perror("bind()");
        exit(ES_SYSCALL_FAILED);
    }

    if (LISTEN_ERROR(listen(sinfo->listening_socket, 5))) {
        perror("listen()");
        exit(ES_SYSCALL_FAILED);
    }
}

void update_max_fd(server_info *sinfo)
{
    client_info *cur_client;

    sinfo->max_fd = sinfo->listening_socket;

    for (cur_client = sinfo->first_client;
        cur_client != NULL;
        cur_client = cur_client->next)
    {
        if (sinfo->max_fd < cur_client->fd)
            sinfo->max_fd = cur_client->fd;
    }
}

client_info *new_client_info(int client_socket)
{
    client_info *client = (client_info *)
        malloc(sizeof(client_info));
    client->next = NULL;
    client->nick = NULL;
    client->fd = client_socket;
    client->connected = 0;
    client->to_disconnect = 0;
    /* client->reason is undefined when (client->to_disconnect == 0) */
    client->in_round = 0;
    client->want_to_next_round = 0;
    /* client->read_buffer now exist, it is okay */
    client->read_available = 0;
    new_parser_info(&(client->pinfo));
    new_msg_buffer(&(client->write_buf));
    new_client_game_data(client);
    return client;
}

/* Get listening socket and put it to read_fds.
 * Note: no malloc */
void init_server_info(server_info *sinfo)
{
    get_listening_socket(sinfo);
    FD_ZERO(&(sinfo->read_fds));
    FD_SET(sinfo->listening_socket, &(sinfo->read_fds));
    sinfo->first_client = NULL;
    update_max_fd(sinfo);
    sinfo->clients_count = 0;
    new_game_data(sinfo);
    process_end_round(sinfo);
}

void mark_client_to_disconnect(client_info *client,
    disconnect_reasons reason)
{
    client->to_disconnect = 1;
    client->reason = reason;
}

void warn_all(server_info *sinfo)
{
    client_info *cur_c;
    unsigned int remain_time =
        sinfo->backward_warnings_counter * TIME_BETWEEN_TIME_EVENTS;

    for (cur_c = sinfo->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        add_msg_head(&(cur_c->write_buf),
            "[Rounds]\n", MSG_ASYNC);
        ADD_S(&(cur_c->write_buf),
"Time remaining to the next game round: ");
        ADD_N(&(cur_c->write_buf), remain_time);
        ADD_S(&(cur_c->write_buf), " (sec)\n");
        if (cur_c->want_to_next_round) {
            ADD_S(&(cur_c->write_buf),
"Your request to participating in next game round is stored.\n");
        } else {
            ADD_S(&(cur_c->write_buf),
"You *not* send request for participating in this round.\n\
You can do it by join command (see \"help join\").\n");
        }
    }

    --(sinfo->backward_warnings_counter);
}

void notify_all_round_countdown_cancel(server_info *sinfo)
{
    client_info *cur_c;

    for (cur_c = sinfo->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        add_msg_head(&(cur_c->write_buf),
            "[Rounds]\n", MSG_ASYNC);
        ADD_S(&(cur_c->write_buf),
"Time countdown for a next round can not be\n\
started because count of clients less then two.\n");
    }
}

void notify_all_round_less_two_players(server_info *sinfo)
{
    client_info *cur_c;

    for (cur_c = sinfo->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        add_msg_head(&(cur_c->write_buf),
            "[Rounds]\n", MSG_ASYNC);
        ADD_S(&(cur_c->write_buf),
"Round can not be started because count of\n\
clients, which send request for participating\n\
in game round, less then two.\n");
    }
}

void try_to_deferred_start_round(server_info *sinfo)
{
    if (!sinfo->in_round
        && (sinfo->time_to_next_event < 0)
        && (sinfo->clients_count > 1))
    {
        sinfo->time_to_next_event = TIME_BETWEEN_TIME_EVENTS;
        sinfo->backward_warnings_counter = WARNINGS_BEFORE_ROUND;
        warn_all(sinfo);
    }
}

void try_to_stop_deferred_start_round(server_info *sinfo)
{
    if (sinfo->clients_count <= 1) {
        sinfo->time_to_next_event = -1;
        sinfo->backward_warnings_counter = 0;
        notify_all_round_countdown_cancel(sinfo);
    }
}

/* Remove client from our structures (if it contain this client). */
void try_to_unregister_client(server_info *sinfo, client_info *client)
{
    client_info *prev_c = NULL;
    client_info *cur_c = sinfo->first_client;
    client_info *next_c = NULL;

    while (cur_c != NULL) {
        next_c = cur_c->next;

        if (cur_c == client) {
            FD_CLR(client->fd, &(sinfo->read_fds));
            if (sinfo->max_fd == client->fd) {
                update_max_fd(sinfo);
            }

            /* if (cur_c == sinfo->first_client) */
            if (prev_c == NULL)
                sinfo->first_client = next_c;
            else
                prev_c->next = next_c;

            if (cur_c == sinfo->last_client)
                sinfo->last_client = prev_c;

            --(sinfo->clients_count);
            if (cur_c->in_round) {
                /* TODO: maybe separate message for players. */
                --(sinfo->players_count);
                if (sinfo->players_count <= 1) {
                    process_end_round(sinfo);
                    try_to_deferred_start_round(sinfo);
                }
            }
            if (! sinfo->in_round) {
                try_to_stop_deferred_start_round(sinfo);
            }
            break;
        }

        prev_c = cur_c;
        cur_c = next_c;
    }
}

#if 0
/* TODO: maybe add prompt to end? */
void add_async_prefixes(server_info *sinfo, client_info *client)
{
    client_info *cur_c;

    for (cur_c = sinfo->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
#if 0
        /* TODO: Necessary? */
        if (! cur_c->connected)
            continue;
#endif
        if (is_msg_buffer_empty(&(cur_c->write_buf)))
            continue;

        /* Add prefix for asynchronous messages. */
        if (cur_c != client) {
            update_time_buf(sinfo->head_async_msg,
                sizeof(sinfo->head_async_msg), TIME_BUF_ASYNC_MSG);
            ADD_PREFIX_STRLEN(&(cur_c->write_buf),
                sinfo->head_async_msg);
        }
    }
}
#endif

/* TODO: use select */
void write_buffers_all_clients(server_info *sinfo)
{
    client_info *cur_c;

    for (cur_c = sinfo->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
#if 0
        /* TODO: Necessary? */
        if (! cur_c->connected)
            continue;
#endif
        if (is_msg_buffer_empty(&(cur_c->write_buf)))
            continue;

        write_msg_buffer(&(cur_c->write_buf), cur_c->fd);
    }
}

/* Disconnect client. */
void try_to_disconnect(server_info *sinfo, client_info *client)
{
    if (! client->to_disconnect)
        return;

    notify_client_about_disconnect_reason(client);
    notify_all_clients_about_disconnect(sinfo, client);
#if 0
    add_async_prefixes(sinfo, client);
#endif
    write_msg_buffer(&(client->write_buf), client->fd);

    try_to_unregister_client(sinfo, client);

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

char *first_vacant_nick(client_info *first_client)
{
    /* Why 12? See comment to number_to_str().
     * First position reserved for 'p' symbol. */
    char *buf = (char *) malloc(12 * sizeof(char));
    client_info *cur_c;
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

void notify_client_connected(server_info *sinfo,
    client_info *new_client)
{
    client_info *cur_c;

    /* Messages for all clients. */
    for (cur_c = sinfo->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        if (cur_c->in_round)
            continue;

        add_msg_head(&(cur_c->write_buf),
            "[Connected]\n", MSG_ASYNC);
        ADD_S(&(cur_c->write_buf),
            "Connected new client. Username: ");
        ADD_S_STRLEN(&(cur_c->write_buf), new_client->nick);
        ADD_S(&(cur_c->write_buf), "\n");

        ADD_SNS(&(cur_c->write_buf),
            "Total connected clients: ",
            sinfo->clients_count, "\n");
    }
}

/* Add new client to our structures.
 * Exit on failure. */
void process_new_client(server_info *sinfo, int listening_socket)
{
    /* We not save information about client IP and port. */
    int client_socket = accept(listening_socket, NULL, NULL);
    client_info *new_client;

    if (ACCEPT_ERROR(client_socket)) {
        perror("accept()");
        exit(ES_SYSCALL_FAILED);
    }

    new_client = new_client_info(client_socket);
    new_client->connected = 1;

    if (sinfo->clients_count == sinfo->max_clients) {
        mark_client_to_disconnect(new_client, REASON_SERVER_FULL);
        try_to_disconnect(sinfo, new_client);
        return;
    }

    ++(sinfo->clients_count);
    new_client->nick = first_vacant_nick(sinfo->first_client);
    notify_client_connected(sinfo, new_client);
#if 0
    add_async_prefixes(sinfo, new_client);
#endif

    if (sinfo->first_client == NULL) {
        sinfo->last_client = sinfo->first_client = new_client;
    } else {
        sinfo->last_client = sinfo->last_client->next = new_client;
    }

    FD_SET(client_socket, &(sinfo->read_fds));
    if (sinfo->max_fd < client_socket) {
        sinfo->max_fd = client_socket;
    }

    try_to_deferred_start_round(sinfo);
    add_prompt(new_client);
}

void process_end_round(server_info *sinfo)
{
    client_info *cur_c;

    for (cur_c = sinfo->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        cur_c->in_round = 0;
    }

    sinfo->players_count = 0;
    sinfo->in_round = 0;
    sinfo->time_to_next_event = -1;
    sinfo->backward_warnings_counter = 0;
}

void process_readed_data(server_info *sinfo, client_info *client)
{
    command *cmd;
    int destroy_str;

    put_new_data_to_parser(&(client->pinfo),
        client->read_buffer,
        client->read_available);

    do {
        cmd = get_cmd(&(client->pinfo));
        if (cmd == NULL)
            break;
        if (cmd->type == CMD_PROTOCOL_PARSE_ERROR) {
            mark_client_to_disconnect(client,
                REASON_PROTOCOL_PARSE_ERROR);
            break;
        }
#ifndef DAEMON
        print_cmd(cmd);
#endif
        destroy_str = execute_cmd(sinfo, client, cmd);
        destroy_cmd(cmd, destroy_str);
#if 0
        add_async_prefixes(sinfo, client);
#endif
        add_prompt(client);
    } while (1);
}

void read_ready_data(server_info *sinfo, fd_set *ready_fds)
{
    client_info *cur_c = sinfo->first_client;
    client_info *next_c;
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
            mark_client_to_disconnect(cur_c, REASON_BY_CLIENT);
        } else if (READ_EOF(read_value)) {
            cur_c->connected = 0;
            mark_client_to_disconnect(cur_c, REASON_BY_CLIENT);
        } else {
            cur_c->read_available = read_value;
            process_readed_data(sinfo, cur_c);
        }

        try_to_disconnect(sinfo, cur_c);
        cur_c = next_c;
    } /* while */
}

void try_to_start_new_round(server_info *sinfo)
{
    client_info *cur_c;

    sinfo->time_to_next_event = -1;
    sinfo->backward_warnings_counter = 0;

    if (sinfo->players_count <= 1) {
        notify_all_round_less_two_players(sinfo);
        try_to_deferred_start_round(sinfo);
        return;
    }

    new_game_data(sinfo);

    for (cur_c = sinfo->first_client;
        cur_c != NULL;
        cur_c = cur_c->next)
    {
        add_msg_head(&(cur_c->write_buf),
            "[Rounds]\n", MSG_ASYNC);

        if (cur_c->want_to_next_round) {
            cur_c->in_round = 1;
            cur_c->want_to_next_round = 0;
            ADD_S(&(cur_c->write_buf),
"Game ready! You are player of this game round.\n");
            new_client_game_data(cur_c);
            /* TODO: write count and list of players. */
        } else {
            ADD_S(&(cur_c->write_buf),
"Game ready! You are *not* player of this game round.\n");
        }
    }

    sinfo->in_round = 1;
}

void process_time_events(server_info *sinfo)
{
    if (sinfo->backward_warnings_counter > 0) {
        sinfo->time_to_next_event = TIME_BETWEEN_TIME_EVENTS;
        warn_all(sinfo);
    } else {
        /* sinfo->backward_warnings_counter == 0 */
        try_to_start_new_round(sinfo);
    }
#if 0
    add_async_prefixes(sinfo, NULL);
#endif
}

#ifdef DAEMON_ALT
void daemon_alt()
{
    int fork_value;

    if (CHDIR_ERROR(chdir("/"))) {
        perror("chdir()");
        exit(ES_SYSCALL_FAILED);
    }

    if (CLOSE_ERROR(close(STDIN_FILENO))
        || CLOSE_ERROR(close(STDOUT_FILENO))
        || CLOSE_ERROR(close(STDERR_FILENO)))
    {
        perror("close()");
        exit(ES_SYSCALL_FAILED);
    }

    if (CLOSE_ERROR(open("/dev/null", O_WRONLY))
        || CLOSE_ERROR(open("/dev/null", O_RDONLY))
        || CLOSE_ERROR(open("/dev/null", O_RDONLY)))
    {
        perror("open()");
        exit(ES_SYSCALL_FAILED);
    }

    fork_value = fork();
    if (FORK_ERROR(fork_value)) {
        perror("fork()");
        exit(ES_SYSCALL_FAILED);
    } else if (FORK_PARENT(fork_value)) {
        exit(0);
    }

    if (SETSID_ERROR(setsid())) {
        perror("setsid()");
        exit(ES_SYSCALL_FAILED);
    }
}
#endif

int main(int argc, char **argv, char **envp)
{
    server_info sinfo;
    int select_value;
    fd_set ready_fds;
    struct timeval tv;
    time_t delta_time;

    sinfo.listening_port = DEFAULT_LISTENING_PORT;
    process_cmd_line_parameters(&sinfo, argv + 1);

#ifdef DAEMON
    printf("Fork to background...\n");
#ifdef DAEMON_ALT
    daemon_alt();
#else
    daemon(0, 0);
#endif
#endif

    init_server_info(&sinfo);

    do {
        ready_fds = sinfo.read_fds;

        /* Wait new client, data from exist clients or
         * expire of time period. */
        if (sinfo.time_to_next_event < 0) {
            select_value = select(sinfo.max_fd + 1, &ready_fds,
                NULL, NULL, NULL);
        } else {
            tv.tv_sec = sinfo.time_to_next_event;
            tv.tv_usec = 0;
            time(&delta_time);
            select_value = select(sinfo.max_fd + 1, &ready_fds,
                NULL, NULL, &tv);
            delta_time = time(NULL) - delta_time;
        }

        if (SELECT_ERROR(select_value)) {
            perror("select()");
            exit(ES_SYSCALL_FAILED);
        }

        if (SELECT_TIMEOUT(select_value)) {
            process_time_events(&sinfo);
        } else if (sinfo.time_to_next_event > 0) {
            sinfo.time_to_next_event -= delta_time;
            if (sinfo.time_to_next_event < 0)
                sinfo.time_to_next_event = 0;
        }

        if (FD_ISSET(sinfo.listening_socket, &ready_fds)) {
            process_new_client(&sinfo, sinfo.listening_socket);
        }

        read_ready_data(&sinfo, &ready_fds);
        write_buffers_all_clients(&sinfo);
    } while (1);

    return 0;
}
