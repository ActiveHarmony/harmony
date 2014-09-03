/*
 * Copyright 2003-2013 Jeffrey K. Hollingsworth
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
#include "defaults.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <libgen.h>
#include <signal.h>
#include <math.h>

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

/*
 * Function prototypes
 */
int vars_init(int argc, char *argv[]);
int network_init(void);
int handle_new_connection(int fd);
int handle_unknown_connection(int fd);
int handle_client_socket(int fd);
int handle_session_socket(int idx);
int available_index(int **list, int *cap);
void client_close(int fd);
int append_http_log(session_state_t *sess, const hpoint_t *pt, double perf);

/*
 * Main variables
 */
int listen_socket;
fd_set listen_fds;
int highest_socket;

int *unk_fds, unk_cap;
int *client_fds, client_cap;
int *http_fds, http_len, http_cap;

hcfg_t *cfg;
char *harmony_dir;
char *session_bin;
hmesg_t mesg_in, mesg_out; /* Maintain two message structures
                            * to avoid overwriting buffers.
                            */
session_state_t *slist;
int slist_cap;

/*
 * Local variables
 */
static int debug_mode = 1;

int main(int argc, char *argv[])
{
    int i, fd_count, retval;
    fd_set ready_fds;

    /* Initialize global variable state. */
    if (vars_init(argc, argv) < 0)
        return -1;

    /* Initialize the HTTP user interface. */
    if (http_init(harmony_dir) < 0)
        return -1;

    /* Initialize socket networking service. */
    if (network_init() < 0)
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
            /* Before all else, handle input from session instances. */
            for (i = 0; i < slist_cap; ++i) {
                if (slist[i].name && FD_ISSET(slist[i].fd, &ready_fds)) {
                    if (handle_session_socket(i) < 0) {
                        FD_CLR(slist[i].fd, &listen_fds);
                    }
                }
            }

            /* Handle new connections */
            if (FD_ISSET(listen_socket, &ready_fds)) {
                retval = handle_new_connection(listen_socket);
                if (retval > 0) {
                    FD_SET(retval, &listen_fds);
                    if (highest_socket < retval)
                        highest_socket = retval;
                }
            }

            /* Handle unknown connections (Unneeded if we switch to UDP) */
            for (i = 0; i < unk_cap; ++i) {
                if (unk_fds[i] == -1)
                    continue;

                if (FD_ISSET(unk_fds[i], &ready_fds)) {
                    if (handle_unknown_connection(unk_fds[i]) < 0)
                        FD_CLR(unk_fds[i], &listen_fds);
                    unk_fds[i] = -1;
                }
            }

            /* Handle harmony messages */
            for (i = 0; i < client_cap; ++i) {
                if (client_fds[i] == -1)
                    continue;

                if (FD_ISSET(client_fds[i], &ready_fds)) {
                    if (handle_client_socket(client_fds[i]) < 0) {
                        client_close(client_fds[i]);
                        client_fds[i] = -1;
                    }
                }
            }

            /* Handle http requests */
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
    return 0;
}

