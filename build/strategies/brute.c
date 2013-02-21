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

#include "strategy.h"
#include "session-core.h"
#include "hsession.h"
#include "hutil.h"
#include "hcfg.h"
#include "defaults.h"
#include "libvertex.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

hpoint_t strategy_best;
double strategy_best_perf;

/* Forward function definitions. */
int strategy_cfg(hmesg_t *mesg);

/* Variables to track current search state. */
vertex_t *curr;
int remaining_passes;

/*
 * Invoked once on strategy load.
 */
int strategy_init(hmesg_t *mesg)
{
    if (libvertex_init(sess) != 0) {
        mesg->data.string = "Could not initialize vertex library.";
        return -1;
    }

    if (strategy_cfg(mesg) != 0)
        return -1;

    strategy_best = HPOINT_INITIALIZER;
    strategy_best_perf = INFINITY;

    curr = vertex_alloc();
    if (!curr) {
        mesg->data.string = "Could not allocate memory for testing vertex.";
        return -1;
    }
    curr->id = 1;
    vertex_min(curr);

    if (hcfg_set(sess->cfg, CFGKEY_STRATEGY_CONVERGED, "0") < 0) {
        mesg->data.string =
            "Could not set " CFGKEY_STRATEGY_CONVERGED " config variable.";
        return -1;
    }
    return 0;
}

int strategy_cfg(hmesg_t *mesg)
{
    const char *cfgstr;

    cfgstr = hcfg_get(sess->cfg, CFGKEY_BRUTE_PASSES);
    if (cfgstr)
        remaining_passes = atoi(cfgstr);

    return 0;
}

/*
 * Generate a new candidate configuration message in the space provided
 * by the hmesg_t parameter.
 */
int strategy_fetch(hmesg_t *mesg)
{
    if (curr->id > 0) {
        if (vertex_to_hpoint(curr, &mesg->data.fetch.cand) != 0)
            goto error;

        if (vertex_incr(curr)) {
            if (--remaining_passes < 0) {
                if (hcfg_set(sess->cfg, CFGKEY_STRATEGY_CONVERGED, "1") != 0)
                    return -1;

                /* Search has converged.  End the search. */
                curr->id = -1;
            }
        }
    }
    else {
        if (hpoint_copy(&mesg->data.fetch.cand, &strategy_best) < 0)
            goto error;
    }

    if (mesg->data.fetch.best.id < strategy_best.id) {
        mesg->data.fetch.best = HPOINT_INITIALIZER;
        if (hpoint_copy(&mesg->data.fetch.best, &strategy_best) < 0)
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
    if (strategy_best_perf > mesg->data.report.perf) {
        strategy_best_perf = mesg->data.report.perf;
        if (hpoint_copy(&strategy_best, &mesg->data.report.cand) < 0) {
            mesg->status = HMESG_STATUS_FAIL;
            mesg->data.string = strerror(errno);
            return -1;
        }
    }

    mesg->status = HMESG_STATUS_OK;
    return 0;
}
