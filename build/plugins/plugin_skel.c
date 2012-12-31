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

#include "session-core.h"
#include "hmesg.h"
#include "hpoint.h"
#include "hutil.h"
#include "hcfg.h"
#include "hsockutil.h"
#include "defaults.h"

/*
 * Name used to identify this plugin.  All Harmony plugins must define
 * this variable.
 */
const char harmony_plugin_name[] = "<plugin>";

/* Be sure all remaining definitions are declared static to avoid
 * possible namspace conflicts in the GOT due to PIC behavior.
 */
static char *buf;
static int buflen;

/*
 * Invoked once on module load.
 *
 * This routine should return 0 on success, and -1 otherwise.
 */
int <plugin>_init(hmesg_t *mesg)
{
    return 0;
}

/*
 * Called after the client API reports a performance value, but before
 * it is processed by the search driver.
 *
 * This routine should return 0 if it is completely done processing
 * the message.  Otherwise, -1 should be returned with mesg->status
 * (and possibly errno) set appropriately.
 */
int <plugin>_report(hmesg_t *mesg)
{
    return 0;
}

/*
 * Called after search driver generates a candidate point, but before
 * that point is returned to the client API.
 *
 * This routine should return 0 if it is completely done processing
 * the message.  Otherwise, -1 should be returned with mesg->status
 * (and possibly errno) set appropriately.
 */
int <plugin>_fetch(hmesg_t *mesg)
{
    return 0;
}

/*
 * Invoked after the tuning session completes.
 */
void <plugin>_fini(void)
{
}
