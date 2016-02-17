/*
 * Copyright 2003-2016 Jeffrey K. Hollingsworth
 *
 * This file is part of Active Harmony.
 *
 * Active Harmony is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Active Harmony is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Active Harmony.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "hserver.h"
#include "httpsvr.h"
#include "hcfg.h"
#include "hmesg.h"
#include "hperf.h"
#include "hsockutil.h"
#include "hutil.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <libgen.h>
#include <signal.h>
#include <math.h>
#include <getopt.h> // For getopt_long(). Requires _GNU_SOURCE.

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

/*
 * Internal helper function prototypes.
 */
static int  launch_session(void);
static int  verbose(const char* fmt, ...);
static int  parse_opts(int argc, char* argv[]);
static int  vars_init(int argc, char* argv[]);
static int  network_init(void);
static int  handle_new_connection(int fd);
static int  handle_unknown_connection(int fd);
static int  handle_client_socket(int fd);
static int  handle_session_socket(void);
static int  available_index(int** list, int* cap);
static int  count_client(int fd);
static void client_close(int fd);
static void update_flags(sinfo_t* sinfo, const char* keyval);
static int  update_state(sinfo_t* sinfo);
static int  append_http_log(sinfo_t* sinfo, const hpoint_t* pt, double perf);

static sinfo_t* sinfo_by_id(int id);
static sinfo_t* sinfo_by_name(const char* name);
static sinfo_t* register_search(int src_fd);
static sinfo_t* open_search(void);

/*
 * Main variables
 */
static int    listen_port = DEFAULT_PORT;
static int    listen_socket;
static int    session_fd;
static fd_set listen_fds;
static int    highest_socket;

static int* unk_fds, unk_cap;
static int* client_fds, client_cap;
static int* http_fds, http_len, http_cap;

static char* harmony_dir;
static char* session_bin;
static hmesg_t mesg;

sinfo_t* slist;
int slist_cap;

/*
 * Local variables
 */
static int verbose_flag;

void usage(const char* prog)
{
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "OPTIONS:\n"
"  -p, --port=PORT   Port to listen to on the local host. (Default: %d)\n"
"  -v, --verbose     Print additional information during operation.\n\n",
            listen_port);
}

int main(int argc, char* argv[])
{
    int i, fd_count, retval;
    fd_set ready_fds;

    // Parse user options.
    if (parse_opts(argc, argv) != 0)
        return -1;

    // Initialize global variable state.
    if (vars_init(argc, argv) < 0)
        return -1;

    // Initialize the HTTP user interface.
    if (http_init(harmony_dir) < 0)
        return -1;

    // Initialize socket networking service.
    if (network_init() < 0)
        return -1;

    // Launch the underlying search session.
    if (launch_session() != 0)
        return -1;

    while (1) {
        ready_fds = listen_fds;
        fd_count = select(highest_socket + 1, &ready_fds, NULL, NULL, NULL);
        if (fd_count == -1) {
            if (errno == EINTR)
                continue;

            perror("Error selecting active sockets");
            break;
        }

        if (fd_count > 0) {
            // Before all else, handle input from session process.
            if (FD_ISSET(session_fd, &ready_fds)) {
                if (handle_session_socket() > 0)
                    // Session socket was closed.  Begin shutdown.
                    goto shutdown;
            }

            // Handle new connections.
            if (FD_ISSET(listen_socket, &ready_fds)) {
                retval = handle_new_connection(listen_socket);
                if (retval > 0) {
                    FD_SET(retval, &listen_fds);
                    if (highest_socket < retval)
                        highest_socket = retval;
                }
            }

            // Handle unknown connections (Unneeded if we switch to UDP).
            for (i = 0; i < unk_cap; ++i) {
                if (unk_fds[i] == -1)
                    continue;

                if (FD_ISSET(unk_fds[i], &ready_fds)) {
                    if (handle_unknown_connection(unk_fds[i]) < 0)
                        FD_CLR(unk_fds[i], &listen_fds);
                    unk_fds[i] = -1;
                }
            }

            // Handle Harmony messages.
            for (i = 0; i < client_cap; ++i) {
                if (client_fds[i] == -1)
                    continue;

                if (FD_ISSET(client_fds[i], &ready_fds)) {
                    retval = handle_client_socket(client_fds[i]);
                    if (retval > 0) goto shutdown;
                    if (retval < 0) {
                        client_close(client_fds[i]);
                        client_fds[i] = -1;
                    }
                }
            }

            // Handle http requests.
            for (i = 0; i < http_cap; ++i) {
                if (http_fds[i] == -1)
                    continue;

                if (FD_ISSET(http_fds[i], &ready_fds)) {
                    if (handle_http_socket(http_fds[i]) < 0) {
                        FD_CLR(http_fds[i], &listen_fds);
                        http_fds[i] = -1;
                        --http_len;
                    }
                }
            }
        }
    }
shutdown:
    return 0;
}

