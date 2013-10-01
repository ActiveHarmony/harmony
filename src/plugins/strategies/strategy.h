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

#include "session-core.h"
#include "hsignature.h"
#include "hpoint.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ===================================================================
 * The following functions are required.  Active Harmony will not
 * recognize shared objects as search strategies unless these
 * functions exist.
 */

/*
 * Generate a new candidate configuration point.
 *
 * Params:
 *   flow  - Inform plug-in manager of search state.
 *   point - New candidate point to be sent to a client.
 *
 * Upon error, this function should call session_error() with a
 * human-readable string explaining the problem and return -1.
 * Otherwise, this routine should return 0, and the contents of the
 * "flow" variable should be set appropriately.
 */
int strategy_generate(hflow_t *flow, hpoint_t *point);

/*
 * Regenerate a rejected candidate configuration point.
 *
 * Params:
 *   flow  - Inform plug-in manager of search state. Also may contain a search
 *           hint from the rejecting layer.
 *   point - Rejected candidate point to regenerate.
 *
 * Upon error, this function should call session_error() with a
 * human-readable string explaining the problem and return -1.
 * Otherwise, this routine should return 0, and the contents of the
 * "flow" variable should be set appropriately.
 */
int strategy_rejected(hflow_t *flow, hpoint_t *point);

/*
 * Analyze the observed performance associated with a configuration point.
 *
 * Params:
 *   trial - Observed point/performance pair.
 *
 * Upon error, this function should call session_error() with a
 * human-readable string explaining the problem and return -1.
 * Otherwise, returning 0 indicates success.
 */
int strategy_analyze(htrial_t *trial);

/*
 * Return the best performing configuration point thus far in the search.
 *
 * Params:
 *   point - Location to store best performing configuration point data.
 *
 * Upon error, this function should call session_error() with a
 * human-readable string explaining the problem and return -1.
 * Otherwise, returning 0 indicates success.
 */
int strategy_best(hpoint_t *point);

/* ===================================================================
 * The following functions are optional.  They will be invoked at the
 * appropriate time if and only if they exist.
 */

/*
 * Invoked once on strategy load.
 *
 * Param:
 *   sig - Details of the parameter space (dimensions, bounds, etc.).
 *
 * Upon error, this function should call session_error() with a
 * human-readable string explaining the problem and return -1.
 * Otherwise, returning 0 indicates success.
 */
int strategy_init(hsignature_t *sig);

/*
 * Invoked when a client joins the tuning session.
 *
 * Params:
 *   id - Uniquely identifying string for the new client.
 *
 * Upon error, this function should call session_error() with a
 * human-readable string explaining the problem and return -1.
 * Otherwise, returning 0 indicates success.
 */
int strategy_join(const char *id);

/*
 * Invoked when a client reads from the configuration system.
 *
 * Params:
 *   key - Configuration key requested.
 *
 * Upon error, this function should call session_error() with a
 * human-readable string explaining the problem and return -1.
 * Otherwise, returning 0 indicates success.
 */
int strategy_getcfg(const char *key);

/*
 * Invoked when a client writes to the configuration system.
 *
 * Params:
 *   key - Configuration key to be modified.
 *   val - New value for configuration key.
 *
 * Upon error, this function should call session_error() with a
 * human-readable string explaining the problem and return -1.
 * Otherwise, returning 0 indicates success.
 */
int strategy_setcfg(const char *key, const char *val);

/*
 * Invoked on session exit.
 *
 * Upon error, this function should call session_error() with a
 * human-readable string explaining the problem and return -1.
 * Otherwise, returning 0 indicates success.
 */
int strategy_fini(void);

#ifdef __cplusplus
}
#endif

#endif
