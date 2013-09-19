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

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/utsname.h>
#include <stdbool.h>

#include "hclient.h"
#include "defaults.h"

#define MAX_STR_LEN 1024

/* For illustration purposes, the performance here is defined by following
 * simple definition:
 *   perf = (p1 - 15)^2 + (p2 - 30)^2 + (p3 - 45)^2 +
 *          (p4 - 60)^2 + (p5 - 75)^2 + (p6 - 90)^2
 *
 * So the theoretical minimum can be found at point:
 *      (15, 30, 45, 60, 75, 90)
 *
 * And a reasonable search range for all parameters is [1-100].
 *
 */
long application(long p1, long p2, long p3, long p4, long p5, long p6)
{
    long perf =
        (p1-15) * (p1-15) +
        (p2-30) * (p2-30) +
        (p3-45) * (p3-45) +
        (p4-60) * (p4-60) +
        (p5-75) * (p5-75) +
        (p6-90) * (p6-90);
    return perf;
}

int get_cpu_info(char *cpu_vendor, char *cpu_model,
                 char *cpu_freq, char *cache_size)
{
    FILE *cpuinfo;
    int core_num;
    bool recorded_vendor;
    bool recorded_model;
    bool recorded_freq;
    bool recorded_cache;

    char line_str[512];
    char ele_name[128];
    char ele_val[128];

    core_num = 0;
    recorded_vendor = recorded_model = recorded_freq = recorded_cache = false;

    /*Open cpuinfo in /proc/cpuinfo*/
    cpuinfo = fopen("/proc/cpuinfo", "r");
    if (cpuinfo == NULL) {
        fprintf(stderr, "Error occurs when acquire cpu information.\n");
        fprintf(stderr, "Log file will not include CPU info.\n");
        return -1;
    }
    else {
        printf("CPU info file opened!\n");
    }

    while (!feof(cpuinfo)) {
        fgets(line_str, sizeof(line_str), cpuinfo);

        if (strlen(line_str) <= 1)
            continue;
        sscanf(line_str, "%[^\t:] : %[^\t:\n]", ele_name, ele_val);

        if (!strcmp(ele_name, "processor")) {
            core_num++;
        }
        else if (!strcmp(ele_name, "vendor_id") && recorded_vendor == false) {
            strcpy(cpu_vendor, ele_val);
            recorded_vendor = true;
        }
        else if (!strcmp(ele_name, "model name") && recorded_model == false) {
            strcpy(cpu_model, ele_val);
            recorded_model = true;
        }
        else if (!strcmp(ele_name, "cpu MHz") && recorded_freq == false) {
            strcpy(cpu_freq, ele_val);
            recorded_freq = true;
        }
        else if (!strcmp(ele_name, "cache size") && recorded_cache == false) {
            strcpy(cache_size, ele_val);
            recorded_cache = true;
        }
    }

    printf("Core num is %d\n", core_num);
    return core_num;
}

char *get_metadata(void)
{
    struct utsname uts; //os info

    int core_num; //number of cpu cores
    char cpu_vendor[32];
    char cpu_model[32];
    char cpu_freq[32];
    char cache_size[32];

    char *retval;

    retval = (char*)malloc(sizeof(char)*MAX_STR_LEN);
    if (uname(&uts) < 0)
        perror("uname() error\n");

    if ( (core_num = get_cpu_info(cpu_vendor, cpu_model,
                                  cpu_freq, cache_size)) < 0)
    {
        fprintf(stderr, "Error getting CPU information!");
        return NULL;
    }

    snprintf(retval, MAX_STR_LEN, "%s$$%s$$%s$$%s$$%d$$%s$$%s$$%s$$%s",
             uts.nodename, uts.sysname, uts.release, uts.machine, core_num,
             cpu_vendor, cpu_model, cpu_freq, cache_size);

    return retval;
}

