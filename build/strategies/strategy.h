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

#ifndef __SEARCH_STRATEGY_H__
#define __SEARCH_STRATEGY_H__

#include "hmesg.h"
#include "hpoint.h"

#ifdef __cplusplus
extern "C" {
#endif

extern hpoint_t best;
extern double best_perf;

/*
 * Invoked once on strategy load.
 */
int strategy_init(hmesg_t *mesg);

/*
 * Generate a new candidate configuration message the space provided
 * by the hmesg_t parameter.
 */
int strategy_fetch(hmesg_t *mesg);

/*
 * Inform the search strategy of an observed performance associated with
 * a configuration point.
 */
int strategy_report(hmesg_t *mesg);

/*
 * Note that search strategies do not support a fini function (yet).
 */

#ifdef __cplusplus
}
#endif

#endif