int verbose(const char* fmt, ...)
{
    int retval;
    va_list ap;

    if (!verbose_flag)
        return 0;

    va_start(ap, fmt);
    retval = vfprintf(stderr, fmt, ap);
    va_end(ap);

    return retval;
}

int parse_opts(int argc, char* argv[])
{
    int c;
    static struct option long_options[] = {
        {"port",    required_argument, NULL, 'p'},
        {"verbose", no_argument,       NULL, 'v'},
        {NULL, 0, NULL, 0}
    };

    while (1) {
        c = getopt_long(argc, argv, "p:v", long_options, NULL);
        if (c == -1)
            break;

        switch(c) {
        case 'p': listen_port = atoi(optarg); break;
        case 'v': verbose_flag = 1; break;

        case ':':
            usage(argv[0]);
            fprintf(stderr, "\nOption ('%c') requires an argument.\n", optopt);
            break;

        case '?':
        default:
            usage(argv[0]);
            fprintf(stderr, "\nInvalid argument ('%c').\n", optopt);
            return -1;
        }
    }

    return 0;
}

int vars_init(int argc, char* argv[])
{
    char* tmppath;
    char* binfile;

    // Ignore signal for writes to broken pipes/sockets.
    signal(SIGPIPE, SIG_IGN);

    //Determine directory where this binary is located
    tmppath = stralloc(argv[0]);
    binfile = stralloc(basename(tmppath));
    free(tmppath);

    if ( (tmppath = getenv(CFGKEY_HARMONY_HOME))) {
        harmony_dir = stralloc(tmppath);
        verbose(CFGKEY_HARMONY_HOME " is %s\n", harmony_dir);
    }
    else {
        if (strchr(argv[0], '/'))
            tmppath = stralloc(argv[0]);
        else
            tmppath = stralloc(search_path(binfile));

        harmony_dir = dirname(tmppath);
        if (strcmp(harmony_dir, ".") == 0)
            harmony_dir = "..";
        else
            harmony_dir = stralloc(dirname(harmony_dir));
        free(tmppath);

        verbose("Detected %s/ as HARMONY_HOME\n", harmony_dir);
    }
    free(binfile);

    // Find supporting binaries and shared objects.
    session_bin = sprintf_alloc("%s/libexec/" SESSION_CORE_EXECFILE,
                                harmony_dir);
    if (!file_exists(session_bin)) {
        fprintf(stderr, "Could not find support files in "
                CFGKEY_HARMONY_HOME "\n");
        return -1;
    }

    // Prepare the file descriptor lists.
    unk_fds = NULL;
    unk_cap = 0;
    if (array_grow(&unk_fds, &unk_cap, sizeof(int)) < 0) {
        perror("Could not allocate memory for initial file descriptor list");
        return -1;
    }
    memset(unk_fds, -1, unk_cap * sizeof(int));

    client_fds = NULL;
    client_cap = 0;
    if (array_grow(&client_fds, &client_cap, sizeof(int)) < 0) {
        perror("Could not allocate memory for harmony file descriptor list");
        return -1;
    }
    memset(client_fds, -1, client_cap * sizeof(int));

    http_fds = NULL;
    http_len = http_cap = 0;
    if (array_grow(&http_fds, &http_cap, sizeof(int)) < 0) {
        perror("Could not allocate memory for http file descriptor list");
        return -1;
    }
    memset(http_fds, -1, http_cap * sizeof(int));

    slist = NULL;
    slist_cap = 0;
    if (array_grow(&slist, &slist_cap, sizeof(sinfo_t)) < 0) {
        perror("Could not allocate memory for session file descriptor list");
        return -1;
    }
    for (int i = 0; i < slist_cap; ++i)
        slist[i].id = -1; // Initialize slist with non-zero defaults.

    return 0;
}

