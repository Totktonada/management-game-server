#include "main.h"

void print_prompt (server_info *sinfo, client_info *client)
{
    update_time_buf (sinfo->prefix_prompt,
        sizeof (sinfo->prefix_prompt), PREFIX_PROMPT);
    ADD_S_STRLEN (&(client->write_buf), sinfo->prefix_prompt);
    ADD_S_STRLEN (&(client->write_buf), client->nick);
    ADD_S (&(client->write_buf), " $ ");
}

void notify_client_about_disconnect_reason (client_info *client)
{
    if (! client->connected)
        return;

    ADD_S (&(client->write_buf), "Disconnecting... Reason: ");

    switch (client->reason) {
    case REASON_SERVER_FULL:
        ADD_S (&(client->write_buf), "server full.\n");
        break;
    case REASON_BY_CLIENT:
        /* Not possible. */
        break;
    case REASON_PROTOCOL_PARSE_ERROR:
        ADD_S (&(client->write_buf), "protocol parse error.\n");
        break;
    case REASON_BANKRUPTING:
        ADD_S (&(client->write_buf), "you are bankrupt!\n");
        break;
    }
}

void notify_all_clients_about_disconnect (const server_info *sinfo,
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

        ADD_S (&(cur_c->write_buf), "Client ");
        ADD_S_STRLEN (&(cur_c->write_buf), client->nick);
        ADD_S (&(cur_c->write_buf), " disconnected.\nReason: ");

        switch (client->reason) {
        case REASON_SERVER_FULL:
            /* Not possible. */
            break;
        case REASON_BY_CLIENT:
            ADD_S (&(cur_c->write_buf),
                "connection closed by client.\n");
            break;
        case REASON_PROTOCOL_PARSE_ERROR:
            ADD_S (&(cur_c->write_buf),
                "protocol parse error.\n");
            break;
        case REASON_BANKRUPTING:
            ADD_S (&(cur_c->write_buf),
                "bankrupting.\n");
            break;
        }
    }
}

/* On success set sinfo->listening_socket
 * to proper value.
 * Exit on failure. */
void get_listening_socket (server_info *sinfo)
{
    struct sockaddr_in addr;
    int so_reuseaddr_value = 1;

    sinfo->listening_socket = socket (AF_INET, SOCK_STREAM, 0);
    if (SOCKET_ERROR (sinfo->listening_socket)) {
        perror ("socket ()");
        exit (ES_SYSCALL_FAILED);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons (sinfo->listening_port);
    /* addr.sin_addr.s_addr = INADDR_ANY; */
    if (INET_ATON_ERROR (
            inet_aton ("127.0.0.1", &(addr.sin_addr))))
    {
        perror ("inet_aton ()");
        exit (ES_SYSCALL_FAILED);
    }

    if (SETSOCKOPT (setsockopt (sinfo->listening_socket,
            SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr_value,
            sizeof (so_reuseaddr_value))))
    {
        perror ("setsockopt ()");
        exit (ES_SYSCALL_FAILED);
    }

    if (BIND_ERROR (bind (sinfo->listening_socket,
            (struct sockaddr *) &addr, sizeof (addr))))
    {
        perror ("bind ()");
        exit (ES_SYSCALL_FAILED);
    }

    if (LISTEN_ERROR (listen (sinfo->listening_socket, 5))) {
        perror ("listen ()");
        exit (ES_SYSCALL_FAILED);
    }
}

void update_max_fd (server_info *sinfo)
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

client_info *new_client_info (int client_socket)
{
    client_info *client = (client_info *)
        malloc (sizeof (client_info));
    client->next = NULL;
    client->fd = client_socket;
    client->connected = 0;
    client->to_disconnect = 0;
    /* client->read_buffer now exist, it is okay */
    client->read_available = 0;
    new_parser_info (&(client->pinfo));
    new_client_game_data (client);
    new_msg_buffer (&(client->write_buf));
    return client;
}

/* Get listening socket and put it to read_fds.
 * Note: no malloc */
void init_server_info (server_info *sinfo)
{
    get_listening_socket (sinfo);
    FD_ZERO (&(sinfo->read_fds));
    FD_SET (sinfo->listening_socket, &(sinfo->read_fds));
    sinfo->first_client = NULL;
    update_max_fd (sinfo);
    new_game_data (sinfo);
}

void mark_client_to_disconnect (client_info *client,
    disconnect_reasons reason)
{
    client->to_disconnect = 1;
    client->reason = reason;
}

/* Remove client from our structures (if it contain this client). */
void try_to_unregister_client (server_info *sinfo, client_info *client)
{
    client_info *prev_c = NULL;
    client_info *cur_c = sinfo->first_client;
    client_info *next_c = NULL;

    while (cur_c != NULL) {
        next_c = cur_c->next;

        if (cur_c == client) {
            FD_CLR (client->fd, &(sinfo->read_fds));
            if (sinfo->max_fd == client->fd) {
                update_max_fd (sinfo);
            }

            if (cur_c == sinfo->first_client)
                sinfo->first_client = next_c;
            else
                prev_c->next = next_c;

            if (cur_c == sinfo->last_client)
                sinfo->last_client = prev_c;

            --sinfo->clients_count;
            break;
        }

        prev_c = cur_c;
        cur_c = next_c;
    }
}

/* TODO: maybe add prompt to end? */
void add_async_prefixes (server_info *sinfo, client_info *client)
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
        if (is_msg_buffer_empty (&(cur_c->write_buf)))
            continue;

        /* Add prefix for asynchronous messages. */
        if (cur_c != client) {
            update_time_buf (sinfo->prefix_async_msg,
                sizeof (sinfo->prefix_async_msg), PREFIX_ASYNC_MSG);
            ADD_PREFIX_STRLEN (&(cur_c->write_buf),
                sinfo->prefix_async_msg);
        }
    }
}

