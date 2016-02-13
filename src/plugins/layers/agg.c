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

/**
 * \page agg Aggregator (agg.so)
 *
 * This processing layer forces each point to be evaluated multiple
 * times before it may proceed through the auto-tuning
 * [feedback loop](\ref intro_feedback).  When the requisite number of
 * evaluations has been reached, an aggregating function is applied to
 * consolidate the set performance values.
 */

#include "session-core.h"
#include "hspace.h"
#include "hpoint.h"
#include "hperf.h"
#include "hutil.h"
#include "hcfg.h"

#include <stdlib.h>
#include <strings.h>

/*
 * Name used to identify this plugin layer.
 * All Harmony plugin layers must define this variable.
 */
const char harmony_layer_name[] = "agg";

/*
 * Configuration variables used in this plugin.
 * These will automatically be registered by session-core upon load.
 */
hcfg_info_t plugin_keyinfo[] = {
    { CFGKEY_AGG_FUNC, NULL,
      "Aggregation function to use.  Valid values are min, max, mean, "
      "and median." },
    { CFGKEY_AGG_TIMES, NULL,
      "Number of performance values to collect before performing the "
      "aggregation function." },
    { NULL }
};

typedef struct store {
    int id;
    int count;
    hperf_t* trial;
} store_t;

typedef enum aggfunc {
    AGG_UNKNOWN = 0,
    AGG_MIN,
    AGG_MAX,
    AGG_MEAN,
    AGG_MEDIAN
} aggfunc_t;

/*
 * Structure to hold all data needed by an individual search instance.
 *
 * To support multiple parallel search sessions, no global variables
 * should be defined or used in this plug-in layer.
 */
typedef struct data {
    aggfunc_t agg_type;
    int       trial_per_point;
    store_t*  slist;
    int       slist_len;
} data_t;

static data_t* data;

/*
 * Internal helper function prototypes.
 */
static data_t* alloc_data(void);
static void    perf_mean(hperf_t* dst, hperf_t* src, int count);
static int     perf_sort(const void* _a, const void* _b);
static int     add_storage(void);

int agg_init(hspace_t* space)
{
    const char* val;

    if (!data) {
        // One-time search instance initialization.
        data = alloc_data();
        if (!data) {
            session_error("Could not allocate data for Aggregator layer");
            return -1;
        }
    }

    // Remaining setup needed for every initialization, including
    // re-initialization due to a restarted search.
    //
    val = hcfg_get(session_cfg, CFGKEY_AGG_FUNC);
    if (!val) {
        session_error(CFGKEY_AGG_FUNC " configuration key empty.");
        return -1;
    }
    if      (strcasecmp(val, "MIN") == 0)    data->agg_type = AGG_MIN;
    else if (strcasecmp(val, "MAX") == 0)    data->agg_type = AGG_MAX;
    else if (strcasecmp(val, "MEAN") == 0)   data->agg_type = AGG_MEAN;
    else if (strcasecmp(val, "MEDIAN") == 0) data->agg_type = AGG_MEDIAN;
    else {
        session_error("Invalid " CFGKEY_AGG_FUNC " configuration value.");
        return -1;
    }

    data->trial_per_point = hcfg_int(session_cfg, CFGKEY_AGG_TIMES);
    if (data->trial_per_point < 2) {
        session_error("Invalid " CFGKEY_AGG_TIMES " configuration value.");
        return -1;
    }

    data->slist = NULL;
    data->slist_len = 0;
    return add_storage();
}

int agg_analyze(hflow_t* flow, htrial_t* trial)
{
    int i;
    store_t* store;

    for (i = 0; i < data->slist_len; ++i) {
        if (data->slist[i].id == trial->point.id || data->slist[i].id == -1)
            break;
    }
    if (i == data->slist_len && add_storage() != 0)
        return -1;

    store = &data->slist[i];
    if (store->id == -1) {
        store->id = trial->point.id;
        store->count = 0;
    }

    if (hperf_copy(&store->trial[ store->count ], &trial->perf) != 0)
        return -1;

    ++store->count;

    if (store->count < data->trial_per_point) {
        flow->status = HFLOW_RETRY;
        return 0;
    }

    switch (data->agg_type) {
    case AGG_MIN:
        for (i = 0; i < data->trial_per_point; ++i)
            if (hperf_cmp(&trial->perf, &store->trial[i]) > 0)
                hperf_copy(&trial->perf, &store->trial[i]);
        break;

    case AGG_MAX:
        for (i = 0; i < data->trial_per_point; ++i)
            if (hperf_cmp(&trial->perf, &store->trial[i]) < 0)
                hperf_copy(&trial->perf, &store->trial[i]);
        break;

    case AGG_MEAN:
        perf_mean(&trial->perf, store->trial, data->trial_per_point);
        break;

    case AGG_MEDIAN:
        qsort(store->trial, data->trial_per_point, sizeof(hperf_t), perf_sort);

        i = (data->trial_per_point - 1) / 2;
        if (i % 2)
            hperf_copy(&trial->perf, &store->trial[i]);
        else
            perf_mean(&trial->perf, &store->trial[i], 2);
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

int agg_fini(void)
{
    for (int i = 0; i < data->slist_len; ++i) {
        for (int j = 0; j < data->trial_per_point; ++j)
            hperf_fini(&data->slist[i].trial[j]);
        free(data->slist[i].trial);
    }
    free(data->slist);

    free(data);
    data = NULL;
    return 0;
}

/*
 * Internal helper function prototypes.
 */
data_t* alloc_data(void)
{
    data_t* retval = calloc(1, sizeof(*retval));
    if (!retval)
        return NULL;

    return retval;
}

void perf_mean(hperf_t* dst, hperf_t* src, int count)
{
    int i, j;

    // Initialize the destination hperf_t.
    for (i = 0; i < dst->len; ++i)
        dst->obj[i] = 0.0;

    // Calculate the mean of each objective individually.
    for (j = 0; j < count; ++j)
        for (i = 0; i < dst->len; ++i)
            dst->obj[i] += src[j].obj[i];

    for (i = 0; i < dst->len; ++i)
        dst->obj[i] /= data->trial_per_point;
}

int perf_sort(const void* _a, const void* _b)
{
    double a = hperf_unify((const hperf_t*)_a);
    double b = hperf_unify((const hperf_t*)_b);

    return (a > b) - (a < b);
}

int add_storage(void)
{
    int prev_len = data->slist_len;

    if (array_grow(&data->slist, &data->slist_len,
                   sizeof(*data->slist)) != 0)
    {
        session_error("Could not allocate memory for aggregator list");
        return -1;
    }

    while (prev_len < data->slist_len) {
        data->slist[prev_len].id = -1;
        data->slist[prev_len].trial = calloc(data->trial_per_point,
                                             sizeof(hperf_t));
        if (!data->slist[prev_len].trial) {
            session_error("Could not allocate memory for trial list");
            return -1;
        }
        ++prev_len;
    }
    return 0;
}
