/*
 * Copyright 2003-2016 Jeffrey K. Hollingsworth
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
#define _XOPEN_SOURCE 500 // Needed for lrand48().

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>

#include "hclient.h"
#include "defaults.h"

#define MAX_LOOP 500
#define MAX_TASK 10

typedef struct sinfo {
    htask_t*    htask;
    long        ival;
    double      rval;
    const char* sval;
} sinfo_t;

const char* fruits[] = {"apples",
                        "bananas",
                        "cherries",
                        "figs",
                        "grapes",
                        "oranges",
                        "peaches",
                        "pineapple",
                        "pears",
                        "watermelon",
                        "strawberries",
                        NULL};

/*
 * A simple performance function is defined here for illustration purposes.
 */
double application(long ival, double rval, const char* string)
{
    int i;
    double sval = 0.0;
    for (i = 0; string[i]; ++i)
        sval += string[i];
    return sval * ival / rval;
}

hdef_t* define_search(void)
{
    hdef_t* hdef = ah_def_alloc();
    if (!hdef) {
        fprintf(stderr, "Error allocating a definition descriptor");
        goto error;
    }

    // Define a tuning variable that resides in the integer domain.
    if (ah_def_int(hdef, "i_var",  1, 1000, 1, NULL) != 0) {
        fprintf(stderr, "Error defining an integer tuning variable");
        goto error;
    }

    // Define a tuning variable that resides in the real domain.
    if (ah_def_real(hdef, "r_var", 0.0001, 1.0, 0.0001, NULL) != 0) {
        fprintf(stderr, "Error defining a real tuning variable");
        goto error;
    }

    for (int i = 0; fruits[i]; ++i) {
        if (ah_def_enum_value(hdef, "s_var", fruits[i]) != 0) {
            fprintf(stderr, "Error defining an enumerated tuning variable");
            goto error;
        }
    }
    return hdef;

  error:
    ah_def_free(hdef);
    return NULL;
}

int start_search(hdesc_t* hdesc, hdef_t* hdef, sinfo_t* sinfo, int num)
{
    char namebuf[32];

    // Provide a unique name to the search.
    snprintf(namebuf, sizeof(namebuf), "multi_%d", num);
    if (ah_def_name(hdef, namebuf) != 0) {
        fprintf(stderr, "Error naming new search");
        return -1;
    }

    // Start a new search in the session.
    sinfo->htask = ah_start(hdesc, hdef);
    if (!sinfo->htask) {
        fprintf(stderr, "Error starting new search");
        return -1;
    }

    if (ah_bind_int(sinfo->htask, "i_var", &sinfo->ival) != 0) {
        fprintf(stderr, "Error binding 'i_var' to local memory");
        return -1;
    }

    if (ah_bind_real(sinfo->htask, "r_var", &sinfo->rval) != 0) {
        fprintf(stderr, "Error binding 'r_var' to local memory");
        return -1;
    }

    if (ah_bind_enum(sinfo->htask, "s_var", &sinfo->sval) != 0) {
        fprintf(stderr, "Error binding 's_var' to local memory");
        return -1;
    }
    return 0;
}

void shuffle(int* order)
{
    for (int i = 0; i < MAX_TASK; ++i) {
        int j = lrand48() % (i + 1);
        order[i] = order[j];
        order[j] = i;
    }
}

int main(int argc, char* argv[])
{
    hdef_t* hdef = NULL;
    int retval = 0;
    double perf;
    sinfo_t slist[MAX_TASK] = {{NULL}};
    int order[MAX_TASK];

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            fprintf(stderr, "Usage: %s"
                    " [KEY_1=VAL_1] ... [KEY_N=VAL_N]\n\n", argv[0]);
            return 0;
        }
    }

    // Initialize a Harmony client.
    hdesc_t* hdesc = ah_alloc();
    if (hdesc == NULL) {
        fprintf(stderr, "Error initializing a Harmony descriptor");
        goto cleanup;
    }
    ah_args(hdesc, &argc, argv);

    printf("Starting Harmony...\n");
    if (ah_connect(hdesc, NULL, 0) != 0) {
        fprintf(stderr, "Error connecting to Harmony session");
        goto error;
    }

    // Define a new tuning search.
    hdef = define_search();
    if (!hdef)
        goto error;

    // Begin a new tuning search.
    for (int i = 0; i < MAX_TASK; ++i) {
        if (start_search(hdesc, hdef, &slist[i], i) != 0)
            goto error;
    }

    // Main tuning loop.
    for (int i = 0; i < MAX_LOOP; ++i) {

        // Fetch new values from the search task in a random order.
        shuffle(order);
        for (int j = 0; j < MAX_TASK; ++j) {
            int idx = order[j];
            if (!slist[idx].htask)
                continue;

            int hresult = ah_fetch(slist[idx].htask);
            if (hresult < 0) {
                fprintf(stderr, "Error fetching values from tuning session");
                goto error;
            }
            else if (hresult == 0) {
                printf("No testing point available.  Waiting.\n");
                sleep(1);
                --j;
                continue;
            }
        }

        // Report new values to the search task in a different random order.
        shuffle(order);
        for (int j = 0; j < MAX_TASK; ++j) {
            int idx = order[j];
            if (!slist[idx].htask)
                continue;

            sinfo_t* sinfo = &slist[idx];
            perf = application(sinfo->ival, sinfo->rval, sinfo->sval);

            if (ah_report(sinfo->htask, &perf) != 0) {
                fprintf(stderr, "Error reporting performance to server");
                goto error;
            }

            if (ah_converged(sinfo->htask)) {
                if (ah_leave(sinfo->htask) != 0) {
                    fprintf(stderr, "Error leaving search #%d", idx);
                    goto error;
                }

                if (ah_best(sinfo->htask) != 0) {
                    fprintf(stderr, "Error retrieving best search");
                    goto error;
                }

                printf("#%d converged. %4d evals. Best: %4ld, %.4f, %s\n",
                       idx, i, sinfo->ival, sinfo->rval, sinfo->sval);

                if (ah_kill(sinfo->htask) != 0) {
                    fprintf(stderr, "Error killing search #%d", idx);
                    goto error;
                }
                sinfo->htask = NULL;
            }
        }
    }

    for (int i = 0; i < MAX_TASK; ++i) {
        if (!slist[i].htask)
            continue;

        if (ah_best(slist[i].htask) != 0) {
            fprintf(stderr, "Error retrieving best search");
            goto error;
        }

        printf("#%d: Leaving after %d evals. Best: %4ld, %.4f, %s\n",
               i, MAX_LOOP, slist[i].ival, slist[i].rval, slist[i].sval);

        if (ah_kill(slist[i].htask) != 0) {
            fprintf(stderr, "Error killing search #%d", i);
            goto error;
        }
    }

    goto cleanup;

  error:
    fprintf(stderr, ": %s\n", ah_error());
    retval = -1;

  cleanup:
    // Close the connection to the tuning session.
    if (ah_close(hdesc) != 0) {
        fprintf(stderr, "Error disconnecting from Harmony session: %s.\n",
                ah_error());
    }
    ah_def_free(hdef);
    ah_free(hdesc);
    return retval;
}