/* TODO: use select */
void write_buffers_all_clients (server_info *sinfo)
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
        if (is_msg_buffer_empty (&(cur_c->write_buf)))
            continue;

        write_msg_buffer (&(cur_c->write_buf), cur_c->fd);
    }
}

/* Disconnect client. */
void try_to_disconnect (server_info *sinfo, client_info *client)
{
    if (! client->to_disconnect)
        return;

    notify_client_about_disconnect_reason (client);
    notify_all_clients_about_disconnect (sinfo, client);
    add_async_prefixes (sinfo, client);
    write_msg_buffer (&(client->write_buf), client->fd);

    try_to_unregister_client (sinfo, client);

    if (SHUTDOWN_ERROR (
        shutdown (client->fd, SHUT_RDWR)))
    {
        perror ("shutdown ()");
        exit (ES_SYSCALL_FAILED);
    }

    client->connected = 0;

    if (CLOSE_ERROR (close (client->fd))) {
        perror ("close ()");
        exit (ES_SYSCALL_FAILED);
    }

    free (client);
}

/* Add new client to our structures.
 * Exit on failure. */
void process_new_client (server_info *sinfo, int listening_socket)
{
    /* We not save information about client IP and port. */
    int client_socket = accept (listening_socket, NULL, NULL);
    client_info *new_client;

    if (ACCEPT_ERROR (client_socket)) {
        perror ("accept ()");
        exit (ES_SYSCALL_FAILED);
    }

    new_client = new_client_info (client_socket);
    new_client->connected = 1;

    if (game_process_new_client (sinfo, new_client)) {
        mark_client_to_disconnect (new_client, REASON_SERVER_FULL);
        try_to_disconnect (sinfo, new_client);
        return;
    }

    add_async_prefixes (sinfo, new_client);

    if (sinfo->first_client == NULL) {
        sinfo->last_client = sinfo->first_client = new_client;
    } else {
        sinfo->last_client = sinfo->last_client->next = new_client;
    }

    FD_SET (client_socket, &(sinfo->read_fds));
    if (sinfo->max_fd < client_socket) {
        sinfo->max_fd = client_socket;
    }

    print_prompt (sinfo, new_client);
}

