/*
 * Copyright 2003-2011 Jeffrey K. Hollingsworth
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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

/***
 *
 * Exit and send a mesage to stderr
 *
 ***/
void h_exit(const char *errmesg)
{
    perror(errmesg);
    exit(1);
}

char *search_path(const char *filename, mode_t and_bits, mode_t or_bits)
{
    static char fullpath[FILENAME_MAX];
    struct stat sb;
    char *path, *pend;

    path = getenv("PATH");
    if (path == NULL)
        return NULL;

    while (path != NULL) {
        pend = strchr(path, ':');
        if (pend == NULL)
            pend = path + strlen(path);

        snprintf(fullpath, sizeof(fullpath), "%.*s/%s",
                 (int)(pend - path), path, filename);

        if (stat(fullpath, &sb) == 0)
            if ((sb.st_mode & and_bits) == and_bits &&
                (sb.st_mode & or_bits) != 0)
            {
                return fullpath;
            }

        if (*pend == '\0')
            path = NULL;
        else
            path = pend + 1;
    }
    return NULL;
}
