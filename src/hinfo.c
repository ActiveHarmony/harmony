/*
 * Copyright 2003-2014 Jeffrey K. Hollingsworth
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
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <limits.h>

#include "session-core.h"
#include "hsession.h"
#include "hcfg.h"
#include "defaults.h"
#include "hmesg.h"
#include "hsockutil.h"
#include "hutil.h"

#define my_name "hinfo"

static int verbose_flag, list_flag, home_flag;

/* Function Prototypes */
int  parse_opts(int argc, char *argv[]);
int  search_libexec(const char *basedir);
void add_if_layer(void *handle, const char *filename);
void add_if_strategy(void *handle, const char *filename);
int  qsort_strcmp(const void *a, const void *b);

/* Global Variables */
char **layer;
int layer_len, layer_cap;
char **strat;
int strat_len, strat_cap;

void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "OPTIONS:\n"
"  -l, --list     List .so files (potential strategies and layers)\n"
"  -h, --home     Just verify presence of and print out libexec directory\n"
"  -v, --verbose  Print info about which hooks are present\n");
}

/* attempts to figure out where libexec dir is
 * possibly returns incorrect path
 * assumes that the calling program is 'hinfo'
 */
char *find_harmony_home(char *argv_str)
{
    char *str = NULL;      /* used for temporary strings. I modify strings
                              to parse them so yeah */
    char *cur_path = NULL; /* for current path being looked at in third case */
    DIR *cur_dir = NULL;   /* look up one line ^ */
    struct dirent *entry;  /* current file in path being searched (third
                              case) */
    int colon_count = 0;   /* what you think it is */
    int i, c;              /* one letter vars for loop stuff */
    int *path_indices;     /* array of ints indicating beginnings of
                              individual paths */

    /* first check environment variable */
    char *home = getenv("HARMONY_HOME");
    if (home != NULL) {
        if (verbose_flag || home_flag)
            printf("HARMONY_HOME is set.\n");
        str = malloc(strlen(home) + 8 + 1); /* home + "/libexec" + \0 */
        strcpy(str, home);
        return str;
    }
    /* see if argv has path in it */
    else if (strncmp(argv_str, "hinfo", 5) != 0) {
        printf("HARMONY_HOME not set."
               " Using path hinfo was called with to locate HARMONY_HOME\n");
        str = malloc(strlen(argv_str) + 7 + 1);  /* ...hinfo, replace hinfo
                                                    with "/../libexec" */
        strcpy(str, argv_str);
        strcpy(str + strlen(argv_str) - strlen(my_name), "/../");
        return str;
    }
    else { /* need to check path */
        char *path = getenv("PATH");
        printf("HARMONY_HOME not set."
               " Searching path and inferring HARMONY_HOME"
               " from hinfo's location\n");
        str = malloc(strlen(path) + 1);
        strcpy(str, path);
        cur_path = malloc(strlen(path) + 11 + 1); /* max single path +
                                                     "/../libexec" */

        /* count colons in path */
        for (i = 0;i < strlen(path);i++) {
            if (str[i] == ':') colon_count++;
        }
        /* now allocate memory for an array of ints to mark
           beginnings of individual paths */
        path_indices = malloc(sizeof(int) * (colon_count + 1));
        path_indices[0] = 0;
        for (i = 0, c = 1;i < strlen(path);i++) {
            if (str[i] == ':') {
                path_indices[c++] = i + 1;
                str[i] = '\0';
            }
        }
        /* search each possibility for hinfo */
        for (i = 0;i < colon_count + 1;i++) {
            strcpy(cur_path, str + path_indices[i]);
            cur_dir = opendir(cur_path);
            if (cur_dir != NULL) {
                /* read every directory entry looking for "hinfo" */
                while (1) {
                    entry = readdir(cur_dir);
                    if (entry == NULL) break;
                    if (strncmp(entry->d_name, my_name, 256) == 0) {
                        strcpy(cur_path + strlen(cur_path), "/../");
                        free(str);
                        free(path_indices);
                        return cur_path;
                    }
                }
            }
        }
    }
    if (path_indices != NULL) free(path_indices);
    if (cur_path != NULL) free(cur_path);
    if (str != NULL) free(str);
    return NULL; /* no hinfo found in PATH??? */
}

int main(int argc, char *argv[])
{
    int i;
    char *harmony_home_path = NULL;
    char real_path[PATH_MAX];

    if (parse_opts(argc, argv) != 0)
        return -1;

    harmony_home_path = find_harmony_home(argv[0]);

    if (harmony_home_path == NULL) {
        printf("Unable to locate libexec path\n");
        return -1;
    }

    realpath(harmony_home_path, real_path);

    //if (verbose_flag || home_flag)
    printf("Harmony home: %s\n", real_path);

    if (list_flag) {
        if (search_libexec(real_path) != 0)
            return -1;

        printf("Valid processing layers:\n");
        for (i = 0; i < layer_len; ++i)
            printf("\t%s\n", layer[i]);
        printf("\n");

        printf("Valid strategies:\n");
        for (i = 0; i < strat_len; ++i)
            printf("\t%s\n", strat[i]);
        printf("\n");
    }

    if (harmony_home_path != NULL) free(harmony_home_path);
    return 0;
}