void process_readed_data (server_info *sinfo, client_info *client)
{
    command *cmd;
    int destroy_str;

    put_new_data_to_parser (&(client->pinfo),
        client->read_buffer,
        client->read_available);

    do {
        cmd = get_cmd (&(client->pinfo));
        if (cmd == NULL)
            break;
        if (cmd->type == CMD_PROTOCOL_PARSE_ERROR) {
            mark_client_to_disconnect (client,
                REASON_PROTOCOL_PARSE_ERROR);
            break;
        }
#ifndef DAEMON
        print_cmd (cmd);
#endif
        destroy_str = execute_cmd (sinfo, client, cmd);
        destroy_cmd (cmd, destroy_str);
        add_async_prefixes (sinfo, client);
        print_prompt (sinfo, client);
    } while (1);
}

void read_ready_data (server_info *sinfo, fd_set *ready_fds)
{
    client_info *cur_c = sinfo->first_client;
    client_info *next_c;
    int read_value;

    while (cur_c != NULL) {
        next_c = cur_c->next;

        if (! FD_ISSET (cur_c->fd, ready_fds)) {
            cur_c = next_c;
            continue;
        }

        read_value = read (cur_c->fd, cur_c->read_buffer,
            sizeof (char) * READ_BUFFER_SIZE);
        if (READ_ERROR (read_value)) {
            perror ("read ()");
            exit (ES_SYSCALL_FAILED);
        } else if (READ_EOF (read_value)) {
            cur_c->connected = 0;
            mark_client_to_disconnect (cur_c, REASON_BY_CLIENT);
        } else {
            cur_c->read_available = read_value;
            process_readed_data (sinfo, cur_c);
        }

        try_to_disconnect (sinfo, cur_c);
        cur_c = next_c;
    } /* while */
}

#ifdef DAEMON_ALT
void daemon_alt ()
{
    int fork_value;

    if (CHDIR_ERROR (chdir ("/"))) {
        perror ("chdir ()");
        exit (ES_SYSCALL_FAILED);
    }

    if (CLOSE_ERROR (close (STDIN_FILENO))
        || CLOSE_ERROR (close (STDOUT_FILENO))
        || CLOSE_ERROR (close (STDERR_FILENO)))
    {
        perror ("close ()");
        exit (ES_SYSCALL_FAILED);
    }

    if (CLOSE_ERROR (open ("/dev/null", O_WRONLY))
        || CLOSE_ERROR (open ("/dev/null", O_RDONLY))
        || CLOSE_ERROR (open ("/dev/null", O_RDONLY)))
    {
        perror ("open ()");
        exit (ES_SYSCALL_FAILED);
    }

    fork_value = fork ();
    if (FORK_ERROR (fork_value)) {
        perror ("fork ()");
        exit (ES_SYSCALL_FAILED);
    } else if (FORK_PARENT (fork_value)) {
        exit (0);
    }

    if (SETSID_ERROR (setsid ())) {
        perror ("setsid ()");
        exit (ES_SYSCALL_FAILED);
    }
}
#endif

int main (int argc, char **argv, char **envp)
{
    server_info sinfo;
    int select_value;
    fd_set ready_fds;

    sinfo.listening_port = DEFAULT_LISTENING_PORT;
    process_cmd_line_parameters (&sinfo, argv + 1);

#ifdef DAEMON
    printf ("Fork to background...\n");
#ifdef DAEMON_ALT
    daemon_alt ();
#else
    daemon (0, 0);
#endif
#endif

    init_server_info (&sinfo);

    do {
        ready_fds = sinfo.read_fds;

        /* Wait new client or data from exist clients */
        select_value = select (sinfo.max_fd + 1, &ready_fds,
            NULL, NULL, 0);
        if (SELECT_ERROR (select_value)) {
            perror ("select ()");
            exit (ES_SYSCALL_FAILED);
        }

        if (SELECT_TIMEOUT (select_value)) {
            /* TODO */;
        }

        if (FD_ISSET (sinfo.listening_socket, &ready_fds)) {
            process_new_client (&sinfo, sinfo.listening_socket);
        }

        read_ready_data (&sinfo, &ready_fds);
        write_buffers_all_clients (&sinfo);
    } while (1);

    return 0;
}
