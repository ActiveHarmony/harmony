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
#include "hutil.h"
#include "hsockutil.h"
#include "defaults.h" /* needed for looking cfg params up */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#define HTTP_ENDL "\r\n"

/* The following opt_* definitions allow the compiler to possibly
 * optimize away a call to strlen().
 */
#define opt_sock_write(x, y) socket_write((x), (y), strlen(y))
#define opt_http_write(x, y) http_chunk_send((x), (y), strlen(y))
#define opt_strncmp(x, y) strncmp((x), (y), strlen(y))

unsigned int http_connection_limit = 32;

typedef enum {
    CONTENT_HTML,
    CONTENT_JAVASCRIPT,
    CONTENT_CSS
} content_t;

typedef struct {
    const char *filename;
    content_t type;
    char *buf;
    unsigned buflen;
} memfile_t;

static memfile_t html_file[] = {
    /* The overview html file must be first in this array. */
    { "overview.cgi",                 CONTENT_HTML,       NULL, 0 },
    { "overview.js",                  CONTENT_JAVASCRIPT, NULL, 0 },
    { "session-view.cgi",             CONTENT_HTML,       NULL, 0 },
    { "session-view.js",              CONTENT_JAVASCRIPT, NULL, 0 },
    { "common.js",                    CONTENT_JAVASCRIPT, NULL, 0 },
    { "activeharmony.css",            CONTENT_CSS,        NULL, 0 },
    { "jquery.min.js",                CONTENT_JAVASCRIPT, NULL, 0 },
    { "jquery.flot.min.js",           CONTENT_JAVASCRIPT, NULL, 0 },
    { "jquery.flot.time.min.js",      CONTENT_JAVASCRIPT, NULL, 0 },
    { "jquery.flot.resize.min.js",    CONTENT_JAVASCRIPT, NULL, 0 },
    { "jquery.flot.selection.min.js", CONTENT_JAVASCRIPT, NULL, 0 },
    { "excanvas.min.js",              CONTENT_JAVASCRIPT, NULL, 0 },
    /* A null entry must end this array. */
    { NULL, CONTENT_HTML, NULL, 0 }
};

static const char status_200[] = "HTTP/1.1 200 OK" HTTP_ENDL;
static const char status_400[] = "HTTP/1.1 400 Bad Request" HTTP_ENDL;
static const char status_404[] = "HTTP/1.1 404 Not Found" HTTP_ENDL;
static const char status_501[] = "HTTP/1.1 501 Not Implemented" HTTP_ENDL;
static const char status_503[] = "HTTP/1.1 503 Service Unavailable" HTTP_ENDL;

static const char http_type_html[] = "Content-Type: text/html" HTTP_ENDL;
static const char http_type_text[] = "Content-Type: text/plain" HTTP_ENDL;
static const char http_type_js[] = "Content-Type: text/javascript" HTTP_ENDL;
static const char http_type_css[] = "Content-Type: text/css" HTTP_ENDL;

static const char http_headers[] =
    "Connection: Keep-Alive"     HTTP_ENDL
    "Keep-Alive: max=1024"       HTTP_ENDL
    "Cache-Control: no-cache"    HTTP_ENDL
    "Transfer-Encoding: chunked" HTTP_ENDL;

char sendbuf[8192];  /* Static buffer used for outgoing HTTP data. */
char recvbuf[10240]; /* Static buffer used for incoming HTTP data. */

/* Internal helper function declarations */
char *uri_decode(char *buf);
int http_request_handle(int fd, char *req);
char *http_request_recv(int fd, char *buf, int buflen, char **data);
int http_chunk_send(int fd, const char *data, int datalen);
int http_overview_send(int fd);
int http_session_send(int fd, const char *req);
char *http_session_header(session_state_t *sess, struct timeval *tv);
int report_append(char **buf, int *buflen, session_state_t *sess,
                  struct timeval *tv, const hpoint_t *pt, const double perf);

