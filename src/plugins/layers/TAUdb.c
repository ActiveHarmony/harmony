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
 * \page TAUdb TAUdb Interface (TAUdb.so)
 *
 * \warning This processing layer is still considered pre-beta.
 *
 * This processing layer uses the TAU Performance System's C API to
 * keep a log of point/performance pairs to disk as they flow through
 * the auto-tuning [feedback loop](\ref intro_feedback).
 *
 * The `LIBTAUDB` [build variable](\ref start_install) must be defined
 * at build time for this plug-in to be available, since it is
 * dependent on a library distributed with TAU.  The distribution of
 * TAU is available here:
 * - http://www.cs.uoregon.edu/research/tau/downloads.php
 *
 * And `LIBTAUDB` should point to the `tools/src/taudb_c_api`
 * directory within that distribution.
 *
 * **Configuration Variables**
 * Key                | Type    | Default  | Description
 * ------------------ | ------- | -------- | -----------
 * TAUDB_NAME         | String  | [none]   | Name of the PostgreSQL database.
 * TAUDB_STORE_METHOD | String  | one_time | Determines when statistics are computed:<br> **one_time** - With each database write.<br> **real_time** - At session cleanup time.
 * TAUDB_STORE_NUM    | Integer | 0        | Number of reports to cache before writing to database.
 * CLIENT_COUNT       | Integer | 1        | Number of participating tuning clients.
 */

#include "session-core.h"
#include "hsignature.h"
#include "hpoint.h"
#include "hutil.h"
#include "defaults.h"

#include "taudb_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>

#define CLIENTID_INIT 16
#define SECONDARY_METADATA_NUM 10
#define REG_STR_LEN 32
#define MAX_STR_LEN 1024
#define POINTS_PER_SAVE 20

char harmony_layer_name[] = "TAUdb";

TAUDB_METRIC      *metric;
TAUDB_TRIAL       *taudb_trial;
TAUDB_CONNECTION  *connection;
TAUDB_THREAD      *thread;
TAUDB_TIMER_GROUP *timer_group;

typedef struct cinfo {
    char *id;
    int timer;
} cinfo_t;
cinfo_t *client = NULL;

hsignature_t sess_sig;
int param_max;
int client_max;
int save_counter = 0;
int taudb_store_type; //1 for real time, 0 for one-time
int total_record_num = 0;

/* Several Pre-declaration */
int client_idx(void);
int save_timer_parameter(TAUDB_TIMER* timer, htrial_t *trial);
int harmony_taudb_trial_init(const char *appName, const char *trialname);
TAUDB_THREAD* harmony_taudb_create_thread(int threadNum);
void harmony_taudb_get_secondary_metadata(TAUDB_THREAD* thread, char* opsys,
                                          char* machine, char* release,
                                          char* hostname, char* procnum,
                                          char* cpuvendor, char* cpumodel,
                                          char* clkfreq, char* cachesize);

/* Initialization of TAUdb
 * Return 0 for success, -1 for error
 */
int TAUdb_init(hsignature_t *sig)
{
    char *tmpstr;

    /*Connecting to TAUdb*/
    tmpstr = (char *) session_getcfg("TAUDB_NAME");
    if (!tmpstr) {
        session_error("TAUdb connection failed: config file not found");
        return -1;
    }

    connection = taudb_connect_config(tmpstr);

    /*Check if the connection has been established*/
    taudb_check_connection(connection);

    /*Initializing Trial*/
    if (harmony_taudb_trial_init(sig->name, sig->name) != 0) {
        session_error("Failed to create TAUdb trial");
        return -1;
    }

    /*Create a metric*/
    metric = taudb_create_metrics(1);
    metric->name = taudb_strdup("TIME");
    taudb_add_metric_to_trial(taudb_trial, metric);

    /*Initializing Timer group*/
    timer_group = taudb_create_timer_groups(1);
    timer_group->name = taudb_strdup("Harmony Perf");
    taudb_add_timer_group_to_trial(taudb_trial, timer_group);

    param_max = sig->range_len;
    client_max = atoi( session_getcfg(CFGKEY_CLIENT_COUNT) );
    taudb_trial->node_count = client_max;

    tmpstr = (char *) session_getcfg("TAUDB_STORE_METHOD");
    if (strcmp(tmpstr, "real_time") == 0) {
        taudb_store_type = 1;
    }
    else if (strcmp(tmpstr, "one_time") == 0) {
        taudb_store_type = 0;
    }
    else {
        session_error("Invalid TAUDB_STORE_METHOD");
        return -1;
    }
    total_record_num = atoi( session_getcfg("TAUDB_STORE_NUM") );

    thread = harmony_taudb_create_thread(client_max);

    /* Client id map to thread id*/
    client = (cinfo_t *) malloc(client_max * sizeof(cinfo_t));
    if (!client) {
        session_error("Internal error: Could not allocate client list");
        return -1;
    }
    memset(client, 0, client_max * sizeof(cinfo_t));

    sess_sig = HSIGNATURE_INITIALIZER;
    if (hsignature_copy(&sess_sig, sig) != 0) {
        session_error("Internal error: Could not copy session signature");
        return -1;
    }
    return 0;
}

