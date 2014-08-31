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
 * \page omega Omega Constraint (constraint.so)
 *
 * Active Harmony allows for basic bounds on tuning variables during
 * session specification, where each tuning variable is bounded
 * individually.  However, some target applications require tuning
 * variables that are dependent upon one another, reducing the number
 * of valid parameters from the strict Cartesian product.
 *
 * This processing layer allows for the specification of such variable
 * dependencies through algebraic statements.  For example, consider a
 * target application with tuning variables `x` and `y`.  If `x` may
 * never be greater than `y`, one could use the following statement:
 *
 *     x < y
 *
 * Also, if the sum of `x` and `y` must remain under a certain
 * constant, one could use the following statement:
 *
 *     x + y = 10
 *
 * If multiple constraint statements are specified, the logical
 * conjunction of the set is applied to the
 * [Search Space](\ref intro_space).
 *
 * \note This processing layer requires the Omega Calculator, which is
 * available at:<br>
 * <https://github.com/davewathaverford/the-omega-project/>.
 *
 * **Configuration Variables**
 * Key             | Type      | Default | Description
 * --------------- | --------- | ------- | -----------
 * OC_BIN          | Filename  | oc      | Location of the Omega Calculator binary.  The `PATH` environment variable will be searched if not found initially.
 * CONSTRAINTS     | String    | [none]  | Constraint statements to be used during this session.  This variable has precedence over `CONSTRAINT_FILE`.
 * CONSTRAINT_FILE | Filename  | [none]  | If the `CONSTRAINTS` variable is not specified, they will be loaded from this file.
 * QUIET           | Boolean   | 0       | Bounds suggestion and rejection messages can be suppressed by setting this variable to 1.
 *
 * \note Some search strategies provide a `REJECT_METHOD`
 * configuration variable that can be used to specify how to deal with
 * rejected points.  This can have great affect on the productivity of
 * a tuning session.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>

#include "session-core.h"
#include "hsignature.h"
#include "hpoint.h"
#include "hutil.h"
#include "hsockutil.h"
#include "defaults.h"

#define MAX_CMD_LEN  4096
#define MAX_TEXT_LEN 1024
#define REG_TEXT_LEN 128
#define SHORT_TEXT_LEN 32

const char harmony_layer_name[] = "constraint";

/* Configuration Keys Used. */
#define CFGKEY_OC_BIN          "OC_BIN"
#define CFGKEY_CONSTRAINTS     "CONSTRAINTS"
#define CFGKEY_CONSTRAINT_FILE "CONSTRAINT_FILE"
#define CFGKEY_QUIET           "QUIET"

/* Forward function declarations. */
int strategy_cfg(hsignature_t *sig);
int build_vars_text(hsignature_t *sig);
int build_bounds_text(hsignature_t *sig);
int build_user_text(void);
int build_point_text(hpoint_t *point);
int update_bounds(hsignature_t *sig);
int check_validity(hpoint_t *point);
char *call_omega_calc(const char *cmd);

/* Keep copy of session information */
hsignature_t local_sig;

const char *omega_bin   = "oc";
char constraints[MAX_TEXT_LEN];

char vars_text[MAX_TEXT_LEN];
char bounds_text[MAX_TEXT_LEN];
char user_text[MAX_TEXT_LEN];
char point_text[MAX_TEXT_LEN];

/* Some global variables */
int quiet;

int constraint_init(hsignature_t *sig)
{
    /* Make a copy of the signature. */
    hsignature_copy(&local_sig, sig);

    /* Initialize layer variables. */
    if (strategy_cfg(sig) != 0)
        return -1;

    if (build_vars_text(sig) != 0)
        return -1;

    if (build_bounds_text(sig) != 0)
        return -1;

    if (build_user_text() != 0)
        return -1;

    /* Calculate the range for each tuning variable, given the constraints. */
    if (update_bounds(sig) != 0)
        return -1;

    return 0;
}

int strategy_cfg(hsignature_t *sig)
{
    const char *cfgval;

    cfgval = session_getcfg(CFGKEY_OC_BIN);
    if (cfgval)
        omega_bin = cfgval;

    if (!file_exists(omega_bin)) {
        omega_bin = search_path(omega_bin);
        if (!omega_bin) {
            session_error("Could not find Omega Calculator executable."
                          "  Use " CFGKEY_OC_BIN " to specify its location.");
            return -1;
        }
    }

    cfgval = session_getcfg(CFGKEY_QUIET);
    if (cfgval) {
        quiet = (*cfgval == '1' ||
                 *cfgval == 't' || *cfgval == 'T' ||
                 *cfgval == 'y' || *cfgval == 'Y');
    }

    cfgval = session_getcfg(CFGKEY_CONSTRAINTS);
    if (cfgval) {
        if (strlen(cfgval) >= sizeof(constraints)) {
            session_error("Constraint string too long");
            return -1;
        }
        strncpy(constraints, cfgval, sizeof(constraints));
    }
    else {
        FILE *fp;

        cfgval = session_getcfg(CFGKEY_CONSTRAINT_FILE);
        if (!cfgval) {
            session_error("No constraints specified.  Either "
                          CFGKEY_CONSTRAINTS " or " CFGKEY_CONSTRAINT_FILE
                          " must be defined.");
            return -1;
        }

        fp = fopen(cfgval, "r");
        if (!fp) {
            session_error("Could not open constraint file");
            return -1;
        }

        fread(constraints, sizeof(char), sizeof(constraints) - 1, fp);
        constraints[sizeof(constraints) - 1] = '\0';

        if (!feof(fp)) {
            session_error("Constraint file too large");
            return -1;
        }

        if (fclose(fp) != 0) {
            session_error("Could not close constraint file");
            return -1;
        }
    }
    return 0;
}