int http_init(const char *basedir)
{
    int fd, i;
    char *filename;
    struct stat sb;

    for (i = 0; html_file[i].filename != NULL; ++i) {
        if (html_file[i].buf != NULL)
            munmap(html_file[i].buf, html_file[i].buflen);

        filename = sprintf_alloc("%s/libexec/html/%s", basedir,
                                 html_file[i].filename);
        if (!filename) {
            perror("Could not allocate temp memory for filename");
            goto error;
        }

        fd = open(filename, O_RDONLY);
        if (fd == -1) {
            fprintf(stderr, "Error on open(%s): %s\n",
                    filename, strerror(errno));
            goto error;
        }

        /* Obtain file size */
        if (fstat(fd, &sb) == -1) {
            fprintf(stderr, "Error on fstat(%s): %s",
                    filename, strerror(errno));
            goto error;
        }
        html_file[i].buflen = sb.st_size;

        html_file[i].buf = (char *)mmap(NULL, html_file[i].buflen,
                                        PROT_READ, MAP_PRIVATE, fd, 0);
        if (html_file[i].buf == (char *)-1) {
            fprintf(stderr, "Error on mmap(%s): %s",
                    filename, strerror(errno));
            goto error;
        }

        if (close(fd) != 0)
            fprintf(stderr, "Ignoring error on close(%s): %s",
                    filename, strerror(errno));

        free(filename);
    }
    return 0;

  error:
    free(filename);
    return -1;
}

void http_send_error(int fd, int status, const char *arg)
{
    const char *status_line;
    const char *message = NULL;

    if (status == 400) {
        status_line = status_400;
        message = "The following request is malformed: ";

    } else if (status == 404) {
        status_line = status_404;
        message = "The requested URL could not be found: ";

    } else if (status == 501) {
        status_line = status_501;
        message = "The following request method has not been implemented: ";

    } else if (status == 503) {
        status_line = status_503;
        message = "The maximum number of HTTP connections has been exceeded.";
    }

    opt_sock_write(fd, status_line);
    opt_sock_write(fd, http_type_html);
    opt_sock_write(fd, http_headers);
    opt_sock_write(fd, HTTP_ENDL);
    opt_http_write(fd, "<html><head><title>");
    opt_http_write(fd, status_line +  9);
    opt_http_write(fd, "</title></head><body><h1>");
    opt_http_write(fd, status_line + 13);
    opt_http_write(fd, "</h1>");
    if (message || arg) {
        opt_http_write(fd, "<p>");
        if (message)
            opt_http_write(fd, message);
        if (arg)
            opt_http_write(fd, arg);
        opt_http_write(fd, "</p>");
    }
    opt_http_write(fd, "<hr>Active Harmony HTTP Server</body></html>");
    opt_http_write(fd, "");
}

int handle_http_socket(int fd)
{
    static char buf[4096];
    char *ptr = NULL, *req, *endp;

    errno = 0;
    req = http_request_recv(fd, buf, sizeof(buf), &ptr);
    while (req != NULL) {
        if (opt_strncmp(req, "GET ") == 0) {
            endp = strchr(req + 4, ' ');
            if (endp == NULL) {
                http_send_error(fd, 400, req); /* Bad Request */
            }
            else {
                *endp = '\0';
                req = uri_decode(req + 4);
                if (http_request_handle(fd, req) < 0)
                    http_send_error(fd, 404, req);  /* Not Found */
            }
        }
        else if (opt_strncmp(req, "OPTIONS ") == 0 ||
                 opt_strncmp(req, "HEAD ")    == 0 ||
                 opt_strncmp(req, "POST ")    == 0 ||
                 opt_strncmp(req, "PUT ")     == 0 ||
                 opt_strncmp(req, "DELETE ")  == 0 ||
                 opt_strncmp(req, "TRACE ")   == 0 ||
                 opt_strncmp(req, "CONNECT ") == 0) {

            /* Unimplemented Function */
            http_send_error(fd, 501, req);
        }

        req = http_request_recv(fd, buf, sizeof(buf), &ptr);
    }

    if (errno == 0) {
        /* No data returned, and no error reported.  Socket closed by peer. */
        printf("[AH]: Closing socket %d (HTTP Socket)\n", fd);
        if (close(fd) < 0)
            printf("[AH]: Error closing HTTP socket\n");

        return -1;
    }

    return 0;
}

/**************************************
 * Internal helper function definitions
 */
