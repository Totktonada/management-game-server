#include "main.h"

char *first_vacant_nick (client_info *first_client)
{
    /* See comment to number_to_str ().
     * First position reserved for 'p' symbol. */
    char *buf = (char *) malloc (12 * sizeof (char));
    client_info *cur_c;
    int nick_number = 0;
    int nick_found;

    *buf = 'p';

    do {
        nick_found = 0;
        cur_c = first_client;
        number_to_str (buf + 1, nick_number);

        while (cur_c != NULL && !nick_found) {
            if (cur_c->nick == NULL) {
                nick_found = 0;
            } else {
                nick_found = STR_EQUAL (buf,
                    cur_c->nick);
            }
            cur_c = cur_c->next;
        }

        ++nick_number;
    } while (nick_found);

    return buf;
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
    client->fd = client_socket;
    /* client->read_buffer now exist, it is okay */
    client->read_available = 0;
    new_parser_info (&(client->pinfo));
    client->next = NULL;
    new_client_game_data (client);
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

/* Remove client from our structures. */
void unregister_client (server_info *sinfo, client_info *client)
{
    client_info *prev_c = NULL;
    client_info *cur_c = sinfo->first_client;
    client_info *next_c = NULL;

    while (cur_c != NULL) {
        next_c = cur_c->next;

        if (cur_c == client) {
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

/* Disconnect client. */
void client_disconnect (server_info *sinfo, client_info *client,
    int client_in_server_info_list)
{
    /* TODO: write "Disconnecting..." */

    if (client_in_server_info_list) {
        FD_CLR (client->fd, &(sinfo->read_fds));
        if (sinfo->max_fd == client->fd) {
            update_max_fd (sinfo);
        }
    }

    if (SHUTDOWN_ERROR (
        shutdown (client->fd, SHUT_RDWR)))
    {
        perror ("shutdown ()");
        exit (ES_SYSCALL_FAILED);
    }
    if (CLOSE_ERROR (close (client->fd))) {
        perror ("close ()");
        exit (ES_SYSCALL_FAILED);
    }
}

/* Add new client to our structures.
 * Exit on failure. */
void process_new_client (server_info *sinfo, int listening_socket)
{
    /* We not save information about client IP and port. */
    int client_socket = accept (listening_socket, NULL, NULL);
    client_info *new_client;
    char msg_disconnect[] = "Server full.\n";

    if (ACCEPT_ERROR (client_socket)) {
        perror ("accept ()");
        exit (ES_SYSCALL_FAILED);
    }

    new_client = new_client_info (client_socket);

    if (game_process_new_client (sinfo)) {
        write (client_socket, msg_disconnect,
            sizeof (msg_disconnect) - 1);
        client_disconnect (sinfo, new_client, 0);
        free (new_client);
        return;
    }

    new_client->nick = first_vacant_nick (sinfo->first_client);

    if (sinfo->first_client == NULL) {
        sinfo->last_client = sinfo->first_client = new_client;
    } else {
        sinfo->last_client = sinfo->last_client->next = new_client;
    }

    FD_SET (client_socket, &(sinfo->read_fds));
    if (sinfo->max_fd < client_socket) {
        sinfo->max_fd = client_socket;
    }
}

void process_readed_data (server_info *sinfo, client_info *client)
{
    command *cmd;

    put_new_data_to_parser (&(client->pinfo),
        client->read_buffer,
        client->read_available);

    do {
        cmd = get_cmd (&(client->pinfo));
        if (cmd == NULL)
            break;
        if (cmd->type == CMD_PROTOCOL_PARSE_ERROR) {
            /* TODO: tell client about disconnect reason. */
            unregister_client (sinfo, client);
            client_disconnect (sinfo, client, 1);
            free (client);
            break;
        }
#ifndef DAEMON
        print_cmd (cmd);
#endif
        execute_cmd (sinfo, client, cmd);
        destroy_cmd (cmd);
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
            /* TODO: messages about this event for all clients. */
            unregister_client (sinfo, cur_c);
            client_disconnect (sinfo, cur_c, 1);
            free (cur_c);
        } else {
            cur_c->read_available = read_value;
            process_readed_data (sinfo, cur_c);
        }

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
    } while (1);

    return 0;
}