int constraint_generate(hflow_t *flow, hpoint_t *point)
{
    int i;

    flow->status = HFLOW_ACCEPT;
    if (!check_validity(point)) {
        flow->status = HFLOW_REJECT;
        flow->point.id = -1;

        if (!quiet) {
            fprintf(stderr, "Rejecting point: {");
            for (i = 0; i < point->n; ++i) {
                hval_t *val = &point->val[i];

                switch (val->type) {
                case HVAL_INT:  fprintf(stderr, "%ld", val->value.i); break;
                case HVAL_REAL: fprintf(stderr, "%lf", val->value.r); break;
                case HVAL_STR:  fprintf(stderr, "%s",  val->value.s); break;
                default:        fprintf(stderr, "<INV>");
                }

                if (i < point->n - 1)
                    fprintf(stderr, ", ");
            }
            fprintf(stderr, "}\n");
        }
    }
    return 0;
}

int constraint_fini(void)
{
    hsignature_fini(&local_sig);
    return 0;
}

int build_vars_text(hsignature_t *sig)
{
    int i;

    vars_text[0] = '\0';
    for (i = 0; i < sig->range_len; ++i) {
        strcat(vars_text, sig->range[i].name);
        if (i < sig->range_len - 1)
            strcat(vars_text, ", ");
    }
    return 0;
}

int build_bounds_text(hsignature_t *sig)
{
    int i;
    char *ptr = bounds_text;
    char *end = bounds_text + sizeof(bounds_text);

    bounds_text[0] = '\0';
    for (i = 0; i < sig->range_len; ++i) {
        hrange_t *range = &sig->range[i];

        /* Fetch min and max according to variable type */
        switch (range->type) {
        case HVAL_INT:
            ptr += snprintf(ptr, end - ptr, "%ld <= %s <= %ld",
                            range->bounds.i.min, range->name,
                            range->bounds.i.max);
            break;

        case HVAL_REAL:
            ptr += snprintf(ptr, end - ptr, "%lf <= %s <= %lf",
                            range->bounds.r.min, range->name,
                            range->bounds.r.max);
            break;

        case HVAL_STR:
        default:
            return -1;
        }

        if (i < sig->range_len - 1)
            ptr += snprintf(ptr, end - ptr, " && ");
    }
    return 0;
}

int build_user_text(void)
{
    const char *ptr = constraints;
    const char *end;
    int len = 0;

    while (*ptr) {
        while (isspace(*ptr)) ++ptr;
        end = ptr + strcspn(ptr, "\n");

        len += end - ptr;
        if (len < sizeof(user_text))
            strncat(user_text, ptr, end - ptr);

        while (isspace(*end)) ++end;
        if (*end) {
            len += 4;
            if (len < sizeof(user_text))
                strcat(user_text, " && ");
        }

        if (len >= sizeof(user_text)) {
            session_error("User constraint string overflow");
            return -1;
        }
        ptr = end;
    }
    return 0;
}

int build_point_text(hpoint_t *point)
{
    int i;
    char *ptr = point_text;
    char *end = point_text + sizeof(point_text);

    point_text[0] = '\0';
    for (i = 0; i < point->n; ++i) {
        hval_t *val = &point->val[i];

        /* Fetch min and max according to variable type */
        switch (val->type) {
        case HVAL_INT:
            ptr += snprintf(ptr, end - ptr, "%s = %ld",
                            local_sig.range[i].name, val->value.i);
            break;

        case HVAL_REAL:
            ptr += snprintf(ptr, end - ptr, "%s = %f",
                            local_sig.range[i].name, val->value.r);
            break;

        case HVAL_STR:
        default:
            return -1;
        }

        if (i < point->n - 1)
            ptr += snprintf(ptr, end - ptr, " && ");
    }
    return 0;
}

/* XXX - We don't actually update the session signature just yet,
 * resulting in correct, but less optimal point generation.
 */
