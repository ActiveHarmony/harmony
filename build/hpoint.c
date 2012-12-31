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

#include "hpoint.h"
#include "hutil.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

hpoint_t HPOINT_INITIALIZER = {-1};

int hpoint_init(hpoint_t *pt, int cap)
{
    pt->id = 0;
    pt->idx = (int *) calloc(cap, sizeof(int));
    if (!pt->idx)
        return -1;
    pt->idx_cap = cap;

    return 0;
}

void hpoint_fini(hpoint_t *pt)
{
    if (pt->idx_cap > 0) {
        free(pt->idx);
        pt->idx = NULL;
        pt->idx_cap = 0;
    }
}

int hpoint_copy(hpoint_t *copy, const hpoint_t *orig)
{
    int *newbuf;

    if (orig->id == -1) {
        hpoint_fini(copy);
        copy->id = -1;
        return 0;
    }

    if (copy->idx_cap != orig->idx_cap) {
        newbuf = (int *) realloc(copy->idx, sizeof(int) * orig->idx_cap);
        if (!newbuf)
            return -1;
        copy->idx = newbuf;
        copy->idx_cap = orig->idx_cap;
    }
    copy->id = orig->id;
    copy->step = orig->step;
    memcpy(copy->idx, orig->idx, sizeof(int) * orig->idx_cap);
    return 0;
}

int hpoint_serialize(char **buf, int *buflen, const hpoint_t *pt)
{
    int i, count, total;

    count = snprintf_serial(buf, buflen, "hpoint:%d %d ", pt->id, pt->step);
    if (count < 0) goto invalid;
    total = count;

    if (pt->id >= 0) {
        count = snprintf_serial(buf, buflen, "%d ", pt->idx_cap);
        if (count < 0) goto invalid;
        total += count;

        for (i = 0; i < pt->idx_cap; ++i) {
            count = snprintf_serial(buf, buflen, "%d ", pt->idx[i]);
            if (count < 0) goto error;
            total += count;
        }
    }
    return total;

  invalid:
    errno = EINVAL;
  error:
    return -1;
}

int hpoint_deserialize(hpoint_t *pt, char *buf)
{
    int i, count, total, newcap;
    int *newbuf;

    if (sscanf(buf, " hpoint:%d %d%n", &pt->id, &pt->step, &count) < 2)
        goto invalid;
    total = count;

    if (pt->id >= 0) {
        if (sscanf(buf + total, " %d%n", &newcap, &count) < 1)
            goto invalid;
        total += count;

        if (pt->idx_cap != newcap) {
            newbuf = (int *) realloc(pt->idx, sizeof(int) * newcap);
            if (!newbuf)
                goto error;
            pt->idx = newbuf;
            pt->idx_cap = newcap;
        }

        for (i = 0; i < pt->idx_cap; ++i) {
            if (sscanf(buf + total, " %d%n", &pt->idx[i], &count) < 1)
                goto invalid;
            total += count;
        }
    }
    return total;

  invalid:
    errno = EINVAL;
  error:
    return -1;
}