int network_init(void)
{
    int optval;
    struct sockaddr_in addr;

    // Create a listening socket.
    verbose("Listening on TCP port: %d\n", listen_port);
    listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket < 0) {
        perror("Could not create listening socket");
        return -1;
    }

    // Set socket options.
    //
    // Try to make the port reusable and have it close as fast as
    // possible without waiting in unnecessary wait states on close.
    //
    optval = 1;
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR,
                   &optval, sizeof(optval)) < 0)
    {
        perror("Could not set options on listening socket");
        return -1;
    }

    // Initialize the socket address.
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((unsigned short)listen_port);

    // Bind the socket to the desired port.
    if (bind(listen_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Could not bind socket to listening address");
        return -1;
    }

    // Set the file descriptor set.
    FD_ZERO(&listen_fds);
    FD_SET(listen_socket, &listen_fds);
    highest_socket = listen_socket;

    if (listen(listen_socket, SOMAXCONN) < 0) {
        perror("Could not listen on listening socket");
        return -1;
    }

    return 0;
}

int handle_new_connection(int fd)
{
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    int idx, newfd;

    newfd = accept(fd, (struct sockaddr*)&addr, &addrlen);
    if (newfd < 0) {
        perror("Error accepting new connection");
        return -1;
    }

    // The peer may not have sent data yet.  To prevent blocking, lets
    // stash the new socket in the unknown_list until we know it has data.
    //
    idx = available_index(&unk_fds, &unk_cap);
    if (idx < 0) {
        perror("Could not grow unknown connection array");
        return -1;
    }

    unk_fds[idx] = newfd;
    verbose("Accepted connection from %s as socket %d\n",
            inet_ntoa(addr.sin_addr), newfd);
    return newfd;
}

int handle_unknown_connection(int fd)
{
    unsigned int header;
    int idx, readlen;

    readlen = recv(fd, &header, sizeof(header), MSG_PEEK | MSG_DONTWAIT);
    if (readlen < 0 || (unsigned int)readlen < sizeof(header)) {
        // Error on recv, or insufficient data.  Close the connection.
        printf("Can't determine type for socket %d.  Closing.\n", fd);
        if (close(fd) < 0)
            perror("Error closing connection");
        return -1;
    }
    else if (ntohl(header) == HMESG_MAGIC) {
        // This is a communication socket from a Harmony client.
        idx = available_index(&client_fds, &client_cap);
        if (idx < 0) {
            perror("Could not grow Harmony connection array");
            if (close(fd) < 0)
                perror("Error closing Harmony connection");
            return -1;
        }
        client_fds[idx] = fd;
    }
    else
    {
        // Consider this an HTTP communication socket.
        if (http_len >= http_connection_limit) {
            printf("Hit HTTP connection limit on socket %d.  Closing.\n", fd);
            http_send_error(fd, 503, NULL);
            if (close(fd) < 0)
                perror("Error closing HTTP connection");
            return -1;
        }

        idx = available_index(&http_fds, &http_cap);
        if (idx < 0) {
            perror("Could not grow HTTP connection array");
            if (close(fd) < 0)
                perror("Error closing HTTP connection");
            return -1;
        }

        http_fds[idx] = fd;
        ++http_len;
    }
    return 0;
}