int update_bounds(hsignature_t *sig)
{
    char cmd[MAX_CMD_LEN];
    char *retstr;
    int i, retval;

    for (i = 0; i < local_sig.range_len; ++i) {
        hrange_t *range = &local_sig.range[i];

        /* Write the domain text with variable constraints to the file */
        snprintf(cmd, sizeof(cmd),
                 "symbolic %s;\n"
                 "D:={[%s]: %s && %s};\n"
                 "range D;",
                 range->name, vars_text, bounds_text, user_text);

        /* Call omega calculator */
        retstr = call_omega_calc(cmd);
        if (!retstr)
            return -1;

        /* Parse the result */
        while (*retstr) {
            if (strncmp(">>>", retstr, 3) == 0)
                goto nextline; /* Skip echo lines. */

            switch (range->type) {
            case HVAL_INT:
                retval = sscanf(retstr, "{ %ld <= %*s <= %ld }\n",
                                &range->bounds.i.min, &range->bounds.i.max);
                break;

            case HVAL_REAL:
                retval = sscanf(retstr, "{ %lf <= %*s <= %lf }\n",
                                &range->bounds.r.min, &range->bounds.r.max);
                break;

            case HVAL_STR:
            default:
                session_error("Constraint layer cannot handle string types");
                return -1;
            }

            if (retval != 2)
                fprintf(stderr, "Unexpected Omega Calculator output: %.*s\n",
                        (int) strcspn(retstr, "\n"), retstr);
            break;

          nextline:
            retstr += strcspn(retstr, "\n");
            if (*retstr)
                ++retstr;
        }
        if (retval != 2) {
            session_error("Error parsing Omega Calculator output");
            return -1;
        }
    }

    if (!quiet) {
        if (!hsignature_equal(sig, &local_sig)) {
            fprintf(stderr, "For the given constraints, we suggest re-running"
                    " the session with these bounds:\n");

            for (i = 0; i < local_sig.range_len; ++i) {
                hrange_t *range = &local_sig.range[i];

                switch (range->type) {
                case HVAL_INT:
                    fprintf(stderr, "INT %s = {%ld, %ld, %ld}\n",
                            range->name,
                            range->bounds.i.min,
                            range->bounds.i.max,
                            range->bounds.i.step);
                    break;
                case HVAL_REAL:
                    fprintf(stderr, "REAL %s = {%lf, %lf, %lf}\n",
                            range->name,
                            range->bounds.r.min,
                            range->bounds.r.max,
                            range->bounds.r.step);
                    break;
                case HVAL_STR:
                default:
                    return -1;
                }
            }
        }
    }
    return 0;
}

int check_validity(hpoint_t *point)
{
    char cmd[MAX_CMD_LEN];
    char *retstr;
    int retval;

    if (build_point_text(point) != 0)
        return -1;

    snprintf(cmd, sizeof(cmd),
             "D:={[%s]: %s && %s && %s};\n"
             "D;",
             vars_text, bounds_text, user_text, point_text);

    /* Call omega test */
    retstr = call_omega_calc(cmd);
    if (!retstr)
        return -1;

    /* Parse result */
    while (*retstr) {
        char c;
        if (strncmp(">>>", retstr, 3) == 0)
            goto nextline; /* Skip echo lines. */

        retval = sscanf(retstr, " { %*s : FALSE %c ", &c);

        if (retval == 1)
            break;

      nextline:
        retstr += strcspn(retstr, "\n");
        if (*retstr)
            ++retstr;
    }

    /* If sscanf ever succeeded, retval will be 1 and the point is invalid. */
    return (retval != 1);
}

char *call_omega_calc(const char *cmd)
{
    static char *buf = NULL;
    static int buf_cap = 4096;
    char *child_argv[2];
    char *ptr;
    pid_t oc_pid;
    int fd, count;

    if (buf == NULL) {
        buf = (char *) malloc(sizeof(char) * buf_cap);
        if (!buf)
            return NULL;
    }

    child_argv[0] = (char *) omega_bin;
    child_argv[1] = NULL;
    fd = socket_launch(omega_bin, child_argv, &oc_pid);
    if (!fd) {
        session_error("Could not launch Omega Calculator");
        return NULL;
    }

    if (socket_write(fd, cmd, strlen(cmd)) < 0) {
        session_error("Could not send command to Omega Calculator");
        return NULL;
    }

    if (shutdown(fd, SHUT_WR) != 0) {
        session_error("Internal error: Could not shutdown write to Omega");
        return NULL;
    }

    ptr = buf;
    while ( (count = socket_read(fd, ptr, buf_cap - (buf - ptr))) > 0) {
        ptr += count;
        if (ptr == buf + buf_cap) {
            if (array_grow(&buf, &buf_cap, sizeof(char)) != 0) {
                session_error("Internal error: Could not grow buffer for"
                              " Omega Calculator input");
                return NULL;
            }
            ptr = buf + strlen(buf);
        }
        *ptr = '\0';
    }
    if (count < 0) {
        session_error("Could not read output from Omega Calculator");
        return NULL;
    }

    if (close(fd) != 0) {
        session_error("Internal error: Could not close Omega socket");
        return NULL;
    }

    if (waitpid(oc_pid, NULL, 0) != oc_pid) {
        session_error("Internal error: Could not reap Omega process");
        return NULL;
    }

    return buf;
}
