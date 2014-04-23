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

/**
 * \page logger Point Logger (log.so)
 *
 * This processing layer writes a log of point/performance pairs to disk as
 * they flow through the auto-tuning [feedback loop](\ref intro_feedback).
 *
 * **Configuration Variables**
 * Key          | Type   | Default | Description
 * ------------ | ------ | ------- | -----------
 * LOGFILE      | String | (none)  | Name of point/performance log file.
 * LOGFILE_MODE | String | a       | Mode to use with `fopen()`.  Valid values are "a" and "w" (without quotes).
 */

#include "session-core.h"
#include "hsignature.h"
#include "hpoint.h"
#include "hperf.h"
#include "defaults.h"

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>

/*
 * Name used to identify this plugin.  All Harmony plugins must define
 * this variable.
 */
const char harmony_layer_name[] = "logger";

FILE *fd;

int logger_init(hsignature_t *sig)
{
    const char *filename;
    const char *mode;
    time_t tm;

    filename = session_getcfg("LOGFILE");
    if (!filename) {
        session_error("LOGFILE config key empty");
        return -1;
    }

    mode = session_getcfg("LOGFILE_MODE");
    if (!mode)
        mode = "a";

    fd = fopen(filename, mode);
    if (!fd) {
        session_error( strerror(errno) );
        return -1;
    }

    tm = time(NULL);
    fprintf(fd, "*\n");
    fprintf(fd, "* Begin tuning session log.\n");
    fprintf(fd, "* Timestamp: %s", asctime( localtime(&tm) ));
    fprintf(fd, "*\n");

    return 0;
}

int logger_join(const char *id)
{
    fprintf(fd, "Client \"%s\" joined the tuning session.\n", id);
    return 0;
}

int logger_analyze(hflow_t *flow, htrial_t *trial)
{
    int i;
    const hpoint_t *pt = &trial->point;

    fprintf(fd, "Point #%d: (", pt->id);
    for (i = 0; i < pt->n; ++i) {
        if (i > 0) fprintf(fd, ",");

        switch (pt->val[i].type) {
        case HVAL_INT:  fprintf(fd, "%ld", pt->val[i].value.i); break;
        case HVAL_REAL: fprintf(fd, "%lf", pt->val[i].value.r); break;
        case HVAL_STR:  fprintf(fd, "%s",  pt->val[i].value.s); break;
        default:
            session_error("Invalid point value type");
            return -1;
        }
    }
    fprintf(fd, ") ");

    if (trial->perf->n > 1) {
        fprintf(fd, "= (");
        for (i = 0; i < trial->perf->n; ++i) {
            if (i > 0) fprintf(fd, ",");
            fprintf(fd, "%lf", trial->perf->p[i]);
        }
        fprintf(fd, ") ");
    }
    fprintf(fd, "=> %lf\n", hperf_unify(trial->perf));

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