int handle_client_socket(int fd)
{
    int idx;
    double perf;
    sinfo_t* sinfo = NULL;

    int retval = mesg_recv(fd, &mesg);
    if (retval <  0) goto error;
    if (retval == 0) {
        client_close(fd);
        return 0;
    }

    // Sanity check input.
    if (mesg.type != HMESG_SESSION && mesg.type != HMESG_JOIN) {
        sinfo = sinfo_by_id(mesg.dest);
        if (!sinfo) {
            mesg.data.string = "Invalid message destination";
            goto error;
        }
    }

    switch (mesg.type) {
    case HMESG_GETCFG:
    case HMESG_BEST:
    case HMESG_FETCH:
    case HMESG_RESTART:
        break;

    case HMESG_SETCFG:
        update_flags(sinfo, mesg.data.string);
        break;

    case HMESG_SESSION:
        sinfo = register_search(fd);
        if (!sinfo)
            goto error;
        break;

    case HMESG_JOIN:
        sinfo = sinfo_by_name(mesg.data.string);
        if (!sinfo) {
            mesg.data.string = "Could not find existing search info";
            goto error;
        }

        idx = available_index(&sinfo->client, &sinfo->client_cap);
        if (idx < 0) {
            mesg.data.string = "Could not grow search client list";
            goto error;
        }

        sinfo->client[idx] = fd;
        ++sinfo->client_len;
        break;

    case HMESG_REPORT:
        perf = hperf_unify(mesg.data.perf);

        for (idx = 0; idx < sinfo->fetched_len; ++idx) {
            if (sinfo->fetched[idx].id == mesg.data.point->id)
                break;
        }
        if (idx < sinfo->fetched_len) {
            const hpoint_t* pt = &sinfo->fetched[idx];

            // Copy point from fetched list to HTTP log.
            if (append_http_log(sinfo, pt, perf) != 0) {
                mesg.data.string = "Could not append to HTTP log";
                goto error;
            }

            // Remove point from fetched list.
            --sinfo->fetched_len;
            if (idx < sinfo->fetched_len) {
                if (hpoint_copy(&sinfo->fetched[idx],
                                &sinfo->fetched[sinfo->fetched_len]) != 0)
                {
                    mesg.data.string = "Could not remove fetch list point";
                    goto error;
                }
            }
        }
        else {
            // Copy point from fetched list to HTTP log.
            if (append_http_log(sinfo, &sinfo->best, perf) != 0) {
                mesg.data.string = "Could not copy best point to HTTP log";
                goto error;
            }
        }
        break;

    default:
        goto error;
    }

    // Overwrite the destination with the client fd.
    mesg.src = fd;

    retval = mesg_forward(session_fd, &mesg);
    if (retval == 0) goto shutdown;
    if (retval <  0) {
        mesg.data.string = "Could not forward message to session";
        goto error;
    }
    return 0;

  error:
    mesg.dest   = mesg.src;
    mesg.src    = -1;
    mesg.status = HMESG_STATUS_FAIL;
    mesg_send(fd, &mesg);
    return -1;

  shutdown:
    fprintf(stderr, "Session socket closed. Shutting server down.\n");
    return 1;
}