int parse_opts(int argc, char *argv[])
{
    int c;
    static struct option long_options[] = {
        {"list",    0, &list_flag,    'l'},
        {"home",    0, &home_flag,    'h'},
        {"verbose", 0, &verbose_flag, 'v'},
        {NULL, 0, NULL, 0}
    };

    while (1) {
        c = getopt_long(argc, argv, "lhv", long_options, NULL);
        if (c == -1)
            break;

        switch(c) {
        case 'l': list_flag = 1; break;
        case 'h': home_flag = 1; break;
        case 'v': verbose_flag = 1; break;
        default:
            continue;
        }
    }

    if (argc < 2 || (!list_flag && !home_flag)) {
        usage(argv[0]);
        fprintf(stderr, "\nNo operation requested.\n");
        return -1;
    }

    return 0;
}

int search_libexec(const char *basedir)
{
    int retval = 0;
    char *fname;
    DIR *dp;

    /* Prepare a DIR pointer for the libexec directory. */
    fname = sprintf_alloc("%s/libexec", basedir);
    if (!fname) {
        perror("Could not build libexec path string");
        exit(-1);
    }

    dp = opendir(fname);
    free(fname);
    if (!dp) {
        perror("Could not open Harmony's libexec directory");
        goto error;
    }

    while (1) {
        struct dirent *entry;
        struct stat finfo;
        void *handle;

        errno = 0;
        entry = readdir(dp);
        if (entry == NULL) {
            if (errno) {
                perror("Could not retrieve directory entry");
                goto error;
            }
            break;
        }

        /* Build the file path string. */
        fname = sprintf_alloc("%s/libexec/%s", basedir, entry->d_name);
        if (!fname) {
            perror("Could not build file path string");
            exit(-1);
        }

        /* Request file metadata. */
        if (stat(fname, &finfo) != 0) {
            if (verbose_flag)
                printf("Warning - Could not stat %s: %s\n",
                       entry->d_name, strerror(errno));
            memset(&finfo, 0, sizeof(finfo));
        }

        /* Only consider regular files. */
        if (S_ISREG(finfo.st_mode)) {
            handle = dlopen(fname, RTLD_LAZY | RTLD_LOCAL);
            if (handle) {
                add_if_layer(handle, entry->d_name);
                add_if_strategy(handle, entry->d_name);

                if (dlclose(handle) != 0)
                    perror("Warning - Could not close dynamic library");
            }
        }
        free(fname);
    }

    qsort(layer, layer_len, sizeof(char *), qsort_strcmp);
    qsort(strat, strat_len, sizeof(char *), qsort_strcmp);
    goto cleanup;

  error:
    retval = -1;

  cleanup:
    if (dp && closedir(dp) != 0) {
        perror("Could not close DIR pointer");
        retval = -1;
    }

    return retval;
}

const char *layer_required[] = {
    "join",
    "generate",
    "analyze",
    "getcfg",
    "setcfg",
    NULL
};

const char *layer_valid[] = {
    "init",
    "fini",
    NULL
};

/* Check if dynamic library handle is a processing layer plug-in.
 * If so, add an entry to the layer list.
 */
void add_if_layer(void *handle, const char *filename)
{
    int i, found = 0;
    char *prefix, *fname;

    /* Plugin layers must define a layer name. */
    prefix = (char *) dlsym(handle, "harmony_layer_name");
    if (!prefix)
        return;

    for (i = 0; layer_required[i] && !found; ++i) {
        fname = sprintf_alloc("%s_%s", prefix, layer_required[i]);
        if (!fname) {
            perror("Could not build layer function name");
            return;
        }

        if (dlsym(handle, fname) != NULL)
            found = 1;

        free(fname);
    }

    if (found) {
        if (layer_len == layer_cap) {
            if (array_grow(&layer, &layer_cap, sizeof(char *)) != 0) {
                perror("Could not grow layer list");
                exit(-1);
            }
        }
        layer[layer_len] = sprintf_alloc("%s (%s)", prefix, filename);
        if (!layer[layer_len]) {
            perror("Could not allocate memory for layer list entry");
            exit(-1);
        }
        ++layer_len;
    }
}

const char *strat_required[] = {
    "strategy_generate",
    "strategy_rejected",
    "strategy_analyze",
    "strategy_best",
    NULL
};

const char *strat_valid[] = {
    "strategy_init",
    "strategy_join",
    "strategy_getcfg",
    "strategy_setcfg",
    "strategy_fini",
    NULL
};

/* Check if dynamic library handle is a strategy plug-in.
 * If so, add an entry to the strategy list.
 */
void add_if_strategy(void *handle, const char *filename)
{
    int i, found = 1;

    for (i = 0; strat_required[i] && found; ++i) {
        if (dlsym(handle, strat_required[i]) == NULL)
            found = 0;
    }

    if (found) {
        if (strat_len == strat_cap) {
            if (array_grow(&strat, &strat_cap, sizeof(char *)) != 0) {
                perror("Could not grow strategy list");
                exit(-1);
            }
        }
        strat[strat_len] = stralloc(filename);
        if (!strat[strat_len]) {
            perror("Could not allocate memory for strategy list entry");
            exit(-1);
        }
        ++strat_len;
    }
}

int qsort_strcmp(const void *a, const void *b)
{
    char * const *_a = a;
    char * const *_b = b;
    return strcmp(*_a, *_b);
}