int TAUdb_join(const char *id)
{
    int idx;
    char node_name[REG_STR_LEN];
    char sys_name[REG_STR_LEN];
    char release[REG_STR_LEN];
    char machine[REG_STR_LEN];
    char proc_num[REG_STR_LEN];
    char cpu_vendor[REG_STR_LEN];
    char cpu_model[REG_STR_LEN];
    char cpu_freq[REG_STR_LEN];
    char cache_size[REG_STR_LEN];

    idx = client_idx();
    if (idx < 0)
        return -1;

    sscanf(id, "%[^$$]$$%[^$$]$$%[^$$]$$%[^$$]$$"
           "%[0-9]$$%[^$$]$$%[^$$]$$%[^$$]$$%[^$$]\n",
           node_name, sys_name, release, machine,
           proc_num, cpu_vendor, cpu_model, cpu_freq, cache_size);

    harmony_taudb_get_secondary_metadata(&thread[idx],
                                         sys_name, machine, release,
                                         node_name, proc_num, cpu_vendor,
                                         cpu_model, cpu_freq, cache_size);
    return 0;
}

/* Invoked upon client reports performance
 * This routine stores the reported performance to TAUdb
 */
int TAUdb_analyze(hflow_t *flow, htrial_t *ah_trial)
{
    int idx;
    char timer_name[REG_STR_LEN];

    //taudb_save_metrics(connection, taudb_trial, 1);

    /*create a timer*/
    TAUDB_TIMER *timer = taudb_create_timers(1);
    TAUDB_TIMER_VALUE *timer_value = taudb_create_timer_values(1);
    TAUDB_TIMER_CALLPATH *timer_callpath = taudb_create_timer_callpaths(1);
    TAUDB_TIMER_CALL_DATA *timer_call_data = taudb_create_timer_call_data(1);
    //TAUDB_TIMER_GROUP *timer_group = taudb_create_timer_groups(1);

    idx = client_idx();
    if (idx < 0)
        return -1;

    /*parse input string and get param information*/
    //timer_group->name = taudb_strdup("Harmony Perf");

    ++client[idx].timer;
    snprintf(timer_name, sizeof(timer_name), "Runtime_%d_%d",
             idx, client[idx].timer);

    timer->name = taudb_strdup(timer_name);

    taudb_add_timer_to_trial(taudb_trial, timer);

    taudb_add_timer_to_timer_group(timer_group, timer);

    timer_callpath->timer = timer;
    timer_callpath->parent = NULL;
    taudb_add_timer_callpath_to_trial(taudb_trial, timer_callpath);

    timer_call_data->key.timer_callpath = timer_callpath;
    timer_call_data->key.thread = &thread[idx];
    timer_call_data->calls = 1;
    timer_call_data->subroutines = 0;
    taudb_add_timer_call_data_to_trial(taudb_trial, timer_call_data);

    timer_value->metric = metric;
    timer_value->inclusive = hperf_unify(ah_trial->perf);
    timer_value->exclusive = hperf_unify(ah_trial->perf);
    timer_value->inclusive_percentage = 100.0;
    timer_value->exclusive_percentage = 100.0;
    timer_value->sum_exclusive_squared = 0.0;
    taudb_add_timer_value_to_timer_call_data(timer_call_data, timer_value);

    if (save_timer_parameter(timer, ah_trial) != 0)
        return -1;

    /*Save the trial*/
    if (taudb_store_type == 0) {
        if (save_counter < total_record_num) {
            ++save_counter;
        }
        else {
            taudb_compute_statistics(taudb_trial);
            taudb_save_trial(connection, taudb_trial, 1, 1);
            taudb_store_type = -1;
        }
    }
    else if (taudb_store_type == 1) {
        if (save_counter < total_record_num) {
            ++save_counter;
        }
        else {
            taudb_save_trial(connection, taudb_trial, 1, 1);
            save_counter = 0;
        }
    }
    return 0;
}

