/*
 * Copyright 2003-2015 Jeffrey K. Hollingsworth
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

#include "hutil.h"

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>

int file_exists(const char* filename)
{
    struct stat sb;
    return (stat(filename, &sb) == 0 && S_ISREG(sb.st_mode));
}

void* file_map(const char* filename, size_t* size)
{
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Error on open(%s): %s\n", filename, strerror(errno));
        return NULL;
    }

    /* Obtain file size */
    struct stat sb;
    void* retval = NULL;
    if (fstat(fd, &sb) != 0) {
        fprintf(stderr, "Error on fstat(%s): %s\n", filename, strerror(errno));
    }
    else {
        retval = mmap(NULL, sb.st_size + 1, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE, fd, 0);
        if (retval == MAP_FAILED) {
            fprintf(stderr, "Error on mmap(%s): %s\n",
                    filename, strerror(errno));
        }
    }

    if (close(fd) != 0) {
        fprintf(stderr, "Warning: Ignoring error on close(%s): %s\n",
                filename, strerror(errno));
    }

    *size = sb.st_size + 1;
    return retval;
}

void file_unmap(void* buf, size_t size)
{
    if (munmap(buf, size) != 0)
        fprintf(stderr, "Ignoring error on munmap(): %s\n", strerror(errno));
}

int array_grow(void* buf, int* capacity, int elem_size)
{
    void* new_buf;
    int new_capacity = 8;

    if (*capacity >= new_capacity)
        new_capacity = *capacity << 1;

    new_buf = realloc(*(void**)buf, new_capacity * elem_size);
    if (new_buf == NULL)
        return -1;

    memset(((char*)new_buf) + (*capacity * elem_size), 0,
           (new_capacity - *capacity) * elem_size);
    *(void**)buf = new_buf;
    *capacity = new_capacity;
    return 0;
}

char* search_path(const char* filename)
{
    /* XXX - Not re-entrant. */
    static char* fullpath = NULL;
    static int pathlen = 0;

    char* path;
    char* pend;
    char* newbuf;
    int count;

    path = getenv("PATH");
    if (path == NULL)
        return NULL;

    pend = path;
    while (*pend != '\0') {
        pend = strchr(path, ':');
        if (!pend)
            pend = path + strlen(path);

      retry:
        count = snprintf(fullpath, pathlen, "%.*s/%s",
                         (int)(pend - path), path, filename);
        if (pathlen <= count) {
            newbuf = realloc(fullpath, count + 1);
            if (!newbuf) return NULL;

            fullpath = newbuf;
            pathlen = count + 1;
            goto retry;
        }

        if (file_exists(fullpath))
            return fullpath;

        path = pend + 1;
    }
    return NULL;
}

char* stralloc(const char* in)
{
    char* out;
    if (!in)
        return NULL;

    out = malloc(sizeof(char) * (strlen(in) + 1));
    if (out != NULL)
        strcpy(out, in);

    return out;
}

char* sprintf_alloc(const char* fmt, ...)
{
    va_list ap;
    int count;
    char* retval;

    va_start(ap, fmt);
    count = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    if (count < 0) return NULL;
    retval = malloc(count + 1);
    if (!retval) return NULL;

    va_start(ap, fmt);
    vsnprintf(retval, count + 1, fmt, ap);
    va_end(ap);

    return retval;
}

int snprintf_grow(char** buf, int* buflen, const char* fmt, ...)
{
    va_list ap;
    int count;
    char* newbuf;

  retry:
    va_start(ap, fmt);
    count = vsnprintf(*buf, *buflen, fmt, ap);
    va_end(ap);

    if (count < 0)
        return -1;

    if (*buflen <= count) {
        newbuf = realloc(*buf, count + 1);
        if (!newbuf)
            return -1;
        *buf = newbuf;
        *buflen = count + 1;
        goto retry;
    }

    return count;
}

int snprintf_serial(char** buf, int* buflen, const char* fmt, ...)
{
    va_list ap;
    int count;

    va_start(ap, fmt);
    count = vsnprintf(*buf, *buflen, fmt, ap);
    va_end(ap);

    if (count < 0)
        return -1;

    *buflen -= count;
    if (*buflen < 0)
        *buflen = 0;
    else
        *buf += count;  /* Only advance *buf if the write was successful. */

    return count;
}

int printstr_serial(char** buf, int* buflen, const char* str)
{
    if (!str) return snprintf_serial(buf, buflen, "0\"0\" ");
    return snprintf_serial(buf, buflen, "%u\"%s\" ", strlen(str), str);
}

int scanstr_serial(const char** str, char* buf)
{
    int count;
    unsigned int len;

    if (sscanf(buf, "%u\"%n", &len, &count) < 1)
        goto invalid;
    buf += count;

    if (len == 0 && *buf == '0') {
        len = 1;
        *str = NULL;
    }
    else if (len < strlen(buf) && buf[len] == '\"') {
        buf[len] = '\0';
        *str = buf;
    }
    else goto invalid;

    return count + len + 1;

  invalid:
    errno = EINVAL;
    return -1;
}