int handle_session_socket(void)
{
    int retval = mesg_recv(session_fd, &mesg);
    if (retval == 0) goto shutdown;
    if (retval <  0) {
        perror("Received malformed message from session");
        return -1;
    }

    sinfo_t* sinfo = NULL;
    if (mesg.type != HMESG_SESSION) {
        sinfo = sinfo_by_id(mesg.src);
        if (!sinfo) {
            mesg.data.string = "Server error: No sinfo for session message";
            goto error;
        }
    }

    switch (mesg.type) {
    case HMESG_SESSION:
        sinfo = open_search();
        break;

    case HMESG_JOIN:
    case HMESG_SETCFG:
    case HMESG_BEST:
    case HMESG_RESTART:
        break;

    case HMESG_GETCFG:
        update_flags(sinfo, mesg.data.string);
        break;

    case HMESG_FETCH:
        if (mesg.status == HMESG_STATUS_OK) {
            // Log this point before we forward it to the client.
            if (sinfo->fetched_len == sinfo->fetched_cap) {
                if (array_grow(&sinfo->fetched, &sinfo->fetched_cap,
                               sizeof(*sinfo->fetched)) != 0)
                {
                    mesg.data.string = "Server error: Couldn't grow fetch log";
                    goto error;
                }
            }

            if (hpoint_copy(&sinfo->fetched[sinfo->fetched_len],
                            mesg.data.point) != 0)
            {
                mesg.data.string = "Server error: Couldn't add to HTTP log";
                goto error;
            }

            if (hpoint_align(&sinfo->fetched[sinfo->fetched_len],
                             &sinfo->space) != 0)
            {
                mesg.data.string = "Server error: Couldn't align point";
                goto error;
            }
            ++sinfo->fetched_len;
        }
        break;

    case HMESG_REPORT:
        if (mesg.status == HMESG_STATUS_OK)
            ++sinfo->reported;
        break;

    default:
        mesg.data.string = "Server error: Invalid message type from session";
        goto error;
    }

    if (sinfo && update_state(sinfo) != 0) {
        mesg.data.string = "Server error: Could not update search state";
        goto error;
    }

    // Messages with a negative destination field came within hserver itself.
    // No need to forward those messages.
    //
    if (mesg.dest >= 0) {
        if (mesg_forward(mesg.dest, &mesg) < 1) {
            perror("Error forwarding message to client");
            client_close(mesg.dest);
        }
    }
    return 0;

  error:
    mesg.status = HMESG_STATUS_FAIL;
    mesg_send(mesg.dest, &mesg);
    return -1;

  shutdown:
    fprintf(stderr, "Session socket closed. Shutting server down.\n");
    return 1;
}

int launch_session(void)
{
    // Fork and exec a session handler.
    char* const child_argv[] = {session_bin,
                                harmony_dir,
                                NULL};
    session_fd = socket_launch(session_bin, child_argv, NULL);
    if (session_fd < 0) {
        perror("Could not launch session process");
        return -1;
    }

    FD_SET(session_fd, &listen_fds);
    if (highest_socket < session_fd)
        highest_socket = session_fd;

    return 0;
}

sinfo_t* register_search(int src_fd)
{
    int idx = -1;
    for (int i = 0; i < slist_cap; ++i) {
        if (!slist[i].name) {
            if (idx < 0)
                idx = i; // Mark first available index as we pass it.
        }
        else if (strcmp(slist[i].name, mesg.state.space->name) == 0) {
            mesg.data.string = "Search name already registered";
            return NULL;
        }
    }
    if (idx < 0) {
        idx = slist_cap;
        if (array_grow(&slist, &slist_cap, sizeof(*slist)) != 0) {
            mesg.data.string = "Could not extend search list";
            return NULL;
        }
        for (int i = idx; i < slist_cap; ++i)
            slist[i].id = -1; // Initialize slist with non-zero defaults.
    }

    sinfo_t* sinfo = &slist[idx];
    if (hspace_copy(&sinfo->space, mesg.state.space) != 0) {
        mesg.data.string = "Could not copy space into search info";
        return NULL;
    }

    const char* cfgstr = hcfg_get(mesg.data.cfg, CFGKEY_STRATEGY);
    if (!cfgstr) {
        int clients = hcfg_int(mesg.data.cfg, CFGKEY_CLIENT_COUNT);
        if (clients < 1)
            cfgstr = "???";
        else if (clients == 1)
            cfgstr = "nm.so";
        else
            cfgstr = "pro.so";
    }
    sinfo->strategy = stralloc(cfgstr);

    sinfo->client_len = 0;
    sinfo->best.id = 0;
    sinfo->best_perf = HUGE_VAL;
    sinfo->status = 0x0;
    sinfo->log_len = 0;
    sinfo->fetched_len = 0;
    sinfo->reported = 0;
    sinfo->id = -src_fd;

    return sinfo;
}

