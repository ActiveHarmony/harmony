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

#include "session-core.h"
#include "hsignature.h"
#include "hpoint.h"

/*
 * Name used to identify this plugin.  All Harmony plugins must define
 * this variable.
 */
const char harmony_layer_name[] = "<plugin>";

/*
 * Invoked once on module load.
 *
 * Param:
 *   sig - Details of the parameter space (dimensions, bounds, etc.).
 *
 * Upon error, this function should call session_error() with a
 * human-readable string explaining the problem and return -1.
 * Otherwise, returning 0 indicates success.
 */
int <plugin>_init(hsignature_t *sig)
{
    return 0;
}

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
int <plugin>_join(const char *id)
{
    return 0;
}

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
int <plugin>_query(const char *key)
{
    return 0;
}

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
int <plugin>_inform(const char *key, const char *val)
{
    return 0;
}

/*
 * Invoked after search driver generates a candidate point, but before
 * that point is returned to the client.
 *
 * Params:
 *   flow - Controls how the plug-in manager will process the point.
 *   pt   - New candidate point to be sent to a client.
 *
 * Upon error, this function should call session_error() with a
 * human-readable string explaining the problem and return -1.
 * Otherwise, this routine should return 0, and the contents of the
 * "flow" variable should be set appropriately.
 */
int <plugin>_generate(hflow_t *flow, hpoint_t *pt)
{
    flow->status = HFLOW_ACCEPT;
    return 0;
}

/*
 * Invoked after the client reports a performance value, but before it
 * is processed by the search strategy.
 *
 * Params:
 *   flow - Controls how the plug-in manager will process the point.
 *   pt   - Candidate point tested by the client.
 *   perf - Performance value associated with the point.
 *
 * Upon error, this function should call session_error() with a
 * human-readable string explaining the problem and return -1.
 * Otherwise, this routine should return 0, and the contents of the
 * "flow" variable should be set appropriately.
 */
int <plugin>_analyze(hflow_t *flow, hpoint_t *pt, double perf)
{
    flow->status = HFLOW_ACCEPT;
    return 0;
}

/*
 * Invoked after the tuning session completes.
 *
 * Upon error, this function should call session_error() with a
 * human-readable string explaining the problem and return -1.
 * Otherwise, returning 0 indicates success.
 */
int <plugin>_fini(void)
{
    return 0;
}