/* Finalize a trial */
void TAUdb_fini(void)
{
    boolean update = 1;
    boolean cascade = 1;
    taudb_compute_statistics(taudb_trial);
    taudb_save_trial(connection, taudb_trial, update, cascade);

    /*Disconnect from TAUdb*/
    taudb_disconnect(connection);
}

/*----------------------------------------------------------
 * All functions below are internal functions
 */

int client_idx(void)
{
    int i;
    const char *curr_id;

    curr_id = session_getcfg(CFGKEY_CURRENT_CLIENT);
    if (!curr_id) {
        session_error("Request detected from invalid client");
        return -1;
    }

    for (i = 0; i < client_max && client[i].id; ++i) {
        if (strcmp(curr_id, client[i].id) == 0)
            return i;
    }

    if (i == client_max) {
        session_error("Too few clients estimated by " CFGKEY_CLIENT_COUNT);
        return -1;
    }

    client[i].id = stralloc(curr_id);
    if (!client[i].id) {
        session_error("Internal error: Could not allocate client id memory");
        return -1;
    }
    return i;
}

int save_timer_parameter(TAUDB_TIMER *timer, htrial_t *trial)
{
    int i;
    char value[32];
    TAUDB_TIMER_PARAMETER* param = taudb_create_timer_parameters(param_max);

    for (i = 0; i < param_max; i++) {
        /*Save name*/
        param[i].name = taudb_strdup(sess_sig.range[i].name);

        /*get value*/
        switch (trial->point.val[i].type) {
        case HVAL_INT:
            snprintf(value, sizeof(value), "%ld", trial->point.val[i].value.i);
            break;

        case HVAL_REAL:
            snprintf(value, sizeof(value), "%f", trial->point.val[i].value.r);
            break;

        case HVAL_STR:
            snprintf(value, sizeof(value), "%s", trial->point.val[i].value.s);
            break;

        default:
            session_error("Invalid point value detected");
            return -1;
        }
        param[i].value = taudb_strdup(value);
        taudb_add_timer_parameter_to_timer(timer, &param[i]);
    }

    return 0;
}

/*Initialize a trial*/
int harmony_taudb_trial_init(const char *appName, const char *trialname)
{
    char startTime[32];
    struct tm *current;
    time_t now;

    /*Check if the connection has been established*/
    taudb_check_connection(connection);

    /*Create a new trial*/
    taudb_trial = taudb_create_trials(1);
    taudb_trial->name = taudb_strdup(trialname);

    /*set the data source to other*/
    taudb_trial->data_source =
        taudb_get_data_source_by_id(taudb_query_data_sources(connection), 999);

    /*Create metadata*/
    //num_metadata = count_num_metadata();
    TAUDB_PRIMARY_METADATA* pm = taudb_create_primary_metadata(2);
    pm[0].name = taudb_strdup("Application");
    pm[0].value = taudb_strdup(appName);
    taudb_add_primary_metadata_to_trial(taudb_trial, &(pm[0]));

    //get the start time of the task
    now = time(0);
    current = localtime(&now);
    snprintf(startTime, 64, "%d%d%d", (int)current->tm_hour,
             (int)current->tm_min, (int)current->tm_sec);
    //pm = taudb_create_primary_metadata(5);
    pm[1].name = taudb_strdup("StartTime");
    pm[1].value = taudb_strdup(startTime);
    taudb_add_primary_metadata_to_trial(taudb_trial, &(pm[1]));

    boolean update = 0;
    boolean cascade = 1;
    taudb_save_trial(connection, taudb_trial, update, cascade);

    return 0;
}

TAUDB_THREAD* harmony_taudb_create_thread(int num)
{
    //int ctr = 0;
    thread = taudb_create_threads(num);
    for (int i = 0; i < num; i++) {
        thread[i].node_rank = i;
        thread[i].thread_rank = 1;
        thread[i].context_rank = 1;
        thread[i].index = 1;
        taudb_add_thread_to_trial(taudb_trial, &thread[i]);
    }
    return thread;
}

