/*
 * Copyright 2003-2015 Jeffrey K. Hollingsworth
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
#include "hutil.h"
#include "hcfg.h"
#include "hsockutil.h"

#include <unistd.h>
#include <string.h>

/*
 * Name used to identify this plugin layer.
 * All Harmony plugin layers must define this variable.
 */
const char harmony_layer_name[] = "httpinfo";

static char* buf = NULL;
static int buflen = 0;

int httpinfo_setcfg(const char* key, const char* val)
{
    if (strcmp(key, CFGKEY_CONVERGED) == 0 ||
        strcmp(key, CFGKEY_STRATEGY)  == 0 ||
        strcmp(key, CFGKEY_PAUSED)    == 0)
    {
        hmesg_t mesg = HMESG_INITIALIZER;

        if (snprintf_grow(&buf, &buflen, "%s=%s", key, val ? val : "") == -1) {
            session_error("Error allocating memory for http info string.");
            return -1;
        }

        mesg.origin = -1;
        mesg.type = HMESG_SETCFG;
        mesg.status = HMESG_STATUS_REQ;
        mesg.data.string = buf;

        if (mesg_send(STDIN_FILENO, &mesg) < 1) {
            session_error("Error sending http info message");
            return -1;
        }
        hmesg_fini(&mesg);
    }

    return 0;
}

int httpinfo_fini(void)
{
    free(buf);
    buflen = 0;
    return 0;
}
