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
#include <libgen.h>

#include "session-core.h"
#include "hsession.h"
#include "hcfg.h"
#include "defaults.h"
#include "hmesg.h"
#include "hsockutil.h"
#include "hutil.h"

static int verbose_flag, list_flag, home_flag;

/* Function Prototypes */
int   parse_opts(int argc, char *argv[]);
char *find_harmony_home(const char *progname);
int   is_valid_harmony_home(const char *dir);
int   search_libexec(const char *home);
void  add_if_layer(void *handle, const char *filename);
void  add_if_strategy(void *handle, const char *filename);
int   qsort_strcmp(const void *a, const void *b);

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

int main(int argc, char *argv[])
{
    int i;
    char *home;

    if (parse_opts(argc, argv) != 0)
        return -1;

    home = find_harmony_home(argv[0]);
    //if (verbose_flag || home_flag)
    printf("Harmony home: %s\n", home);

    if (list_flag) {
        if (search_libexec(home) != 0)
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

    free(home);
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

/*
 * Determine the location of the Active Harmony installation directory.
 */
char *find_harmony_home(const char *argv0)
{
    char *retval;
    int home_from_env = 0;

    /* First check HARMONY_HOME environment variable. */
    retval = getenv("HARMONY_HOME");
    if (retval) {
        if (verbose_flag || home_flag)
            printf("HARMONY_HOME is set.\n");

        retval = stralloc(retval);
        if (!retval) {
            perror("Could not allocate memory for home path");
            exit(-1);
        }
        home_from_env = 1;
    }
    /* See if program invocation specified a path. */
    else if (strchr(argv0, '/')) {
        retval = sprintf_alloc("%s   ", argv0); /* Allocate 3 extra chars.*/
        if (!retval) {
            perror("Could not allocate memory for home path");
            exit(-1);
        }
        retval = dirname(retval);
        strcat(retval, "/..");
    }
    /* As a last resort, search the PATH environment variable. */
    else {
        char *dirpath;
        char *tmpbuf = stralloc(argv0);

        if (!tmpbuf) {
            perror("Could not allocate temporary memory for program name");
            exit(-1);
        }

        dirpath = search_path( basename(tmpbuf) );
        if (!dirpath) {
            fprintf(stderr, "Could not find HARMONY_HOME\n");
            exit(-1);
        }

        retval = sprintf_alloc("%s/..", dirname(dirpath));
        if (!retval) {
            perror("Could not allocate memory for home path");
            exit(-1);
        }
        free(tmpbuf);
    }

    if (!is_valid_harmony_home(retval)) {
        if (home_from_env) {
            fprintf(stderr, "HARMONY_HOME (\"%s\") does not refer to a valid"
                    " Active Harmony installation directory.\n", retval);
        }
        else {
            fprintf(stderr, "%s is not within a valid Active Harmony"
                    " installation directory.\n", argv0);
        }
        exit(-1);
    }

    return retval;
}

int is_valid_harmony_home(const char *dir)
{
    static const char *home_file[] = {
        /* Not a complete list.  Just enough to move forward confidently. */
        "bin/hinfo",
        "bin/tuna",
        "libexec/random.so",
        NULL
    };
    int i, valid = 1;
    char *tmpbuf;

    for (i = 0; valid && home_file[i]; ++i) {
        tmpbuf = sprintf_alloc("%s/%s", dir, home_file[i]);
        if (!tmpbuf) {
            perror("Could not allocate memory for file path");
            exit(-1);
        }

        if (!file_exists(tmpbuf))
            valid = 0;

        free(tmpbuf);
    }
    return valid;
}

int search_libexec(const char *home)
{
    int retval = 0;
    char *fname;
    DIR *dp;

    /* Prepare a DIR pointer for the libexec directory. */
    fname = sprintf_alloc("%s/libexec", home);
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
        fname = sprintf_alloc("%s/libexec/%s", home, entry->d_name);
        if (!fname) {
            perror("Could not build file path string");
            exit(-1);
        }

        /* Request file metadata. */
        if (stat(fname, &finfo) != 0) {
            if (verbose_flag)
                printf("Warning: Could not stat %s: %s\n",
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
                    perror("Warning: Could not close dynamic library");
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