sinfo_t* open_search(void)
{
    sinfo_t* sinfo = NULL;

    // Find the registered search.
    for (int i = 0; i < slist_cap; ++i) {
        if (slist[i].id == -mesg.dest) {
            sinfo = &slist[i];
            break;
        }
    }

    if (!sinfo) {
        fprintf(stderr, "Could not find matching HMESG_SESSION request.\n");
        return NULL;
    }

    if (mesg.status != HMESG_STATUS_OK) {
        // Reset the sinfo slot.
        goto error;
    }
    else {
        // Enable the sinfo slot.
        sinfo->name = sinfo->space.name;
        sinfo->id = mesg.src;

        // Initialize HTTP server fields.
        if (gettimeofday(&sinfo->start, NULL) != 0) {
            perror("Could not save search start time");
            goto error;
        }

        // Add the initiating client to the search info.
        int idx = available_index(&sinfo->client, &sinfo->client_cap);
        if (idx < 0) {
            perror("Could not grow search client list");
            goto error;
        }
        sinfo->client[idx] = mesg.dest;
        ++sinfo->client_len;
    }
    return sinfo;

  error:
    sinfo->name = NULL;
    sinfo->id   = -1;
    return NULL;
}

void search_close(sinfo_t* sinfo)
{
    sinfo->name = NULL;
    sinfo->id   = -1;

    // For each client, see if it is participating in any other
    // searches.  If not, close its socket.
    //
    for (int i = 0; i < sinfo->client_cap; ++i) {
        if (sinfo->client[i] == -1)
            continue;

        if (count_client(sinfo->client[i]) == 1)
            client_close(sinfo->client[i]);
    }

    free(sinfo->strategy);
    sinfo->best.id = 0;
}

int count_client(int fd)
{
    int retval = 0;

    for (int i = 0; i < slist_cap; ++i) {
        if (!slist[i].name)
            continue;

        for (int j = 0; j < slist[i].client_cap; ++j) {
            if (slist[i].client[j] == fd)
                ++retval;
        }
    }
    return retval;
}

void client_close(int fd)
{
    for (int i = 0; i < slist_cap; ++i) {
        if (!slist[i].name)
            continue;

        for (int j = 0; j < slist[i].client_cap; ++j) {
            if (slist[i].client[j] == fd) {
                slist[i].client[j] = -1;
                --slist[i].client_len;
                break;
            }
        }
    }

    for (int i = 0; i < client_cap; ++i) {
        if (client_fds[i] == fd) {
            client_fds[i] = -1;
            break;
        }
    }

    if (shutdown(fd, SHUT_RDWR) != 0 || close(fd) != 0)
        perror("Error closing client connection post error");

    FD_CLR(fd, &listen_fds);
}

int available_index(int** list, int* cap)
{
    int i, orig_cap;

    for (i = 0; i < *cap; ++i)
        if ((*list)[i] == -1)
            return i;

    orig_cap = *cap;
    if (array_grow(list, cap, sizeof(int)) < 0)
        return -1;
    memset((*list) + orig_cap, -1, (*cap - orig_cap) * sizeof(int));

    return orig_cap;
}

void update_flags(sinfo_t* sinfo, const char* keyval)
{
    int val = 0;

    sscanf(keyval, CFGKEY_CONVERGED "=%n", &val);
    if (val) {
        if (keyval[val] == '1')
            sinfo->status |= STATUS_CONVERGED;
        else
            sinfo->status &= ~STATUS_CONVERGED;
        return;
    }

    sscanf(keyval, CFGKEY_PAUSED "=%n", &val);
    if (val) {
        if (keyval[val] == '1')
            sinfo->status |= STATUS_PAUSED;
        else
            sinfo->status &= ~STATUS_PAUSED;
        return;
    }
}

