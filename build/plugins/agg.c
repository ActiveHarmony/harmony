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
#include "hutil.h"

#include <stdlib.h>
#include <strings.h>

/*
 * Name used to identify this plugin.  All Harmony plugins must define
 * this variable.
 */
const char harmony_layer_name[] = "agg";

void perf_sort(double *arr, int count);
int add_storage(void);

typedef struct store {
    int id;
    int count;
    double trial[];
} store_t;

typedef enum aggfunc {
    AGG_UNKNOWN = 0,
    AGG_MIN,
    AGG_MAX,
    AGG_MEAN,
    AGG_MEDIAN
} aggfunc_t;

aggfunc_t agg_type;
int trial_per_point;
int sizeof_store;
store_t *slist;
int slist_len;

int agg_init(hsignature_t *sig)
{
    const char *val;

    val = session_query("AGG_FUNCTION");
    if (!val) {
        session_error("AGG_FUNCTION configuration key empty");
        return -1;
    }
    if      (strcasecmp(val, "MIN") == 0)    agg_type = AGG_MIN;
    else if (strcasecmp(val, "MAX") == 0)    agg_type = AGG_MAX;
    else if (strcasecmp(val, "MEAN") == 0)   agg_type = AGG_MEAN;
    else if (strcasecmp(val, "MEDIAN") == 0) agg_type = AGG_MEDIAN;
    else {
        session_error("Invalide AGG_FUNCTION configuration value");
        return -1;
    }

    val = session_query("AGG_TIMES");
    if (!val) {
        session_error("AGG_TIMES configuration key empty");
        return -1;
    }

    trial_per_point = atoi(val);
    if (!trial_per_point) {
        session_error("Invalid AGG_TIMES configuration value");
        return -1;
    }
    sizeof_store = sizeof(store_t) + sizeof(double) * trial_per_point;

    return add_storage();
}

int agg_analyze(hflow_t *flow, htrial_t *trial)
{
    int i;
    store_t *store;

    for (i = 0; i < slist_len; ++i) {
        if (slist[i].id == trial->point.id || slist[i].id == -1)
            break;
    }
    if (i == slist_len && add_storage() != 0)
        return -1;

    store = &slist[i];
    if (store->id == -1) {
        store->id = trial->point.id;
        store->count = 0;
    }
    store->trial[ store->count ] = trial->perf;
    ++store->count;

    if (store->count < trial_per_point) {
        flow->status = HFLOW_RETRY;
        return 0;
    }

    switch (agg_type) {
    case AGG_MIN:
        for (i = 0; i < trial_per_point; ++i)
            if (trial->perf > store->trial[i])
                trial->perf = store->trial[i];
        break;

    case AGG_MAX:
        for (i = 0; i < trial_per_point; ++i)
            if (trial->perf < store->trial[i])
                trial->perf = store->trial[i];
        break;

    case AGG_MEAN:
        trial->perf = 0;
        for (i = 0; i < trial_per_point; ++i)
            trial->perf += store->trial[i];
        trial->perf /= trial_per_point;
        break;

    case AGG_MEDIAN:
        perf_sort(store->trial, trial_per_point);

        i = (trial_per_point - 1) / 2;
        if (i % 2)
            trial->perf = store->trial[i];
        else
            trial->perf = (store->trial[i] + store->trial[i+1]) / 2;
        break;

    default:
        session_error("Internal error: Invalid AGG type");
        return -1;
    }

    store->id = -1;
    store->count = 0;
    flow->status = HFLOW_ACCEPT;
    return 0;
}

void perf_sort(double *arr, int count)
{
    int i, j;
    double tmp;

    for (i = 0; i < count - 1; ++i)
        for (j = i + 1; j < count; ++j)
            if (arr[i] > arr[j]) {
                tmp    = arr[i];
                arr[i] = arr[j];
                arr[j] = tmp;
            }
}

int add_storage(void)
{
    int prev_len = slist_len;

    if (array_grow(&slist, &slist_len, sizeof_store) != 0) {
        session_error("Could not allocate memory for aggregator list");
        return -1;
    }

    while (prev_len < slist_len) {
        slist[prev_len].id = -1;
        ++prev_len;
    }
    return 0;
}
