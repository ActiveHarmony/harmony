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
 * conjunction of the set is applied to the [search space](\ref
 * intro_space).
 *
 * \note This processing layer requires the Omega Calculator, which is
 * available at:<br>
 * <https://github.com/davewathaverford/the-omega-project/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>

#include "session-core.h"
#include "hsignature.h"
#include "hpoint.h"
#include "hutil.h"
#include "hutil.h"
#include "defaults.h"

#define MAX_TEXT_LEN 1024
#define REG_TEXT_LEN 128
#define SHORT_TEXT_LEN 32

const char harmony_layer_name[] = "constraint";

/* Keep copy of session information */
hsignature_t sess_sig;

char relation_text[MAX_TEXT_LEN]; /* per param constraint, fixed after
                                   * initialization
                                   */
char dep_text[MAX_TEXT_LEN]; /* user input constraint text */
char domain_text_init[REG_TEXT_LEN];
char domain_text[MAX_TEXT_LEN]; /* The entire omega command sent to
                                 * omega calculator, including
                                 * relation_text[]
                                 */
char param_value_text[MAX_TEXT_LEN];

char relation[REG_TEXT_LEN];

/* Some global variables */
int paramNum;

char *build_param_constraint(int idx)
{
    char paramName[SHORT_TEXT_LEN];
    char paramMin[SHORT_TEXT_LEN];
    char paramMax[SHORT_TEXT_LEN];
    char paramStep[SHORT_TEXT_LEN];

    snprintf(paramName, sizeof(paramName), "%s", sess_sig.range[idx].name);

    /* Fetch min and max according to variable type */
    switch (sess_sig.range[idx].type) {
    case HVAL_INT:
        snprintf(paramMin, sizeof(paramMin), "%ld", sess_sig.range[idx].bounds.i.min);
        snprintf(paramMax, sizeof(paramMax), "%ld", sess_sig.range[idx].bounds.i.max);
        snprintf(paramStep, sizeof(paramStep), "%ld", sess_sig.range[idx].bounds.i.step);
        break;

    case HVAL_REAL:
        snprintf(paramMin, sizeof(paramMin), "%f", sess_sig.range[idx].bounds.r.min);
        snprintf(paramMax, sizeof(paramMax), "%f", sess_sig.range[idx].bounds.r.max);
        snprintf(paramStep, sizeof(paramStep), "%f", sess_sig.range[idx].bounds.r.step);
        break;

    case HVAL_STR:

    default:
        break;
    }

    snprintf(relation, sizeof(relation), "%s <= %s <= %s",
             paramMin, paramName, paramMax);

    return relation;
}

static void read_dep_constraint() {
    FILE *dep_file;
    dep_file = fopen("constraint.in", "rb+");
    if (dep_file == NULL) {
        strcpy(dep_text, "\0");
        return;
    }

    char line[REG_TEXT_LEN];
    strcpy(dep_text, "");
    while(fgets(line, sizeof line, dep_file)) {
        if (line != NULL) {
            strcat(dep_text, " && ");
            strcat(dep_text, line);
        }
    }
}

static void read_param_value(hpoint_t *pt) {
/*reset the value string*/
    strcpy(param_value_text, "");

/*read current values*/
    for (int i = 0; i < paramNum; i++) {
        char name[SHORT_TEXT_LEN];
        char temp[REG_TEXT_LEN];

        long val_i;
        double val_r;
        strcpy(name, sess_sig.range[i].name);

        switch (sess_sig.range[i].type) {
        case HVAL_INT:
            val_i = pt->val[i].value.i;
            snprintf(temp, sizeof(temp), " && %s = %ld", name, val_i);
            strcat(param_value_text, temp);
            break;

        case HVAL_REAL:
            val_r = pt->val[i].value.r;
            snprintf(temp, sizeof(temp), " && %s = %f", name, val_r);
            strcat(param_value_text, temp);
            break;

        case HVAL_STR:
        case HVAL_UNKNOWN:
        case HVAL_MAX:
            break;
        }
    }
//fprintf(stderr, "Values are %s\n", param_value_text);

}