int http_request_handle(int fd, char *req)
{
    char *arg;
    int i;

    arg = strchr(req, '?');
    if (arg) {
        *arg = '\0';
        ++arg;
    }

    if (strcmp(req, "/") == 0) {
        opt_sock_write(fd, status_200);
        opt_sock_write(fd, http_type_html);
        opt_sock_write(fd, http_headers);
        opt_sock_write(fd, HTTP_ENDL);
        http_chunk_send(fd, html_file[0].buf, html_file[0].buflen);
        opt_http_write(fd, "");

        return 0;
    }
    else if (strcmp(req, "/session-list") == 0) {
        opt_sock_write(fd, status_200);
        opt_sock_write(fd, http_type_text);
        opt_sock_write(fd, http_headers);
        opt_sock_write(fd, HTTP_ENDL);

        http_overview_send(fd);
        opt_http_write(fd, "");
        return 0;
    }
    else if (strcmp(req, "/session-data") == 0) {
        opt_sock_write(fd, status_200);
        opt_sock_write(fd, http_type_text);
        opt_sock_write(fd, http_headers);
        opt_sock_write(fd, HTTP_ENDL);

        http_session_send(fd, arg);
        opt_http_write(fd, "");
        return 0;
    }
    else if (strcmp(req, "/kill") == 0) {
        opt_sock_write(fd, status_200);
        opt_sock_write(fd, http_type_text);
        opt_sock_write(fd, http_headers);
        opt_sock_write(fd, HTTP_ENDL);
        if (arg) {
            for (i = 0; i < slist_cap; ++i) {
                if (slist[i].name && strcmp(slist[i].name, arg) == 0) {
                    session_close(&slist[i]);
                    opt_http_write(fd, "OK");
                    opt_http_write(fd, "");
                    return 0;
                }
            }
        }
        opt_http_write(fd, "FAIL");
        opt_http_write(fd, "");
        return 0;

    }
    else if (strncmp(req, "/pause", 6) == 0) {
       // An argument must be supplied to indicate which session to pause
       printf("pause received\n");
       if (arg) {
            for (i = 0; i < slist_cap; ++i) {
                if (slist[i].name && strcmp(slist[i].name, arg) == 0) {
                    slist[i].paused = 1;
                    opt_http_write(fd, "OK");
                    opt_http_write(fd, "");
                    return 0;
                }
            }
        }
        opt_http_write(fd, "FAIL");
        opt_http_write(fd, "");
        return 0;
    } 
    else if (strncmp(req, "/resume", 7) == 0) {
       // argument indicates which session to resume
       if (arg) {
            for (i = 0; i < slist_cap; ++i) {
                if (slist[i].name && strcmp(slist[i].name, arg) == 0) {
                    slist[i].paused = 0;
                    opt_http_write(fd, "OK");
                    opt_http_write(fd, "");
                    return 0;
                }
            }
        }
        opt_http_write(fd, "FAIL");
        opt_http_write(fd, ""); 
        return 0;
    }
    else if (strncmp(req, "/restart", 8) == 0) {    
        opt_sock_write(fd, status_200);
        opt_sock_write(fd, http_type_text);
        opt_sock_write(fd, http_headers);
        opt_sock_write(fd, HTTP_ENDL);  
        // argument indicates which session to restart
        if(arg) {
          session_restart(arg);
        }
        opt_http_write(fd, "FAIL");
        opt_http_write(fd, ""); 
        return 0; 
    }
    else if(strncmp(req, "/strategy", 9) == 0) {
        /* if these lines aren't there the browser
           keeps waiting until hserver is closed */
        opt_sock_write(fd, status_200);
        opt_sock_write(fd, http_type_text);
        opt_sock_write(fd, http_headers);
        opt_sock_write(fd, HTTP_ENDL); 
        
        /* look up specified session */
        if(arg) {
          for (i = 0; i < slist_cap; ++i) {
            if (slist[i].name && strcmp(slist[i].name, arg) == 0) {
              /* strategy name is set in session_open, off the 
                 initial session message */
              opt_http_write(fd, slist[i].strategy_name); 
              opt_http_write(fd, "");
              return 0;
            }
          }
        } else {
          opt_http_write(fd, "FAIL");
          opt_http_write(fd, "");
          return 0;
        }
    } else if(strncmp(req, "/converged", 10) == 0) {
        opt_sock_write(fd, status_200);
        opt_sock_write(fd, http_type_text);
        opt_sock_write(fd, http_headers);
        opt_sock_write(fd, HTTP_ENDL);  
        if(arg) {
          for (i = 0; i < slist_cap; ++i) {
            if (slist[i].name && strcmp(slist[i].name, arg) == 0) {
              /* convereged var is set in handle_session_socket
                 when the session indicates that it has 
                 converved */
              /*opt_http_write(fd, slist[i].converged?"1":"0"); 
              opt_http_write(fd, "");
              return 0;*/

              /* Create an empty message */
              hmesg_t converged_mesg;
              converged_mesg = HMESG_INITIALIZER;

              /* ask session whether strategy has converged */
              converged_mesg.type = HMESG_GETCFG;
              converged_mesg.data.string = CFGKEY_STRATEGY_CONVERGED;
              converged_mesg.status = HMESG_STATUS_REQ;

              mesg_send(slist[i].fd, &converged_mesg);
              mesg_recv(slist[i].fd, &converged_mesg);

              /* tell interface that session has converged if reply says so */
              if(converged_mesg.data.string && converged_mesg.data.string[0] == '1') {
                opt_http_write(fd, "1");
              } else opt_http_write(fd, "0");
              opt_http_write(fd, "");
              return 0;
            }
          }
        } else {
          opt_http_write(fd, "FAIL");
          opt_http_write(fd, "");
          return 0;
        }
    }

    /* If request is not handled by any special cases above,
       look for a known html file corresponding to the request. */
    for (i = 0; html_file[i].filename != NULL; ++i) {
        if (strcmp(req + 1, html_file[i].filename) == 0) {
            opt_sock_write(fd, status_200);
            switch (html_file[i].type) {
            case CONTENT_HTML:
                opt_sock_write(fd, http_type_html);
                break;
            case CONTENT_JAVASCRIPT:
                opt_sock_write(fd, http_type_js);
                break;
            case CONTENT_CSS:
                opt_sock_write(fd, http_type_css);
                break;
            }
            opt_sock_write(fd, http_headers);
            opt_sock_write(fd, HTTP_ENDL);

            http_chunk_send(fd, html_file[i].buf, html_file[i].buflen);
            opt_http_write(fd, "");
            return 0;
        }
    }

    return -1;
}

