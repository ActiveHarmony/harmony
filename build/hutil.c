/*
 * Copyright 2003-2011 Jeffrey K. Hollingsworth
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

#include "hutil.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>

/***
 *
 * Exit and send a mesage to stderr
 *
 ***/
void h_exit(const char *errmesg)
{
    perror(errmesg);
    exit(1);
}

char *search_path(const char *filename, mode_t and_bits, mode_t or_bits)
{
    static char fullpath[FILENAME_MAX];
    struct stat sb;
    char *path, *pend;

    path = getenv("PATH");
    if (path == NULL)
        return NULL;

    while (path != NULL) {
        pend = strchr(path, ':');
        if (pend == NULL)
            pend = path + strlen(path);

        snprintf(fullpath, sizeof(fullpath), "%.*s/%s",
                 (int)(pend - path), path, filename);

        if (stat(fullpath, &sb) == 0)
            if ((sb.st_mode & and_bits) == and_bits &&
                (sb.st_mode & or_bits) != 0)
            {
                return fullpath;
            }

        if (*pend == '\0')
            path = NULL;
        else
            path = pend + 1;
    }
    return NULL;
}

int get_cpu_info(char *cpu_vendor, char *cpu_model, char *cpu_freq, char *cache_size) {
    FILE *cpuinfo;
    int core_num;
    bool recorded_vendor;
    bool recorded_model;
    bool recorded_freq;
    bool recorded_cache;

    char line_str[256];
    char ele_name[128];
    char ele_val[128];
    
    core_num = 0;
    recorded_vendor = recorded_model = recorded_freq = recorded_cache = false;

    /*Open cpuinfo in /proc/cpuinfo*/
    cpuinfo = fopen("/proc/cpuinfo", "r");
    if (cpuinfo == NULL) {
	fprintf(stderr, "Error occurs when acquire cpu information.\nLog file will not include CPU info.\n");
	return -1;
    } else {
	printf("CPU info file opened!\n");
    }

    while(!feof(cpuinfo)) {
	fgets(line_str, 512, cpuinfo);

	if (strlen(line_str) <= 1)
	    continue;
	sscanf(line_str, "%[^\t:] : %[^\t:]", ele_name, ele_val);

	if (!strcmp(ele_name, "processor")) {
	    core_num++;
	} else if (!strcmp(ele_name, "vendor_id") && recorded_vendor == false) {
	    strcpy(cpu_vendor, ele_val);
	    recorded_vendor = true;
	} else if (!strcmp(ele_name, "model name") && recorded_model == false) {
	    strcpy(cpu_model, ele_val);
	    recorded_model = true;
	} else if (!strcmp(ele_name, "cpu MHz") && recorded_freq == false) {
	    strcpy(cpu_freq, ele_val);
	    recorded_freq = true;
	} else if (!strcmp(ele_name, "cache size") && recorded_cache == false) {
	    strcpy(cache_size, ele_val);
	    recorded_cache = true;
	}
    }

    printf("Core num is %d\n", core_num);

    return core_num;

}