static void call_omega_calc()
{
    char cmdline[256];

    sprintf(cmdline, "./oc domain.in > result.out");
    system(cmdline);
}

int constraint_init(hsignature_t *sig)
{
    /* Initializaing constraint */
    fprintf(stderr, "Initializing constraint plugin for session %s, please make sure the omega calculator is installed under 'bin' directory.\nPlease specify parameter dependencies in 'constraint.in' under 'bin' directory.\n The example of the content of 'constraint.in' is as follows:\n To assign a dependency for two parameters named 'x' and 'y', simply type in constraints like 'x + y < 10' in the file.", sig->name);

    /* Have a copy of signature */
    hsignature_copy(&sess_sig, sig);

    paramNum = sig->range_cap;

    /* Create domain text input */
    FILE *domain_file;
    domain_file = fopen("domain.in", "wb");
    if (domain_file == NULL) {
        freopen("domain.in", "wb", domain_file);
    }
    fclose(domain_file);

    /* Initializing domain_text_init */
    strcpy(domain_text_init, "D:={[");

    /* Iterate through each variable and create relation text */
    for (int i = 0; i < paramNum; i++) {
        /* Build relation text */
        strcat(relation_text, build_param_constraint(i));
        if (i < paramNum - 1)
            strcat(relation_text, " && ");

        /* Fill domain_text_init */
        strcat(domain_text_init, sig->range[i].name);
        if (i < paramNum - 1)
            strcat(domain_text_init, ",");
    }

    /* Finalizaing domain_text_init */
    strcat(domain_text_init, "]:");

    /* Iterate through user input domain and create constraint text */
    read_dep_constraint();

    /* Calculate the range for each variable */
    for (int i = 0; i < paramNum; i++) {
        //int new_low, new_upper;
        //double new_low, new_upper;
        char paramName[SHORT_TEXT_LEN];
        strcpy(paramName, sess_sig.range[i].name);

        FILE *domain_file;
        domain_file = fopen("domain.in", "rwb+");

        /* Write the domain text with variable constraints to the file */
        fprintf(domain_file, "symbolic %s;\n%s%s%s};\nRange D;",
                paramName, domain_text_init, relation_text, dep_text);

        fclose(domain_file);

        /* Call omega calculator */
        call_omega_calc();

        /* Parse the result */
        FILE *result_file;
        char line[MAX_TEXT_LEN];
        char min_text[SHORT_TEXT_LEN];
        char max_text[SHORT_TEXT_LEN];
        char name[SHORT_TEXT_LEN];
        result_file = fopen("result.out", "rb+");

        while(fgets(line, sizeof line, result_file)) {
            if (sscanf(line, "{ %s <= %s <= %s}\n",
                       min_text, name, max_text) > 0)
            {
                if (strcmp(name, paramName) == 0) {
                    sig->range[i].bounds.i.min = atoi(min_text);
                    sig->range[i].bounds.i.max = atoi(max_text);
                    break;
                }
            }
        }
        fclose(result_file);

    /* Update the result */
    }

    return 0;
}

int constraint_generate(hflow_t *flow, hpoint_t *pt)
{
    /* Read current param values */
    flow->status = HFLOW_ACCEPT;
    read_param_value(pt);

    /* Built Omega test command using new parameter set */
    FILE *domain_file;
    domain_file = fopen("domain.in", "w");

    fprintf(domain_file, "%s%s%s%s};\nD;",
            domain_text_init, relation_text, dep_text, param_value_text);

    fclose(domain_file);

    /* Call omega test */
    call_omega_calc();

    /* Parse result */
    FILE *result_file;
    char line[REG_TEXT_LEN];
    char param_list[REG_TEXT_LEN];
    char result[SHORT_TEXT_LEN];
    result_file = fopen("result.out", "rb+");
    while(fgets(line, sizeof line, result_file)) {
        if (sscanf(line, "{%s : %s }\n", param_list, result) > 0) {
            if(strcmp(result, "FALSE") == 0) {
                flow->status = HFLOW_REJECT;
                break;
            }
        }
    }

    fclose(result_file);
    return 0;
}