char *uri_decode(char *buf)
{
    char *head = buf;
    char *tail = buf;
    unsigned int conv;
    int count;

    while (*head != '\0') {
        if (*head == '%') {
            if (sscanf(head, "%%%x%n", &conv, &count) && conv < 0xFF) {
                head += count;
                *tail = (char)conv;
                ++tail;
            }
        }

        if (tail != head)
            *tail = *head;
        ++tail;
        ++head;
    }
    if (tail != head)
        *tail = '\0';

    return buf;
}

int http_chunk_send(int fd, const char *data, int datalen)
{
    int n;
    char buf[11];

    n = snprintf(buf, sizeof(buf), "%x" HTTP_ENDL, datalen);

    /* Relying on TCP buffering to keep this somewhat efficient. */
    socket_write(fd, buf, n);
    if (datalen > 0)
        n += socket_write(fd, data, datalen);
    return n + opt_sock_write(fd, HTTP_ENDL);
}

char *http_request_recv(int fd, char *buf, int buflen, char **data)
{
    const char delim[] = HTTP_ENDL HTTP_ENDL;
    char *retval, *split;
    int len, recvlen;

    if (!*data) {
        *data = buf;
        *buf = '\0';
    }

    while ( (split = strstr(*data, delim)) == NULL) {
        len = strlen(*data);

        if (*data == buf && len == (buflen-1)) {
            /* Buffer overflow.  Return truncated request. */
            break;
        }

        if (*data != buf) {
            if (len > 0) {
                /* Move existing data to the front of buffer. */
                if (len < *data - buf) {
                    strcpy(buf, *data);

                } else {
                    /* Memory region overlaps.  Can't use strcpy(). */
                    for (len = 0; (*data)[len] != '\0'; ++len)
                        buf[len] = (*data)[len];
                    buf[len] = '\0';
                }
            }
            *data = buf;
        }

        /* Read more data from socket into the remaining buffer space. */
        recvlen = recv(fd, buf + len, buflen - (len + 1), MSG_DONTWAIT);
        if (recvlen < 0) {
            return NULL;
        } else if (recvlen == 0) {
            if (len == 0) {
                /* No data in buffer, and connection was closed. */
                return NULL;
            }
            break;
        }

        len += recvlen;
        buf[len] = '\0';
    }

    retval = *data;
    if (split) {
        memset(split, '\0', strlen(delim));
        *data = split + strlen(delim);

    } else {
        /* Socket closed while buffer not empty.  Return remaining data */
        *data += len;
    }
    // /* DEBUG */ printf("HTTP REQ: %s", retval);
    return retval;
}

