#include "main.h"

/* On success returns descriptor
 * of listening socket.
 * Exit on failure. */
int get_listening_socket ()
{
    int listening_socket;
    struct sockaddr_in addr;
    int so_reuseaddr_value = 1;

    listening_socket = socket (AF_INET, SOCK_STREAM, 0);
    if (SOCKET_ERROR (listening_socket)) {
        perror ("socket ()");
        exit (ES_SYSCALL_FAILED);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons (LISTENING_PORT);
    /* addr.sin_addr.s_addr = INADDR_ANY; */
    if (INET_ATON_ERROR (
            inet_aton ("127.0.0.1", &(addr.sin_addr))))
    {
        perror ("inet_aton ()");
        exit (ES_SYSCALL_FAILED);
    }

    if (SETSOCKOPT (setsockopt (listening_socket,
            SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr_value,
            sizeof (so_reuseaddr_value))))
    {
        perror ("setsockopt ()");
        exit (ES_SYSCALL_FAILED);
    }

    if (BIND_ERROR (bind (listening_socket,
            (struct sockaddr *) &addr, sizeof (addr))))
    {
        perror ("bind ()");
        exit (ES_SYSCALL_FAILED);
    }

    if (LISTEN_ERROR (listen (listening_socket, 5))) {
        perror ("listen ()");
        exit (ES_SYSCALL_FAILED);
    }

    return listening_socket;
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
    return client;
}

/* Get listening socket and put it to read_fds.
 * Note: no malloc */
void new_server_info (server_info *sinfo)
{
    sinfo->listening_socket = get_listening_socket ();
    FD_ZERO (&(sinfo->read_fds));
    FD_SET (sinfo->listening_socket, &(sinfo->read_fds));
    sinfo->first_client = NULL;
    update_max_fd (sinfo);
    new_game_data (&(sinfo->gdata));
}

/* Add new client to our structures.
 * Exit on failure. */
void process_new_client (server_info *sinfo, int listening_socket)
{
    /* We not save information about client IP and port. */
    int client_socket = accept (listening_socket, NULL, NULL);

    if (ACCEPT_ERROR (client_socket)) {
        perror ("accept ()");
        exit (ES_SYSCALL_FAILED);
    }

    if (sinfo->first_client == NULL) {
        sinfo->last_client = sinfo->first_client =
            new_client_info (client_socket);
    } else {
        sinfo->last_client = sinfo->last_client->next =
            new_client_info (client_socket);
    }

    ++sinfo->gdata.clients_count;

    FD_SET (client_socket, &(sinfo->read_fds));
    if (sinfo->max_fd < client_socket) {
        sinfo->max_fd = client_socket;
    }
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

            --sinfo->gdata.clients_count;
            break;
        }

        prev_c = cur_c;
        cur_c = next_c;
    }
}

/* Disconnect client */
void client_disconnect (server_info *sinfo, client_info *client)
{
    /* TODO: write "Disconnecting..." */

    FD_CLR (client->fd, &(sinfo->read_fds));
    if (sinfo->max_fd == client->fd) {
        update_max_fd (sinfo);
    }

    if (SHUTDOWN_ERROR (
        shutdown (client->fd, SHUT_RDWR)))
    {
        perror ("shutdown ()");
        exit (ES_SYSCALL_FAILED);
    }
    if (CLOSE_ERROR (close (client->fd))) {
        perror ("shutdown ()");
        exit (ES_SYSCALL_FAILED);
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
            /* TODO: tell user about disconnect reason. */
            client_disconnect (sinfo, client);
            unregister_client (sinfo, client);
            free (client);
            break;
        }
#ifndef DAEMON
        print_cmd (cmd);
#endif
        execute_cmd (&(sinfo->gdata), client->fd, cmd);
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
            client_disconnect (sinfo, cur_c);
            unregister_client (sinfo, cur_c);
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

#ifdef DAEMON
    printf ("Fork to background...\n");
#ifdef DAEMON_ALT
    daemon_alt ();
#else
    daemon (0, 0);
#endif
#endif

    new_server_info (&sinfo);

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
