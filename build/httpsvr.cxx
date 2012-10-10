/*
 * Copyright 2003-2012 Jeffrey K. Hollingsworth
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
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <cerrno>
#include <string>
#include <limits.h>

#include "hserver.h"   /* for tcl_inter definition */
#include "hsockutil.h"

#define HTTP_ENDL "\r\n"
unsigned int http_connection_limit = 32;

enum content_t { CONTENT_HTML, CONTENT_JAVASCRIPT, CONTENT_CSS };
struct memfile_t {
    const char *filename;
    content_t type;
    char *buf;
    unsigned buflen;
};

static struct memfile_t html_file[] = {
    /* The default html file must be first in this array. */
    { "index.html",                   CONTENT_HTML,       NULL, 0 },
    { "jquery-1.6.2.min.js",          CONTENT_JAVASCRIPT, NULL, 0 },
    { "jquery.flot.min.js",           CONTENT_JAVASCRIPT, NULL, 0 },
    { "jquery.flot.resize.min.js",    CONTENT_JAVASCRIPT, NULL, 0 },
    { "jquery.flot.selection.min.js", CONTENT_JAVASCRIPT, NULL, 0 },
    { "excanvas.min.js",              CONTENT_JAVASCRIPT, NULL, 0 },
    { "activeharmonyUI.js",           CONTENT_JAVASCRIPT, NULL, 0 },
    { "activeharmonyUI.css",          CONTENT_CSS,        NULL, 0 },
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
    "Transfer-Encoding: chunked" HTTP_ENDL;

/* Internal helper function declarations */
int http_handle_request(int fd, char *req);
char *uri_decode(char *buf);
void http_send_chunk(int fd, const char *data, int datalen);
char *http_recv_request(int fd, char *buf, int buflen, char **data);

int http_init(const char *basedir)
{
    char path[FILENAME_MAX];
    int fd, i;
    struct stat sb;

    for (i = 0; html_file[i].filename != NULL; ++i) {
        if (html_file[i].buf != NULL) {
            munmap(html_file[i].buf, html_file[i].buflen);
        }

        snprintf(path, sizeof(path), "%s/%s", basedir, html_file[i].filename);
        fd = open(path, O_RDONLY);
        if (fd == -1) {
            printf("[AH]: Error opening HTTP server file %s\n",
                   html_file[i].filename);
            return -1;
        }

        /* Obtain file size */
        if (fstat(fd, &sb) == -1) {
            printf("[AH]: Error during fstat(): %s\n", strerror(errno));
            return -1;
        }
        html_file[i].buflen = sb.st_size;

        html_file[i].buf = (char *)mmap(NULL, html_file[i].buflen,
                                        PROT_READ, MAP_PRIVATE, fd, 0);
        if (html_file[i].buf == (char *)-1) {
            printf("[AH]: Error during mmap(): %s\n", strerror(errno));
            return -1;
        }

        if (close(fd) != 0) {
            printf("[AH]: Error during close(): %s. Ignoring.\n",
                   strerror(errno));
        }
    }
    return 0;
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

    socket_write(fd, status_line, strlen(status_line));
    socket_write(fd, http_type_html, strlen(http_type_html));
    socket_write(fd, http_headers, strlen(http_headers));
    socket_write(fd, HTTP_ENDL, strlen(HTTP_ENDL));
    http_send_chunk(fd, "<HTML><HEAD><TITLE>", 19);
    http_send_chunk(fd, status_line +  9, strlen(status_line +  9));
    http_send_chunk(fd, "</TITLE></HEAD><BODY><H1>", 25);
    http_send_chunk(fd, status_line + 13, strlen(status_line + 13));
    http_send_chunk(fd, "</H1>", 5);
    if (message || arg) {
        http_send_chunk(fd, "<P>",  3);
        if (message)
            http_send_chunk(fd, message, strlen(message));
        if (arg)
            http_send_chunk(fd, arg, strlen(arg));
        http_send_chunk(fd, "</P>", 4);
    }
    http_send_chunk(fd, "<HR>Active Harmony HTTP Server</BODY></HTML>", 44);
    http_send_chunk(fd, NULL, 0);
}

int handle_http_socket(int fd)
{
    char buf[4096];  /* This is not reentrant.  Is that a problem? */
    char *ptr = NULL, *req, *endp;

    errno = 0;
    req = http_recv_request(fd, buf, sizeof(buf), &ptr);
    while (req != NULL) {
        if (strncmp(req, "GET ", 4) == 0) {
            endp = strchr(req + 4, ' ');
            if (endp == NULL) {
                /* Bad Request */
                http_send_error(fd, 400, req);

            } else {
                *endp = '\0';
                req = uri_decode(req + 4);
                if (http_handle_request(fd, req) < 0) {
                    /* Not Found */
                    http_send_error(fd, 404, req);
                }
            }

        } else if (strncmp(req, "OPTIONS ", 8) == 0 ||
                   strncmp(req, "HEAD ", 5)    == 0 ||
                   strncmp(req, "POST ", 5)    == 0 ||
                   strncmp(req, "PUT ", 4)     == 0 ||
                   strncmp(req, "DELETE ", 7)  == 0 ||
                   strncmp(req, "TRACE ", 6)   == 0 ||
                   strncmp(req, "CONNECT ", 8) == 0) {

            /* Unimplemented Function */
            http_send_error(fd, 501, req);
        }

        req = http_recv_request(fd, buf, sizeof(buf), &ptr);
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
int http_handle_request(int fd, char *req)
{
    int i;
    char buf[4096]; /* This is not reentrant.  Is that a problem? */
    char *arg;

    arg = strchr(req, '?');
    if (arg) {
        *arg = '\0';
        ++arg;
    }

    if (strcmp(req, "/") == 0) {
        socket_write(fd, status_200, strlen(status_200));
        socket_write(fd, http_type_html, strlen(http_type_html));
        socket_write(fd, http_headers, strlen(http_headers));
        socket_write(fd, HTTP_ENDL, strlen(HTTP_ENDL));
        http_send_chunk(fd, html_file[0].buf, html_file[0].buflen);
        http_send_chunk(fd, NULL, 0);
        return 0;

    } else if (strcmp(req, "/data-refresh") == 0) {
        socket_write(fd, status_200, strlen(status_200));
        socket_write(fd, http_type_text, strlen(http_type_text));
        socket_write(fd, http_headers, strlen(http_headers));
        socket_write(fd, HTTP_ENDL, strlen(HTTP_ENDL));

        sprintf(buf, "time:%u000|", (unsigned int)time(NULL));
        http_send_chunk(fd, buf, strlen(buf));
        Tcl_Eval(tcl_inter, "http_status");
        if (*tcl_inter->result != '\0') {
            http_send_chunk(fd, tcl_inter->result,
                            strlen(tcl_inter->result));
        }

        unsigned int last = 0;
        if (arg) {
            sscanf(arg, "%u", &last);
        }
        std::string confs = history_since(last);
        if (confs.length() > 0) {
            http_send_chunk(fd, confs.c_str(), confs.length());
        }

        http_send_chunk(fd, NULL, 0);
        return 0;

    } else if (strcmp(req, "/hup") == 0) {
        http_init(harmonyBinDir);

        socket_write(fd, status_200, strlen(status_200));
        socket_write(fd, http_type_text, strlen(http_type_text));
        socket_write(fd, http_headers, strlen(http_headers));
        socket_write(fd, HTTP_ENDL, strlen(HTTP_ENDL));
        http_send_chunk(fd, "OK", 2);
        http_send_chunk(fd, NULL, 0);
        return 0;

    } else if (strcmp(req, "/var") == 0) {
        socket_write(fd, status_200, strlen(status_200));
        socket_write(fd, http_type_text, strlen(http_type_text));
        socket_write(fd, http_headers, strlen(http_headers));
        socket_write(fd, HTTP_ENDL, strlen(HTTP_ENDL));

        if (!arg) {
            sprintf(buf, "info globals");
            http_send_chunk(fd, "TCL Command: '", 14);
            http_send_chunk(fd, buf, strlen(buf));
            http_send_chunk(fd, "'\r\n'", 3);

            Tcl_Eval(tcl_inter, buf);
            sprintf(buf, "TCL Result: '%s'", tcl_inter->result);
            http_send_chunk(fd, buf, strlen(buf));
            http_send_chunk(fd, NULL, 0);

        } else {
            sprintf(buf, "var_to_string [info globals %s]", arg);
            http_send_chunk(fd, "TCL Command: '", 14);
            http_send_chunk(fd, buf, strlen(buf));
            http_send_chunk(fd, "'\r\n'", 3);

            Tcl_Eval(tcl_inter, buf);
            http_send_chunk(fd, "TCL Result:\r\n", 13);
            if (*tcl_inter->result != '\0') {
                http_send_chunk(fd, tcl_inter->result,
                                strlen(tcl_inter->result));
            }
            http_send_chunk(fd, NULL, 0);
        }
        return 0;
    }

    for (i = 0; html_file[i].filename != NULL; ++i) {
        if (strcmp(req + 1, html_file[i].filename) == 0) {
            socket_write(fd, status_200, strlen(status_200));
            switch (html_file[i].type) {
            case CONTENT_HTML:
                socket_write(fd, http_type_html, strlen(http_type_html));
                break;
            case CONTENT_JAVASCRIPT:
                socket_write(fd, http_type_js, strlen(http_type_js));
                break;
            case CONTENT_CSS:
                socket_write(fd, http_type_css, strlen(http_type_css));
                break;
            }
            socket_write(fd, http_headers, strlen(http_headers));
            socket_write(fd, HTTP_ENDL, strlen(HTTP_ENDL));

            http_send_chunk(fd, html_file[i].buf, html_file[i].buflen);
            http_send_chunk(fd, NULL, 0);
            return 0;
        }
    }

    /* I don't know how to handle this request. */
    return -1;
}

char *uri_decode(char *buf)
{
    char *head = buf;
    char *tail = buf;
    unsigned int conv, count;

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

void http_send_chunk(int fd, const char *data, int datalen)
{
    int n;
    char buf[10];

    n = snprintf(buf, sizeof(buf), "%x" HTTP_ENDL, datalen);

    /* Relying on TCP buffering to keep this somewhat efficient. */
    socket_write(fd, buf, n);
    if (datalen > 0)
        socket_write(fd, data, datalen);
    socket_write(fd, HTTP_ENDL, strlen(HTTP_ENDL));
}

char *http_recv_request(int fd, char *buf, int buflen, char **data)
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