int http_overview_send(int fd)
{
    char *buf, *ptr;
    int i, j, buflen, count, total;

    sendbuf[0] = '\0';
    buf = sendbuf;
    buflen = sizeof(sendbuf);
    total = 0;

    for (i = 0; i < slist_cap; ++i) {
        if (!slist[i].name)
            continue;

        ptr = buf;
        count = snprintf_serial(&buf, &buflen, "%s:%ld%03ld:%d:%d:",
                                slist[i].name,
                                slist[i].start.tv_sec,
                                slist[i].start.tv_usec/1000,
                                slist[i].client_len,
                                slist[i].reported);
        if (count < 0)
            return -1;
        total += count;

        if (slist[i].best.id == -1) {
            count = snprintf_serial(&buf, &buflen, "&lt;unknown&gt;");
            if (count < 0)
                return -1;
            total += count;
        }
        else {
            for (j = 0; j < slist[i].best.n; ++j) {
                switch (slist[i].best.val[j].type) {
                case HVAL_INT:
                    count = snprintf_serial(&buf, &buflen, "%ld",
                                            slist[i].best.val[j].value.i);
                    if (count < 0)
                        return -1;
                    total += count;
                    break;
                case HVAL_REAL:
                    count = snprintf_serial(&buf, &buflen, "%lf",
                                            slist[i].best.val[j].value.r);
                    if (count < 0)
                        return -1;
                    total += count;
                    break;
                case HVAL_STR:
                    count = snprintf_serial(&buf, &buflen, "\"%s\"",
                                            slist[i].best.val[j].value.s);
                    if (count < 0)
                        return -1;
                    total += count;
                    break;
                default:
                    break;
                }

                if (j != slist[i].best.n - 1) {
                    count = snprintf_serial(&buf, &buflen, " ");
                    if (count < 0)
                        return -1;
                    total += count;
                }
            }

            count = snprintf_serial(&buf, &buflen, "|");
            if (count < 0)
                return -1;
            total += count;
        }

        if (total >= sizeof(sendbuf)) {
            /* Corner Case: No room for any session data.  Error out. */
            if (ptr == sendbuf) {
                opt_http_write(fd, "FAIL");
                return -1;
            }

            /* Make room in buffer by sending the contents thus far. */
            *ptr = '\0';
            opt_http_write(fd, sendbuf);

            /* Reset the loop variables and try again. */
            buf = sendbuf;
            buflen = sizeof(sendbuf);
            total = 0;
            --i;
        }
    }

    opt_http_write(fd, sendbuf);
    return 0;
}