int vars_init(int argc, char *argv[])
{
    char *tmppath, *binfile, *cfgpath;

    /*
     * Install proper signal handling.
     */
    signal(SIGPOLL, SIG_IGN);

    /*
     * Determine directory where this binary is located.
     */
    tmppath = stralloc(argv[0]);
    binfile = stralloc(basename(tmppath));
    free(tmppath);

    cfg = hcfg_alloc();
    if (!cfg) {
        perror("Could not initialize the global configuration structure");
        return -1;
    }

    if ( (tmppath = getenv(CFGKEY_HARMONY_HOME))) {
        harmony_dir = stralloc(tmppath);
        printf(CFGKEY_HARMONY_HOME " is %s\n", harmony_dir);
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

        printf("Detected %s/ as " CFGKEY_HARMONY_HOME "\n", harmony_dir);
    }
    free(binfile);

    if (hcfg_set(cfg, CFGKEY_HARMONY_HOME, harmony_dir) < 0) {
        perror("Could not set " CFGKEY_HARMONY_HOME " in global config");
        return -1;
    }

    /*
     * Find supporting binaries and shared objects.
     */
    session_bin = sprintf_alloc("%s/libexec/" SESSION_CORE_EXECFILE,
                                harmony_dir);
    if (!file_exists(session_bin)) {
        fprintf(stderr, "Could not find support files in HARMONY_HOME\n");
        return -1;
    }

    /*
     * Config file search
     */
    if (argc > 1) {
        /* Option #1: Command-line parameter. */
        cfgpath = stralloc(argv[1]);
    }
    else {
        /* Option #2: Environment variable. */
        if ( (tmppath = getenv("HARMONY_CONFIG"))) {
            cfgpath = stralloc(tmppath);
        }
        else {
            /* Option #3: Check if default filename exists. */
            cfgpath = sprintf_alloc("%s/bin/" DEFAULT_CONFIG_FILENAME,
                                    harmony_dir);
            if (!file_exists(cfgpath)) {
                free(cfgpath);
                cfgpath = NULL;
            }
        }
    }

    /*
     * Load config file, if found.
     */
    if (cfgpath) {
        if (strcmp(cfgpath, "-") == 0)
            printf("Reading config values from <stdin>\n");
        else
            printf("Reading config values from %s\n", cfgpath);

        if (hcfg_load(cfg, cfgpath) != 0)
            return -1;

        free(cfgpath);
    }
    else {
        printf("No config file found.  Proceeding with default values.\n");
    }

    mesg_in = HMESG_INITIALIZER;
    mesg_out = HMESG_INITIALIZER;

    /*
     * Prepare the file descriptor lists.
     */
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
    if (array_grow(&slist, &slist_cap, sizeof(session_state_t)) < 0) {
        perror("Could not allocate memory for session file descriptor list");
        return -1;
    }

    return 0;
}

int network_init(void)
{
    const char *cfgval;
    int listen_port, optval;
    struct sockaddr_in addr;

    /* try to get the port info from the environment */
    if (getenv("HARMONY_S_PORT") != NULL) {
        listen_port = atoi(getenv("HARMONY_S_PORT"));
    }
    else if ( (cfgval = hcfg_get(cfg, CFGKEY_SERVER_PORT))) {
        listen_port = atoi(cfgval);
    }
    else {
        printf("Using default TCP port: %d\n", DEFAULT_PORT);
        listen_port = DEFAULT_PORT;
    }

    /* create a listening socket */
    listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket < 0) {
        perror("Could not create listening socket");
        return -1;
    }

    /* set socket options. We try to make the port reusable and have it
       close as fast as possible without waiting in unnecessary wait states
       on close. */
    optval = 1;
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR,
                   &optval, sizeof(optval)) < 0)
    {
        perror("Could not set options on listening socket");
        return -1;
    }

    /* Initialize the socket address. */
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(listen_port);

    /* Bind the socket to the desired port. */
    if (bind(listen_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Could not bind socket to listening address");
        return -1;
    }

    /*
     * Set the file descriptor set
     */
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

    newfd = accept(fd, (struct sockaddr *)&addr, &addrlen);
    if (newfd < 0) {
        perror("Error accepting new connection");
        return -1;
    }

    /* The peer may not have sent data yet.  To prevent blocking, lets
     * stash the new socket in the unknown_list until we know it has data.
     */
    idx = available_index(&unk_fds, &unk_cap);
    if (idx < 0) {
        perror("Could not grow unknown connection array");
        return -1;
    }

    unk_fds[idx] = newfd;
    if (debug_mode) printf("Accepted connection from %s as socket %d\n",
                           inet_ntoa(addr.sin_addr), newfd);
    return newfd;
}