int main(int argc, char **argv)
{
    const char *name;
    char *ptr;
    hdesc_t *hdesc;
    int i, retval, loop = 200;
    long perf = -1000;
    char *metadata;

    /* Variables to hold the application's runtime tunable parameters.
     * Once bound to a Harmony tuning session, these variables will be
     * modified upon harmony_fetch() to a new testing configuration.
     */
    long param_1;
    long param_2;
    long param_3;
    long param_4;
    long param_5;
    long param_6;

    retval = 0;
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            fprintf(stderr, "Usage: %s [session_name]"
                    " [KEY_1=VAL_1] .. [KEY_N=VAL_N]\n\n", argv[0]);
            return 0;
        }
    }

    /* Initialize a Harmony client. */
    hdesc = harmony_init();
    if (hdesc == NULL) {
        fprintf(stderr, "Failed to initialize a harmony session.\n");
        return -1;
    }

    /* Set a unique id for ourselves */
    metadata = get_metadata();
    printf("Metadata is %s\n", metadata);
    if (harmony_id(hdesc, metadata) != 0) {
        perror("Error setting client id");
        return -1;
    }

    /* Process the program arguments. */
    i = 1;
    name = "TAUdb_example";
    if (argc > 1 && !strchr(argv[1], '=')) {
        name = argv[1];
        ++i;
    }

    /* TAUDB_STORE_METHOD can be "real_time" or any number
     *
     * "one_time": First "TAUDB_STORE_NUM" number of data will be
     * loaded at the end
     *
     * "real_time": data will be loaded at real time, but can't use
     * paraprof or perfexplorer to visualize the data, storing
     * interval is "TAUDB_STORE_NUM" number of data per save (to
     * reduce the overhead for taudb_save_trial())
     */
    errno = 0;
    harmony_session_name(hdesc, name);
    harmony_setcfg(hdesc, CFGKEY_SESSION_LAYERS, "TAUdb.so");
    harmony_setcfg(hdesc, CFGKEY_CLIENT_COUNT, "1");
    harmony_setcfg(hdesc, "TAUDB_STORE_METHOD", "one_time");
    harmony_setcfg(hdesc, "TAUDB_STORE_NUM", "150");
    if (errno) {
        perror("Error during session setup");
        return -1;
    }

    while (i < argc) {
        ptr = strchr(argv[i], '=');
        if (!ptr) {
            fprintf(stderr, "Invalid parameter '%s'\n", argv[i]);
            return -1;
        }

        errno = 0;
        *(ptr++) = '\0';
        harmony_setcfg(hdesc, argv[i], ptr);
        if (errno) {
            fprintf(stderr, "Failed to set config var %s\n", argv[i]);
            return -1;
        }
        ++i;
    }

    if (harmony_int(hdesc, "param_1", 1, 100, 1) < 0 ||
        harmony_int(hdesc, "param_2", 1, 100, 1) < 0 ||
        harmony_int(hdesc, "param_3", 1, 100, 1) < 0 ||
        harmony_int(hdesc, "param_4", 1, 100, 1) < 0 ||
        harmony_int(hdesc, "param_5", 1, 100, 1) < 0 ||
        harmony_int(hdesc, "param_6", 1, 100, 1) < 0)
    {
        fprintf(stderr, "Failed to define tuning session\n");
        return -1;
    }

    printf("Launching tuning session.\n");
    if (harmony_launch(hdesc, NULL, 0) != 0) {
        fprintf(stderr, "Could not launch tuning session: %s\n",
                harmony_error_string(hdesc));
        return -1;
    }

    printf("Starting Harmony...\n");

    /* Bind the session variables to local variables. */
    if (harmony_bind_int(hdesc, "param_1", &param_1) < 0 ||
        harmony_bind_int(hdesc, "param_2", &param_2) < 0 ||
        harmony_bind_int(hdesc, "param_3", &param_3) < 0 ||
        harmony_bind_int(hdesc, "param_4", &param_4) < 0 ||
        harmony_bind_int(hdesc, "param_5", &param_5) < 0 ||
        harmony_bind_int(hdesc, "param_6", &param_6) < 0)
    {
        fprintf(stderr, "Failed to register variable\n");
        retval = -1;
        goto cleanup;
    }

    /* Join this client to the tuning session we established above. */
    if (harmony_join(hdesc, NULL, 0, name) < 0) {
        fprintf(stderr, "Could not connect to harmony server: %s\n",
                harmony_error_string(hdesc));
        retval = -1;
        goto cleanup;
    }
    printf("Connected to harmony server.\n");

    /* main loop */
    for (i = 0; !harmony_converged(hdesc) && i < loop; i++) {
        int hresult = harmony_fetch(hdesc);
        if (hresult < 0) {
            fprintf(stderr, "Failed to fetch values from server: %s\n",
                    harmony_error_string(hdesc));
            retval = -1;
            goto cleanup;
        }
        else if (hresult == 0) {
            /* New values were not available at this time.
             * Bundles remain unchanged by Harmony system.
             */
        }
        else if (hresult > 0) {
            /* The Harmony system modified the variable values.
             * Do any systemic updates to deal with such a change.
             */
        }

        /* Run one full iteration of the application (or code variant).
         *
         * Here our application is rather simple. Definition of performance can
         * be user-defined. Depending on application, it can be MFlops/sec,
         * time to complete the entire run of the application, cache hits vs.
         * misses and so on.
         *
         * For searching the parameter space in a Transformation framework,
         * just run different parameterized code variants here. A simple
         * mapping between the parameters and the code-variants is needed to
         * call the appropriate code variant.
         */

        perf = application(param_1, param_2, param_3,
                           param_4, param_5, param_6);

        if (hresult > 0) {
            /* Only print performance if new values were fetched. */
            printf("%ld, %ld, %ld, %ld, %ld, %ld = %ld\n",
                   param_1, param_2, param_3,
                   param_4, param_5, param_6, perf);
        }

        /* Report the performance we've just measured. */
        if (harmony_report(hdesc, perf) < 0) {
            fprintf(stderr, "Failed to report performance to server.\n");
            retval = -1;
            goto cleanup;
        }
    }

    /* Leave the session */
    if (harmony_leave(hdesc) < 0) {
        fprintf(stderr, "Failed to disconnect from harmony server.\n");
        retval = -1;
    }

  cleanup:
    harmony_fini(hdesc);
    return retval;
}