int http_session_send(int fd, const char *req)
{
    char *ptr, *buf;
    int i, idx, buflen, count, total;
    struct timeval tv;
    session_state_t *sess;

    ptr = strchr(req, '&');
    if (ptr) {
        buflen = ptr - req;
        idx = atoi(++ptr);
    }
    else {
        buflen = strlen(req);
        idx = 0;
    }

    for (i = 0; i < slist_cap; ++i) {
        if (!slist[i].name)
            continue;

        if (strncmp(req, slist[i].name, buflen) == 0 &&
            slist[i].name[buflen] == '\0')
        {
            sess = &slist[i];
            break;
        }
    }
    if (i == slist_cap)
        goto error;

    if (gettimeofday(&tv, NULL) != 0)
        goto error;

    buf = http_session_header(sess, &tv);
    if (!buf)
        goto error;
    opt_http_write(fd, buf);

    sendbuf[0] = '\0';
    buf = sendbuf;
    buflen = sizeof(sendbuf);
    total = 0;

    for (i = idx; i < sess->log_len; ++i) {
        ptr = buf;

        count = snprintf_serial(&buf, &buflen, "|coord:");
        if (count < 0)
            goto error;
        total += count;

        count = report_append(&buf, &buflen, sess, &sess->log[i].stamp,
                              &sess->log[i].pt, sess->log[i].perf);
        if (count < 0)
            goto error;
        total += count;

        if (total >= sizeof(sendbuf)) {
            /* Corner Case: No room for any session data.  Error out. */
            if (ptr == sendbuf)
                goto error;

            /* Make room in buffer by sending the contents thus far. */
            *ptr = '\0';
            opt_http_write(fd, sendbuf);

            /* Reset the loop variables and try again. */
            buf = sendbuf;
            buflen = sizeof(sendbuf);
            total = 0;
            --i;
        }
    }

    printf("HTTP session send: %s\n", sendbuf);

    opt_http_write(fd, sendbuf);
    return 0;

  error:
    opt_http_write(fd, "FAIL");
    return -1;
}

char *http_session_header(session_state_t *sess, struct timeval *tv)
{
    int i, count, total;
    char *buf;
    int buflen;

    buf = sendbuf;
    buflen = sizeof(sendbuf);
    total = 0;

    count = snprintf_serial(&buf, &buflen, "time:%ld%03ld|app:%s|var:",
                            tv->tv_sec, tv->tv_usec/1000, sess->name);
    if (count < 0)
        return NULL;
    total += count;

    for (i = 0; i < sess->sig.range_len - 1; ++i) {
        count = snprintf_serial(&buf, &buflen, "g:%s,",
                                sess->sig.range[i].name);
        if (count < 0)
            return NULL;
        total += count;
    }
    count = snprintf_serial(&buf, &buflen, "g:%s", sess->sig.range[i].name);
    if (count < 0)
        return NULL;
    total += count;

    count = snprintf_serial(&buf, &buflen, "|coord:best");
    if (count < 0)
        return NULL;
    total += count;

    count = report_append(&buf, &buflen, sess, NULL,
                          &sess->best, sess->best_perf);
    if (count < 0)
        return NULL;
    total += count;

    return sendbuf;
}

int report_append(char **buf, int *buflen, session_state_t *sess,
                  struct timeval *tv, const hpoint_t *pt, const double perf)
{
    int i, count, total;

    total = 0;
    if (tv) {
        count = snprintf_serial(buf, buflen, "%ld%03ld,%ld%03ld",
                                tv->tv_sec, tv->tv_usec/1000,
                                tv->tv_sec, tv->tv_usec/1000);
        if (count < 0)
            return -1;
        total += count;
    }

    for (i = 0; i < pt->n; ++i) {
        if (pt->id == -1) {
            count = snprintf_serial(buf, buflen, ",?");
            if (count < 0)
                return -1;
            total += count;
        }
        else {
            count = snprintf_serial(buf, buflen, ",");
            if (count < 0)
                return -1;
            total += count;

            if (tv) {
                count = snprintf_serial(buf, buflen, "%s:",
                                        sess->sig.range[i].name);
                if (count < 0)
                    return -1;
                total += count;
            }

            switch (pt->val[i].type) {
            case HVAL_INT:
                count = snprintf_serial(buf, buflen, "%ld",
                                        pt->val[i].value.i);
                break;
            case HVAL_REAL:
                count = snprintf_serial(buf, buflen, "%lf",
                                        pt->val[i].value.r);
                break;
            case HVAL_STR:
                count = snprintf_serial(buf, buflen, "%s",
                                        pt->val[i].value.s);
                break;
            default:
                return -1;
            }
            if (count < 0)
                return -1;
            total += count;
        }
    }

    if (pt->id == -1) {
        count = snprintf_serial(buf, buflen, ",?");
    }
    else {
        count = snprintf_serial(buf, buflen, ",%lf", perf);
    }
    if (count < 0)
        return -1;
    total += count;

    return total;
}
