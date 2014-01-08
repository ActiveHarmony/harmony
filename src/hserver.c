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
    int idx, retval, client_idx;
    session_state_t *sess;

    sess = NULL;
    retval = mesg_recv(fd, &mesg_in);


    if (retval == 0)
        goto shutdown;
    if (retval < 0)
        goto error;

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

    case HMESG_FETCH:
    case HMESG_GETCFG:
    case HMESG_SETCFG:
        break;

    case HMESG_REPORT:
        /* Before we forward this report to the session-core process,
         * add it to our HTTP log.
         */
        if (mesg_in.data.report.cand.id == -1)
            break;

        /* keep track of last point id in case there's a restart,
           in which case sess->restart_id is set to sess->last_id */
        sess->last_id = mesg_in.data.report.cand.id;

        if (sess->log_len == sess->log_cap) {
            if (array_grow(&sess->log, &sess->log_cap,
                           sizeof(http_log_t)) < 0)
            {
                perror("Could not grow HTTP log");
                goto error;
            }
        }
        if (gettimeofday(&sess->log[sess->log_len].stamp, NULL) != 0)
            goto error;

        if (hpoint_copy(&sess->log[sess->log_len].pt,
                        &mesg_in.data.report.cand) < 0)
        {
            perror("Internal error copying hpoint to HTTP log");
            goto error;
        }
        sess->log[sess->log_len].perf = mesg_in.data.report.perf;
        ++sess->log_len;

        /* Also update our best point data, if necessary. */
        /* If busy is set indicating that the session is paused, don't bother
        to update the best point. Data received while paused isn't really relevant
        to the session anyways, and sometimes causes a realloc failure for some reason
        if we do try to update best point data */
        if (sess->best_perf > mesg_in.data.report.perf && mesg_in.status != HMESG_STATUS_BUSY) {
            if (hpoint_copy(&sess->best, &mesg_in.data.report.cand) < 0) {
                perror("Internal error copying best point information");
                goto error;
            }
            sess->best_perf = mesg_in.data.report.perf;
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

    // For restart, we need to clear client state. 
    // Saving the last message type received makes that easier

    /* look up the client in session_state's client list by fd */
    for(client_idx = 0;client_idx < sess->client_len;client_idx++) {
      if(sess->client[client_idx] == fd) break;
    }

    // Case for fetch msg for paused session. deflect fetch messages
    // and tell the client the session is busy so it keeps
    // testing the current point
    if (sess->paused && mesg_in.type == HMESG_FETCH) {
      // client sees BUSY and replies with the busy status
      // so the session knows that the subsequent report (with busy status)
      // is for a paused session and therefore should be deflected 
      // as well.
      mesg_out.status = HMESG_STATUS_BUSY;
      // this message is going back to the client so the destination
      // should indicate which session it belongs to.
      mesg_out.dest = idx;
      // send message back to client
      if (mesg_send(fd, &mesg_out) < 1) {
        perror("Error deflecting fetch message back to client (session paused)");
        FD_CLR(sess->fd, &listen_fds);
        session_close(sess);
        goto shutdown; 
      }
    } 
    // Case for client reporting to a paused session 
    // the session shouldn't be getting these reports.
    // no checking of paused variable, because right when session is resumed it'll
    // still get a message back from the client that corresponds to data gathered
    // while it was paused. We still need to respond to that message with the "paused" response
    else if (mesg_in.type == HMESG_REPORT && mesg_in.status == HMESG_STATUS_BUSY) {
      // Tell the client it's all good
      mesg_out.status = HMESG_STATUS_OK;
      // Going back to client, so indicate the session it belongs to
      mesg_out.dest = idx;
      // send message back to client. Keep the session out of the loop while things are paused
      if (mesg_send(fd, &mesg_out) < 1) {
        perror("Error deflecting report message back to client (session paused)");
        FD_CLR(sess->fd, &listen_fds);
        session_close(sess);
        goto shutdown;  
      }
    }
    /* case for message to a defunct session not needed - 
       the new session's ids start from 1, but that doesn't seem
        to knock anything out of place */
    /* base case - forward message to session as normal */
    else if (mesg_send(sess->fd, &mesg_out) < 1) {
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

    switch (mesg_in.type) {
    case HMESG_SESSION:
        /* after a restart, the hserver-generated session message's response
           should not be forwarded to the client */
        if (sess->restart_id > 0) {
          mesg_in.type = HMESG_UNKNOWN;
          return 0;
        }
    case HMESG_JOIN:
    case HMESG_SETCFG:
    case HMESG_GETCFG:
    case HMESG_FETCH:
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
    const char *strategy_name;

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
    sess->best = HPOINT_INITIALIZER;
    sess->best_perf = INFINITY;

    if (hcfg_merge(mesg->data.session.cfg, cfg) < 0) {
        mesg_out.data.string = "Internal error merging config structures";
        goto error;
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

    sess->reported = 0;

    if (gettimeofday(&sess->start, NULL) < 0)
        goto error;

    sess->log_len = 0;

    FD_SET(sess->fd, &listen_fds);
    if (highest_socket < sess->fd)
        highest_socket = sess->fd;

    sess->paused = 0; /* session should not start paused or converged */
    sess->converged = 0; 

    /* save session strategy */
    strategy_name = hcfg_get(mesg->data.session.cfg, CFGKEY_SESSION_STRATEGY);
    if(strategy_name == NULL)
      sess->strategy_name = DEFAULT_STRATEGY;
    else {
      sess->strategy_name = malloc(strlen(strategy_name) + 1);
      strcpy(sess->strategy_name, strategy_name);
    }

    /* save src_id string for restart */
    sess->src_id = malloc(strlen(mesg->src_id) + 1);
    strcpy(sess->src_id, mesg->src_id);

    /* also save initial hcfg for restart */
    sess->ini_hcfg = hcfg_copy(mesg->data.session.cfg);

    /* set restart id */
    sess->restart_id = 0;
    sess->last_id = 0;

    return sess;

  error:
    free(sess->name);
    sess->name = NULL;
    return NULL;
}

/* Restarts the session with the specified name:
    - Closes the old session
    - Opens a new session, and configures it in the same way 
      as the previous one.
   On failure, simply returns without restarting.
   args:
     -name = session name
     -stuff after name; = point (or simplex) to start from. semicolon-delimited list of coords */
void session_restart(char *name) {
  int idx, i;
  char *child_argv[2];
  char *coord_ptr = NULL;
  session_state_t *sess = NULL;
  hmesg_t ini_mesg = HMESG_INITIALIZER;
  
  /* check for extra params (is semicolon present?) */
  for(i = 0;i < strlen(name);i++) {
    if(name[i] == ';') {
      name[i] = '\0';   // new str now starts after this. prevents mishaps with name comp later on
      coord_ptr = &name[i] + 1;
      break;
    }
  }
  
  /* Look up session to restart by name */
  for (idx = 0; idx < slist_cap; ++idx) {
    if (slist[idx].name && strcmp(slist[idx].name, name) == 0) {
      sess = &slist[idx]; 
      break;
    }
  }
  
  if (idx == slist_cap) { /* session with specified name not found */
    return;
  }

  /* Close old session (close socket so it dies) */
  printf("Restart: old fd is %d\n", sess->fd);
  close(sess->fd);

  /* Start new session:
      Fork and exec a session handler. 
      Set sess->fd to that of the new session */
  child_argv[0] = sess->name;
  child_argv[1] = NULL;
  sess->fd = socket_launch(session_bin, child_argv, NULL);

  /* so that messages with id <= id of last message from client
     before restart are not passed on to session */
  sess->restart_id = sess->last_id;
  
  /* send initial session message */
  hmesg_scrub(&ini_mesg);
  ini_mesg.dest = sess->fd;
  ini_mesg.type = HMESG_SESSION;
  ini_mesg.status = HMESG_STATUS_REQ;
  ini_mesg.src_id = sess->src_id;
  hsession_init(&ini_mesg.data.session);
  /* hsignature does not change */
  hsignature_copy(&ini_mesg.data.session.sig, &sess->sig);

  /* replay initial configuration */
  /* coords provided -> set that in cfg. otherwise just replay initial config */
  ini_mesg.data.session.cfg = hcfg_copy(sess->ini_hcfg);
  if(coord_ptr != NULL) {
    hcfg_set(ini_mesg.data.session.cfg, CFGKEY_POINT_DATA, coord_ptr);
    hcfg_set(ini_mesg.data.session.cfg, CFGKEY_INIT_METHOD, "point_set");
  } 
  
  mesg_send(sess->fd, &ini_mesg);
}

void session_close(session_state_t *sess)
{
    int i;

    free(sess->name);
    sess->name = NULL;
    free(sess->src_id);
    sess->src_id = NULL;
    hpoint_fini(&sess->best);

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