int handle_unknown_connection(int fd)
{
    unsigned int header;
    int idx, readlen;

    readlen = recv(fd, &header, sizeof(header), MSG_PEEK | MSG_DONTWAIT);
    if (readlen < 0 || (unsigned int)readlen < sizeof(header)) {
        /* Error on recv, or insufficient data.  Close the connection. */
        printf("Can't determine type for socket %d.  Closing.\n", fd);
        if (close(fd) < 0)
            perror("Error closing connection");
        return -1;
    }
    else if (ntohl(header) == HMESG_MAGIC) {
        /* This is a communication socket from a Harmony client. */
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
        /* Consider this an HTTP communication socket. */
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
    int idx, i, retval;
    double perf;
    session_state_t *sess;

    sess = NULL;
    retval = mesg_recv(fd, &mesg_in);
    if (retval == 0)
        goto shutdown;
    if (retval < 0)
        goto error;

    if (mesg_in.dest == -1) {
        /* Message was intended to be ignored. */
        return 0;
    }

    /* Sanity check input */
    if (mesg_in.type != HMESG_SESSION && mesg_in.type != HMESG_JOIN) {
        idx = mesg_in.dest;
        if (idx < 0 || idx >= slist_cap || slist[idx].name == NULL) {
            errno = EINVAL;
            goto error;
        }
        sess = &slist[idx];
    }

    switch (mesg_in.type) {
    case HMESG_GETCFG:
    case HMESG_SETCFG:
    case HMESG_BEST:
    case HMESG_FETCH:
    case HMESG_RESTART:
        break;

    case HMESG_SESSION:
        sess = session_open(&mesg_in);
        if (!sess) {
            errno = EINVAL;
            goto error;
        }
        break;

    case HMESG_JOIN:
        for (idx = 0; idx < slist_cap; ++idx) {
            if (!slist[idx].name)
                continue;
            if (strcmp(slist[idx].name, mesg_in.data.join.name) == 0)
                break;
        }
        if (idx == slist_cap) {
            errno = EINVAL;
            goto error;
        }
        sess = &slist[idx];

        idx = available_index(&sess->client, &sess->client_cap);
        if (idx < 0) {
            perror("Could not grow session client list");
            goto error;
        }

        sess->client[idx] = fd;
        ++sess->client_len;
        break;

    case HMESG_REPORT:
        perf = hperf_unify(mesg_in.data.report.perf);

        for (i = 0; i < sess->fetched_len; ++i) {
            if (sess->fetched[i].id == mesg_in.data.report.cand_id)
                break;
        }
        if (i < sess->fetched_len) {
            const hpoint_t *pt = &sess->fetched[i];

            /* Copy point from fetched list to HTTP log. */
            if (append_http_log(sess, pt, perf) != 0) {
                perror("Internal error copying fetched point to HTTP log");
                goto error;
            }

            /* Also update our best point data, if necessary. */
            if (sess->best_perf > perf && sess->best.id != pt->id) {
                sess->best_perf = perf;
                if (hpoint_copy(&sess->best, pt) != 0) {
                    perror("Internal error copying hpoint to best");
                    goto error;
                }
            }

            /* Remove point from fetched list. */
            --sess->fetched_len;
            if (i < sess->fetched_len) {
                if (hpoint_copy(&sess->fetched[i],
                                &sess->fetched[sess->fetched_len]) != 0)
                {
                    perror("Internal error copying hpoint within fetch log");
                    goto error;
                }
            }
        }
        else {
            /* Copy point from fetched list to HTTP log. */
            if (append_http_log(sess, &sess->best, perf) != 0) {
                perror("Internal error copying best point to HTTP log");
                goto error;
            }
        }
        break;

    default:
        goto error;
    }

    mesg_out.dest = fd;
    mesg_out.type = mesg_in.type;
    mesg_out.status = mesg_in.status;
    mesg_out.src_id = mesg_in.src_id;
    mesg_out.data = mesg_in.data;
    if (mesg_send(sess->fd, &mesg_out) < 1) {
        perror("Error forwarding message to session");
        FD_CLR(sess->fd, &listen_fds);
        session_close(sess);
        goto shutdown;
    }
    /* mesg_in data was free()'ed along with mesg_out. */
    mesg_in.type = HMESG_UNKNOWN;

    return 0;

  error:
    mesg_out.type = mesg_in.type;
    if (mesg_out.status != HMESG_STATUS_FAIL) {
        mesg_out.status = HMESG_STATUS_FAIL;
        mesg_out.data.string = strerror(errno);
    }
    mesg_send(fd, &mesg_out);

  shutdown:
    return -1;
}

int handle_session_socket(int idx)
{
    session_state_t *sess;

    sess = &slist[idx];
    if (mesg_recv(sess->fd, &mesg_in) < 1)
        goto error;

    if (mesg_in.dest == -1) {
        if (mesg_in.status == HMESG_STATUS_REQ) {
            if (handle_http_info(sess, (char *)mesg_in.data.string) != 0) {
                perror("Error handling http info message");
                goto error;
            }
        }
        return 0;
    }

    switch (mesg_in.type) {
    case HMESG_SESSION:
    case HMESG_JOIN:
    case HMESG_SETCFG:
    case HMESG_GETCFG:
    case HMESG_BEST:
    case HMESG_RESTART:
        break;

    case HMESG_FETCH:
        if (mesg_in.status == HMESG_STATUS_OK) {
            /* Log this point before we forward it to the client. */
            if (sess->fetched_len == sess->fetched_cap) {
                if (array_grow(&sess->fetched, &sess->fetched_cap,
                               sizeof(hpoint_t *)) != 0)
                {
                    perror("Could not grow fetch log");
                    goto error;
                }
            }

            if (hpoint_copy(&sess->fetched[sess->fetched_len],
                            &mesg_in.data.point) != 0)
            {
                perror("Internal error copying hpoint to HTTP log");
                goto error;
            }
            ++sess->fetched_len;
        }
        break;

    case HMESG_REPORT:
        if (mesg_in.status == HMESG_STATUS_OK)
            ++sess->reported;
        break;

    default:
        errno = EINVAL;
        goto error;
    }

    mesg_out.dest = idx;
    mesg_out.type = mesg_in.type;
    mesg_out.status = mesg_in.status;
    mesg_out.src_id = mesg_in.src_id;
    mesg_out.data = mesg_in.data;
    if (mesg_send(mesg_in.dest, &mesg_out) < 1) {
        perror("Error forwarding message to client");
        client_close(mesg_in.dest);
    }
    /* mesg_in data was free()'ed along with mesg_out. */
    mesg_in.type = HMESG_UNKNOWN;

    return 0;

  error:
    session_close(sess);
    return -1;
}

session_state_t *session_open(hmesg_t *mesg)
{
    int i, idx, clients_expected;
    session_state_t *sess;
    char *child_argv[2];
    const char *cfgstr;

    idx = -1;
    /* check if session already exists, and return if it does */
    for (i = 0; i < slist_cap; ++i) {
        if (!slist[i].name) {
            if (idx < 0)
                idx = i;
        }
        else if (strcmp(slist[i].name, mesg->data.session.sig.name) == 0) {
            mesg_out.status = HMESG_STATUS_FAIL;
            mesg_out.data.string = "Session name already exists.";
            return NULL;
        }
    }
    /* expand session list arr if necessary, and add
       the new session to the session list arr */
    if (idx == -1) {
        idx = slist_cap;
        if (array_grow(&slist, &slist_cap, sizeof(session_state_t)) < 0)
            return NULL;
    }
    sess = &slist[idx];

    sess->name = stralloc(mesg->data.session.sig.name);
    if (!sess->name)
        return NULL;

    sess->client_len = 0;
    hpoint_fini(&sess->best);
    sess->best_perf = INFINITY;

    if (hcfg_merge(mesg->data.session.cfg, cfg) < 0) {
        mesg_out.data.string = "Internal error merging config structures";
        goto error;
    }

    /* Force sessions to load the httpinfo plugin layer. */
    #define HTTPINFO "httpinfo.so"
    cfgstr = hcfg_get(mesg->data.session.cfg, CFGKEY_SESSION_LAYERS);
    if (!cfgstr || *cfgstr == '\0') {
        hcfg_set(mesg->data.session.cfg, CFGKEY_SESSION_LAYERS, HTTPINFO);
    }
    else if (strstr(cfgstr, HTTPINFO) == NULL) {
        char *buf = sprintf_alloc(HTTPINFO ";%s", cfgstr);
        hcfg_set(mesg->data.session.cfg, CFGKEY_SESSION_LAYERS, buf);
        free(buf);
    }

    /* Make sure we have at least 1 expected client. */
    cfgstr = hcfg_get(mesg->data.session.cfg, CFGKEY_CLIENT_COUNT);
    if (!cfgstr) {
        clients_expected = DEFAULT_CLIENT_COUNT;
    }
    else {
        clients_expected = atoi(cfgstr);
    }
    if (clients_expected < 1) {
        mesg_out.data.string = "Invalid config value for " CFGKEY_CLIENT_COUNT;
        goto error;
    }

    /* Fork and exec a session handler. */
    child_argv[0] = sess->name;
    child_argv[1] = NULL;
    sess->fd = socket_launch(session_bin, child_argv, NULL);
    if (sess->fd < 0)
        goto error;

    /* Initialize HTTP server fields. */
    if (hsignature_copy(&sess->sig, &mesg->data.session.sig) != 0)
        goto error;

    if (gettimeofday(&sess->start, NULL) < 0)
        goto error;

    cfgstr = hcfg_get(mesg->data.session.cfg, CFGKEY_SESSION_STRATEGY);
    sess->strategy = stralloc(cfgstr);
    sess->converged = 0;
    sess->log_len = 0;
    sess->reported = 0;

    FD_SET(sess->fd, &listen_fds);
    if (highest_socket < sess->fd)
        highest_socket = sess->fd;

    return sess;

  error:
    free(sess->name);
    sess->name = NULL;
    return NULL;
}

void session_close(session_state_t *sess)
{
    int i;

    free(sess->strategy);
    free(sess->name);
    sess->name = NULL;

    for (i = 0; i < sess->client_cap; ++i) {
        if (sess->client[i] != -1)
            client_close(sess->client[i]);
    }

    hsignature_fini(&sess->sig);
}

void client_close(int fd)
{
    int i, j;

    for (i = 0; i < slist_cap; ++i) {
        for (j = 0; j < slist[i].client_cap; ++j) {
            if (slist[i].client[j] == fd) {
                slist[i].client[j] = -1;
                --slist[i].client_len;
                break;
            }
        }
    }

    for (i = 0; i < client_cap; ++i) {
        if (client_fds[i] == fd) {
            client_fds[i] = -1;
            break;
        }
    }

    if (close(fd) < 0)
        perror("Error closing client connection post error");

    FD_CLR(fd, &listen_fds);
}

int available_index(int **list, int *cap)
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

int append_http_log(session_state_t *sess, const hpoint_t *pt, double perf)
{
    http_log_t *entry;

    /* Extend HTTP log if necessary. */
    if (sess->log_len == sess->log_cap) {
        if (array_grow(&sess->log, &sess->log_cap, sizeof(http_log_t)) != 0) {
            perror("Could not grow HTTP log");
            return -1;
        }
    }
    entry = &sess->log[sess->log_len];

    if (hpoint_copy(&entry->pt, pt) != 0) {
        perror("Internal error copying point into HTTP log");
        return -1;
    }

    entry->perf = perf;
    if (gettimeofday(&entry->stamp, NULL) != 0)
        return -1;

    ++sess->log_len;
    return 0;
}

int session_setcfg(session_state_t *sess, const char *key, const char *val)
{
    hmesg_t mesg = HMESG_INITIALIZER;
    char *buf = sprintf_alloc("%s=%s", key, val ? val : "");
    int retval = 0;

    mesg.dest = -1;
    mesg.type = HMESG_SETCFG;
    mesg.status = HMESG_STATUS_REQ;
    mesg.data.string = buf;

    if (mesg_send(sess->fd, &mesg) < 1)
        retval = -1;

    free(buf);
    hmesg_fini(&mesg);
    return retval;
}

int session_restart(session_state_t *sess)
{
    hmesg_t mesg = HMESG_INITIALIZER;
    int retval = 0;

    mesg.dest = -1;
    mesg.type = HMESG_RESTART;
    mesg.status = HMESG_STATUS_REQ;

    if (mesg_send(sess->fd, &mesg) < 1)
        retval = -1;

    hmesg_fini(&mesg);
    return retval;
}
