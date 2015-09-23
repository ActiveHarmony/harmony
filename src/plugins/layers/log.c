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

/**
 * \page logger Point Logger (log.so)
 *
 * This processing layer writes a log of point/performance pairs to disk as
 * they flow through the auto-tuning [feedback loop](\ref intro_feedback).
 */

#include "session-core.h"
#include "hspace.h"
#include "hpoint.h"
#include "hperf.h"
#include "hcfg.h"

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>

/*
 * Name used to identify this plugin layer.
 * All Harmony plugin layers must define this variable.
 */
const char harmony_layer_name[] = "logger";

/*
 * Configuration variables used in this plugin.
 * These will automatically be registered by session-core upon load.
 */
hcfg_info_t plugin_keyinfo[] = {
    { CFGKEY_LOG_FILE, NULL,
      "Name of point/performance log file." },
    { CFGKEY_LOG_MODE, "a",
      "Mode to use with 'fopen()'.  Valid values are a for append, "
      "and w for overwrite." },
    { NULL }
};

FILE* fd;

int logger_init(hspace_t* space)
{
    const char* filename = hcfg_get(session_cfg, CFGKEY_LOG_FILE);
    const char* mode     = hcfg_get(session_cfg, CFGKEY_LOG_MODE);
    time_t tm;

    if (!filename) {
        session_error(CFGKEY_LOG_FILE " config key empty.");
        return -1;
    }

    fd = fopen(filename, mode);
    if (!fd) {
        session_error( strerror(errno) );
        return -1;
    }

    tm = time(NULL);
    fprintf(fd, "* Begin tuning session log.\n");
    fprintf(fd, "* Timestamp: %s", asctime( localtime(&tm) ));

    return 0;
}

int logger_join(const char* id)
{
    fprintf(fd, "Client \"%s\" joined the tuning session.\n", id);
    return 0;
}

int logger_analyze(hflow_t* flow, htrial_t* trial)
{
    int i;

    fprintf(fd, "Point #%d: (", trial->point.id);
    for (i = 0; i < trial->point.len; ++i) {
        hval_t* v = &trial->point.val[i];
        if (i > 0) fprintf(fd, ",");

        switch (v->type) {
        case HVAL_INT:  fprintf(fd, "%ld", v->value.i); break;
        case HVAL_REAL: fprintf(fd, "%lf[%la]", v->value.r, v->value.r); break;
        case HVAL_STR:  fprintf(fd, "\"%s\"", v->value.s); break;
        default:
            session_error("Invalid point value type");
            return -1;
        }
    }
    fprintf(fd, ") ");

    if (trial->perf.len > 1) {
        fprintf(fd, "=> (");
        for (i = 0; i < trial->perf.len; ++i) {
            if (i > 0) fprintf(fd, ",");
            fprintf(fd, "%lf[%la]", trial->perf.obj[i], trial->perf.obj[i]);
        }
        fprintf(fd, ") ");
    }
    fprintf(fd, "=> %lf\n", hperf_unify(&trial->perf));
    fflush(fd);

    flow->status = HFLOW_ACCEPT;
    return 0;
}

int logger_fini(void)
{
    fprintf(fd, "*\n");
    fprintf(fd, "* End tuning session.\n");
    fprintf(fd, "*\n");

    if (fclose(fd) != 0) {
        session_error( strerror(errno) );
        return -1;
    }

    return 0;
}
