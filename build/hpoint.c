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

#include "hpoint.h"
#include "hutil.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

hpoint_t HPOINT_INITIALIZER = {-1, 0, NULL, 0, 0};

void hpoint_scrub(hpoint_t *pt);

int hpoint_init(hpoint_t *pt, int n)
{
    pt->id = 0;
    pt->n = n;
    pt->val = (hval_t *) calloc(n, sizeof(hval_t));
    if (!pt->val)
        return -1;
    pt->memlevel = 0;
    pt->step = -1;

    return 0;
}

void hpoint_fini(hpoint_t *pt)
{
    if (pt->n > 0) {
        hpoint_scrub(pt);
        free(pt->val);
    }
    *pt = HPOINT_INITIALIZER;
}

int hpoint_copy(hpoint_t *copy, const hpoint_t *orig)
{
    int i;
    hval_t *newbuf;
    char *newstr;

    if (orig->id == -1) {
        hpoint_fini(copy);
        return 0;
    }

    copy->id = orig->id;
    hpoint_scrub(copy);
    if (copy->n != orig->n) {
        newbuf = (hval_t *) realloc(copy->val, sizeof(hval_t) * orig->n);
        if (!newbuf)
            return -1;
        copy->val = newbuf;
        copy->n = orig->n;
    }

    copy->memlevel = 0;
    memcpy(copy->val, orig->val, sizeof(hval_t) * orig->n);
    for (i = 0; i < copy->n; ++i) {
        if (copy->val[i].type == HVAL_STR) {
            newstr = stralloc(copy->val[i].value.s);
            if (!newstr)
                return -1;
            copy->val[i].value.s = newstr;
            copy->memlevel = 1;
        }
    }

    copy->step = orig->step;
    return 0;
}

int hpoint_serialize(char **buf, int *buflen, const hpoint_t *pt)
{
    int i, count, total;

    count = snprintf_serial(buf, buflen, "hpoint:%d %d ", pt->id, pt->step);
    if (count < 0) goto invalid;
    total = count;

    if (pt->id >= 0) {
        count = snprintf_serial(buf, buflen, "%d ", pt->n);
        if (count < 0) goto invalid;
        total += count;

        for (i = 0; i < pt->n; ++i) {
            count = hval_serialize(buf, buflen, &pt->val[i]);
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
    int i, count, total, new_n;
    hval_t *newbuf;

    if (sscanf(buf, " hpoint:%d %d%n", &pt->id, &pt->step, &count) < 2)
        goto invalid;
    total = count;

    if (pt->id >= 0) {
        if (sscanf(buf + total, " %d%n", &new_n, &count) < 1)
            goto invalid;
        total += count;

        if (pt->n != new_n) {
            newbuf = (hval_t *) realloc(pt->val, sizeof(hval_t) * new_n);
            if (!newbuf)
                goto error;
            pt->val = newbuf;
            pt->n = new_n;
        }

        for (i = 0; i < pt->n; ++i) {
            count = hval_deserialize(&pt->val[i], buf + total);
            if (count < 0) goto error;
            total += count;
        }
    }

    pt->memlevel = 0;
    return total;

  invalid:
    errno = EINVAL;
  error:
    return -1;
}

void hpoint_scrub(hpoint_t *pt)
{
    int i;

    if (pt->memlevel == 1) {
        for (i = 0; i < pt->n; ++i) {
            if (pt->val[i].type == HVAL_STR)
                free((char *)pt->val[i].value.s);
        }
        pt->memlevel = 0;
    }
}
