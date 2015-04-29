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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <float.h>

hsession_t HSESSION_INITIALIZER = {{0}};

/*
 * Session API Implementation
 */
int hsession_copy(hsession_t *dst, const hsession_t *src)
{
    if (hsignature_copy(&dst->sig, &src->sig) != 0)
        return -1;
    if (hcfg_copy(&dst->cfg, &src->cfg) != 0)
        return -1;
    return 0;
}

void hsession_fini(hsession_t *sess)
{
    hsignature_fini(&sess->sig);
    hcfg_fini(&sess->cfg);
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

    count = hcfg_serialize(buf, buflen, &sess->cfg);
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

    count = hcfg_deserialize(&sess->cfg, buf + total);
    if (count < 0) goto error;
    total += count;

    return total;

  invalid:
    errno = EINVAL;
  error:
    return -1;
}