/*Get per client metadata*/
void harmony_taudb_get_secondary_metadata(TAUDB_THREAD* thread, char* opsys,
                                          char* machine, char* release,
                                          char* hostname, char* procnum,
                                          char* cpuvendor, char* cpumodel,
                                          char* clkfreq, char* cachesize)
{
    //TAUDB_SECONDARY_METADATA* cur = taudb_create_secondary_metadata(1);
    TAUDB_SECONDARY_METADATA* sm = taudb_create_secondary_metadata(1);

    /*Loading os info*/
    fprintf(stderr, "Loading OS information.\n");
    sm->key.thread = thread;
    sm->key.name = taudb_strdup("OS");
    sm->value = (char**)malloc(sizeof(char*));
    sm->value[0] = taudb_strdup(opsys);
    taudb_add_secondary_metadata_to_trial(taudb_trial, sm);
    fprintf(stderr, "OS name loaded.\n");

    sm = taudb_create_secondary_metadata(1);
    sm->key.thread = thread;
    sm->key.name = taudb_strdup("Machine");
    sm->value = (char**)malloc(sizeof(char*));
    sm->value[0] = taudb_strdup(machine);
    taudb_add_secondary_metadata_to_trial(taudb_trial, sm);
    fprintf(stderr, "Machine name loaded.\n");

    sm = taudb_create_secondary_metadata(1);
    sm->key.thread = thread;
    sm->key.name = taudb_strdup("Release");
    sm->value = (char**)malloc(sizeof(char*));
    sm->value[0] = taudb_strdup(release);
    taudb_add_secondary_metadata_to_trial(taudb_trial, sm);
    fprintf(stderr, "Release name loaded.\n");

    sm = taudb_create_secondary_metadata(1);
    sm->key.thread = thread;
    sm->key.name = taudb_strdup("HostName");
    sm->value = (char**)malloc(sizeof(char*));
    sm->value[0] = taudb_strdup(hostname);
    taudb_add_secondary_metadata_to_trial(taudb_trial, sm);
    fprintf(stderr, "Host name loaded.\n");

    /*Loading CPU info*/
    fprintf(stderr, "Loading CPU information.\n");
    sm = taudb_create_secondary_metadata(1);
    sm->key.thread = thread;
    sm->key.name = taudb_strdup("ProcNum");
    sm->value = (char**)malloc(sizeof(char*));
    sm->value[0] = taudb_strdup(procnum);
    taudb_add_secondary_metadata_to_trial(taudb_trial, sm);
    fprintf(stderr, "Processor num loaded.\n");

    sm = taudb_create_secondary_metadata(1);
    sm->key.thread = thread;
    sm->key.name = taudb_strdup("CPUVendor");
    sm->value = (char**)malloc(sizeof(char*));
    sm->value[0] = taudb_strdup(cpuvendor);
    taudb_add_secondary_metadata_to_trial(taudb_trial, sm);
    fprintf(stderr, "CPU vendor loaded.\n");

    sm = taudb_create_secondary_metadata(1);
    sm->key.thread = thread;
    sm->key.name = taudb_strdup("CPUModel");
    sm->value = (char**)malloc(sizeof(char*));
    sm->value[0] = taudb_strdup(cpumodel);
    taudb_add_secondary_metadata_to_trial(taudb_trial, sm);
    fprintf(stderr, "CPU model loaded.\n");

    sm = taudb_create_secondary_metadata(1);
    sm->key.thread = thread;
    sm->key.name = taudb_strdup("ClockFreq");
    sm->value = (char**)malloc(sizeof(char*));
    sm->value[0] = taudb_strdup(clkfreq);
    taudb_add_secondary_metadata_to_trial(taudb_trial, sm);
    fprintf(stderr, "Clock frequency loaded.\n");

    sm = taudb_create_secondary_metadata(1);
    sm->key.thread = thread;
    sm->key.name = taudb_strdup("CacheSize");
    sm->value = (char**)malloc(sizeof(char*));
    sm->value[0] = taudb_strdup(cachesize);
    taudb_add_secondary_metadata_to_trial(taudb_trial, sm);
    fprintf(stderr, "Cache size loaded.\n");

    //taudb_add_secondary_metadata_to_secondary_metadata(root, &cur);
    fprintf(stderr, "Saving secondary metadata...\n");
    boolean update = 1;
    boolean cascade = 1;
    //taudb_add_secondary_metadata_to_trial(taudb_trial, &(sm[0]));
    //taudb_save_secondary_metadata(connection, taudb_trial, update);
    taudb_save_trial(connection, taudb_trial, update, cascade);
    fprintf(stderr, "Secondary metadata saving complete.\n");
}