int update_state(sinfo_t* sinfo)
{
    if (mesg.type == HMESG_SESSION ||
        mesg.type == HMESG_JOIN)
    {
        // Session state doesn't need to be updated yet.
        return 0;
    }

    if (mesg.state.space->id > sinfo->space.id) {
        if (hspace_copy(&sinfo->space, mesg.state.space) != 0) {
            perror("Could not copy search space");
            return -1;
        }
    }

    if (mesg.state.best->id > sinfo->best.id) {
        int i;
        for (i = sinfo->log_len - 1; i >= 0; --i) {
            if (sinfo->log[i].pt.id == mesg.state.best->id) {
                sinfo->best_perf = sinfo->log[i].perf;
                break;
            }
        }
        if (i < 0)
            sinfo->best_perf = NAN;

        if (hpoint_copy(&sinfo->best, mesg.state.best) != 0) {
            perror("Internal error copying hpoint to best");
            return -1;
        }

        if (hpoint_align(&sinfo->best, &sinfo->space) != 0) {
            perror("Could not align best point to search space");
            return -1;
        }
    }
    return 0;
}


int append_http_log(sinfo_t* sinfo, const hpoint_t* pt, double perf)
{
    http_log_t* entry;

    // Extend HTTP log if necessary.
    if (sinfo->log_len == sinfo->log_cap) {
        if (array_grow(&sinfo->log, &sinfo->log_cap,
                       sizeof(*sinfo->log)) != 0)
        {
            perror("Could not grow HTTP log");
            return -1;
        }
    }
    entry = &sinfo->log[sinfo->log_len];

    if (hpoint_copy(&entry->pt, pt) != 0) {
        perror("Internal error copying point into HTTP log");
        return -1;
    }

    entry->perf = perf;
    if (gettimeofday(&entry->stamp, NULL) != 0)
        return -1;

    ++sinfo->log_len;
    return 0;
}

sinfo_t* sinfo_by_id(int id)
{
    for (int i = 0; i < slist_cap; ++i) {
        if (slist[i].name && slist[i].id == id)
            return &slist[i];
    }
    return NULL;
}

sinfo_t* sinfo_by_name(const char* name)
{
    for (int i = 0; i < slist_cap; ++i) {
        if (slist[i].name && strcmp(slist[i].name, name) == 0)
            return &slist[i];
    }
    return NULL;
}

int search_setcfg(sinfo_t* sinfo, const char* key, const char* val)
{
    char* buf = sprintf_alloc("%s=%s", key, val ? val : "");
    int retval = 0;

    mesg.dest = sinfo->id;
    mesg.src = -1;
    mesg.type = HMESG_SETCFG;
    mesg.status = HMESG_STATUS_REQ;
    mesg.data.string = buf;
    mesg.state.space = &sinfo->space;
    mesg.state.best = &sinfo->best;
    mesg.state.client = "<hserver>";

    if (mesg_send(session_fd, &mesg) < 1)
        retval = -1;

    free(buf);
    return retval;
}

int search_refresh(sinfo_t* sinfo)
{
    mesg.dest = sinfo->id;
    mesg.src = -1;
    mesg.type = HMESG_GETCFG;
    mesg.status = HMESG_STATUS_REQ;
    mesg.state.space = &sinfo->space;
    mesg.state.best = &sinfo->best;
    mesg.state.client = "<hserver>";

    mesg.data.string = CFGKEY_CONVERGED;
    if (mesg_send(session_fd, &mesg) < 1)
        return -1;

    mesg.data.string = CFGKEY_PAUSED;
    if (mesg_send(session_fd, &mesg) < 1)
        return -1;

    return 0;
}

int search_restart(sinfo_t* sinfo)
{
    int retval = 0;

    mesg.dest = sinfo->id;
    mesg.src = -1;
    mesg.type = HMESG_RESTART;
    mesg.status = HMESG_STATUS_REQ;
    mesg.state.space = &sinfo->space;
    mesg.state.best = &sinfo->best;
    mesg.state.client = "<hserver>";

    if (mesg_send(session_fd, &mesg) < 1)
        retval = -1;

    return retval;
}
