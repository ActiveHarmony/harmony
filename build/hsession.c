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

#include "hsession.h"
#include "hcfg.h"
#include "hmesg.h"
#include "hutil.h"
#include "hsockutil.h"
#include "defaults.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <float.h>

/*
 * Session API Implementation
 */
int hsession_init(hsession_t *sess)
{
    sess->sig = HSIGNATURE_INITIALIZER;
    sess->cfg = hcfg_alloc();
    sess->sig.memlevel = 1;
    if (sess->cfg == NULL)
        return -1;

    return 0;
}

int hsession_copy(hsession_t *dst, const hsession_t *src)
{
    if (hsignature_copy(&dst->sig, &src->sig) < 0)
        return -1;

    if (dst->cfg)
        hcfg_free(dst->cfg);
    dst->cfg = hcfg_copy(src->cfg);
    if (!dst->cfg)
        return -1;

    return 0;
}

void hsession_fini(hsession_t *sess)
{
    hsignature_fini(&sess->sig);
    hcfg_free(sess->cfg);
}

int hsession_int(hsession_t *sess, const char *name,
                 long min, long max, long step)
{
    hrange_t *range;

    if (max < min) {
        errno = EINVAL;
        return -1;
    }

    range = hrange_add(&sess->sig, name);
    if (!range)
        return -1;
 
    range->type = HVAL_INT;
    range->bounds.i.min = min;
    range->bounds.i.max = max;
    range->bounds.i.step = step;
    range->max_idx = ((max - min) / step) + 1;

    if (sess->sig.memlevel < 1)
        sess->sig.memlevel = 1;

    return 0;
}

int hsession_real(hsession_t *sess, const char *name,
                  double min, double max, double step)
{
    hrange_t *range;

    if (max < min) {
        errno = EINVAL;
        return -1;
    }

    range = hrange_add(&sess->sig, name);
    if (!range)
        return -1;

    range->type = HVAL_REAL;
    range->bounds.r.min = min;
    range->bounds.r.max = max;
    range->bounds.r.step = step;

    /* Inflate the step calculation (max-min)/step by machine
     * epsilon/2 to make up for floating-point inaccuracy, without
     * affecting the index calculation.
     */
    range->max_idx = ((max - min) / step) + (DBL_EPSILON / 2) + 1;

    if (sess->sig.memlevel < 1)
        sess->sig.memlevel = 1;

    return 0;
}

int hsession_enum(hsession_t *sess, const char *name, const char *value)
{
    int i;
    hrange_t *range;

    range = hrange_find(&sess->sig, name);
    if (range && range->type != HVAL_STR) {
        errno = EINVAL;
        return -1;
    }

    if (!range) {
        range = hrange_add(&sess->sig, name);
        if (!range)
            return -1;

        range->type = HVAL_STR;
    }

    for (i = 0; i < range->bounds.e.set_len; ++i) {
        if (strcmp(value, range->bounds.e.set[i]) == 0) {
            errno = EINVAL;
            return -1;
        }
    }

    if (range->bounds.e.set_len == range->bounds.e.set_cap) {
        if (array_grow(&range->bounds.e.set, &range->bounds.e.set_cap,
                       sizeof(char *)) < 0)
        {
            return -1;
        }
    }

    range->bounds.e.set[i] = stralloc(value);
    if (!range->bounds.e.set[i])
        return -1;

    ++range->bounds.e.set_len;
    range->max_idx = range->bounds.e.set_len;

    if (sess->sig.memlevel < 2)
        sess->sig.memlevel = 2;  /* Free string memory with this session. */

    return 0;
}

int hsession_name(hsession_t *sess, const char *name)
{
    sess->sig.name = stralloc(name);
    if (!sess->sig.name)
        return -1;
    sess->sig.memlevel = 2;
    return 0;
}

int hsession_strategy(hsession_t *sess, const char *strategy)
{
    if (hcfg_set(sess->cfg, CFGKEY_SESSION_STRATEGY, strategy) < 0)
        return -1;
    return 0;
}

int hsession_plugin_list(hsession_t *sess, const char *list)
{
    if (hcfg_set(sess->cfg, CFGKEY_SESSION_PLUGINS, list) < 0)
        return -1;
    return 0;
}

int hsession_cfg(hsession_t *sess, const char *key, const char *val)
{
    return hcfg_set(sess->cfg, key, val);
}

const char *hsession_launch(hsession_t *sess, const char *host, int port)
{
    static hmesg_t mesg;
    const char *retval;
    int socket;

    /* Sanity check input */
    if (!sess->sig.name || sess->sig.range_len < 1)
        return strerror(EINVAL);

    socket = tcp_connect(host, port);
    if (socket < 0)
        return strerror(errno);

    hmesg_scrub(&mesg);
    mesg.type = HMESG_SESSION;
    mesg.status = HMESG_STATUS_REQ;
    hsession_init(&mesg.data.session);
    hsession_copy(&mesg.data.session, sess);

    /* Send session information to server. */
    retval = NULL;
    if (mesg_send(socket, &mesg) < 1 ||
        mesg_recv(socket, &mesg) < 1 ||
        mesg.status != HMESG_STATUS_OK)
    {
        retval = mesg.data.string;
    }

    close(socket);
    return retval;
}

int hsession_serialize(char **buf, int *buflen, const hsession_t *sess)
{
    int count, total;

    count = snprintf_serial(buf, buflen, "session: ");
    if (count < 0) goto invalid;
    total = count;

    count = hsignature_serialize(buf, buflen, &sess->sig);
    if (count < 0) goto error;
    total += count;

    count = hcfg_serialize(buf, buflen, sess->cfg);
    if (count < 0) goto error;
    total += count;

    return total;

  invalid:
    errno = EINVAL;
  error:
    return -1;
}

int hsession_deserialize(hsession_t *sess, char *buf)
{
    int i, count, total;

    for (i = 0; isspace(buf[i]); ++i);
    if (strncmp("session:", buf + i, 8) != 0)
        goto invalid;
    total = i + 8;

    count = hsignature_deserialize(&sess->sig, buf + total);
    if (count < 0) goto error;
    total += count;

    count = hcfg_deserialize(sess->cfg, buf + total);
    if (count < 0) goto error;
    total += count;

    sess->sig.memlevel = 1;
    return total;

  invalid:
    errno = EINVAL;
  error:
    return -1;
}
