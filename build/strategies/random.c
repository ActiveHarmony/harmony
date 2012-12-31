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

#include "strategy.h"
#include "session-core.h"
#include "hsession.h"
#include "hutil.h"
#include "hcfg.h"
#include "defaults.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

hpoint_t best;
double best_perf;

/* Be sure all remaining definitions are declared static to avoid
 * possible namspace conflicts in the GOT due to PIC behavior.
 */
static hpoint_t curr;

static void curr_randomize(void);

/*
 * Invoked once on strategy load.
 */
int strategy_init(hmesg_t *mesg)
{
    const char *cfgstr;

    best = HPOINT_INITIALIZER;
    best_perf = INFINITY;

    cfgstr = hcfg_get(sess->cfg, CFGKEY_RANDOM_SEED);
    if (cfgstr)
        srand(atoi(cfgstr));

    if (hcfg_set(sess->cfg, CFGKEY_STRATEGY_CONVERGED, "0") < 0)
        return -1;

    if (hpoint_init(&curr, sess->sig.range_len) < 0) {
        mesg->data.string = "Could not allocate memory for point structure";
        return -1;
    }

    curr_randomize();
    return 0;
}

/*
 * Generate a new candidate configuration message in the space provided
 * by the hmesg_t parameter.
 */
int strategy_fetch(hmesg_t *mesg)
{
    mesg->data.fetch.cand = HPOINT_INITIALIZER;
    if (hpoint_copy(&mesg->data.fetch.cand, &curr) < 0)
        goto error;
    curr_randomize();

    if (mesg->data.fetch.best.id < best.id) {
        mesg->data.fetch.best = HPOINT_INITIALIZER;
        if (hpoint_copy(&mesg->data.fetch.best, &best) < 0)
            goto error;
    }
    else {
        mesg->data.fetch.best.id = -1;
    }

    mesg->status = HMESG_STATUS_OK;
    return 0;

  error:
    mesg->status = HMESG_STATUS_FAIL;
    mesg->data.string = strerror(errno);
    return -1;
}

/*
 * Inform the search strategy of an observed performance associated with
 * a configuration point.
 */
int strategy_report(hmesg_t *mesg)
{
    if (best_perf > mesg->data.report.perf) {
        best_perf = mesg->data.report.perf;
        if (hpoint_copy(&best, &mesg->data.report.cand) < 0) {
            mesg->status = HMESG_STATUS_FAIL;
            mesg->data.string = strerror(errno);
            return -1;
        }
    }

    mesg->status = HMESG_STATUS_OK;
    return 0;
}

void curr_randomize(void)
{
    int i;

    ++curr.id;
    for (i = 0; i < curr.idx_cap; ++i)
        curr.idx[i] = rand() % sess->sig.range[i].max_idx;
}
