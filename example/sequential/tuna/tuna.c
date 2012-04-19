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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

#include "hclient.h"
#include "hmesgs.h"

typedef enum method_t {
    METHOD_WALL,
    METHOD_USER,
    METHOD_SYS,
    METHOD_OUTPUT,

    METHOD_MAX
} method_t;

void usage(const char *);
void parseArgs(int, char **);
int handle_int(int, char *);
int handle_real(int, char *);
int handle_enum(int, char *);
int handle_method(char *);
int prepare_client_argv();
FILE *tuna_popen(const char *, char **, pid_t *);
double tv_to_double(struct timeval *tv);

hdesc_t *hdesc = NULL;
method_t method = METHOD_WALL;
unsigned int max_loop = -1;
unsigned int quiet = 0;
unsigned int verbose = 0;

#define MAX_PARAM 64
unsigned int param_count = 0;
hval_type param_type[MAX_PARAM];
void *param_data[MAX_PARAM];

int client_argc_template, client_argc;
char **client_argv_template, **client_argv;
char *client_argv_buf, *client_argv_bufend;

int main(int argc, char **argv)
{
    int i, hresult, line_start, count;
    char readbuf[4096];
    FILE *fptr;
    double perf = 0.0;

    struct timeval wall_start, wall_end, wall_time;

    pid_t pid, w_res;
    int client_status;
    struct rusage client_usage;

    if (argc < 2) {
        usage(argv[0]);
        return -1;
    }

    /* Initialize the Harmony descriptor */
    hdesc = harmony_init("tuna", HARMONY_IO_POLL);
    if (hdesc == NULL) {
        fprintf(stderr, "Failed to initialize a Harmony descriptor.\n");
        return -1;
    }

    /* Parse the command line arguments. */
    parseArgs(argc, argv);

    /* Connect to Harmony server and register ourselves as a client. */
    printf("Starting Harmony...\n");
    if (harmony_connect(hdesc, NULL, 0) < 0) {
        fprintf(stderr, "Could not connect to harmony server.\n");
        return -1;
    }

    for (i = 0; max_loop < 0 || i < max_loop; ++i) {
        hresult = harmony_fetch(hdesc);
        if (hresult < 0) {
            fprintf(stderr, "Failed to fetch values from server.\n");
            return -1;
        }
        else if (hresult > 0) {
            /* The Harmony system modified the variable values. */
            prepare_client_argv();
        }

        if (gettimeofday(&wall_start, NULL) != 0) {
            perror("Error on gettimeofday()");
            return -1;
        }

        fptr = tuna_popen(client_argv[0], client_argv, &pid);
        if (!fptr) {
            return -1;
        }

        count = 0;
        line_start = 1;
        while (fgets(readbuf, sizeof(readbuf), fptr)) {
            if (line_start) sscanf(readbuf, "%lf", &perf);
            if (!quiet) printf("%s", readbuf);
            line_start = (strchr(readbuf, '\n') != NULL);
            ++count;
        }
        fclose(fptr);

        if (verbose)
            printf("Called fgets() %d times before fclose()\n", count);

        do {
            w_res = wait4(pid, &client_status, 0, &client_usage);
            if (w_res < 0) {
                perror("Error on wait4()");
                return -1;
            }
        } while (w_res == 0);

        if (gettimeofday(&wall_end, NULL) != 0) {
            perror("Error on gettimeofday()");
            return -1;
        }
        timersub(&wall_end, &wall_start, &wall_time);

        switch (method) {
        case METHOD_WALL:   perf = tv_to_double(&wall_time); break;
        case METHOD_USER:   perf = tv_to_double(&client_usage.ru_utime); break;
        case METHOD_SYS:    perf = tv_to_double(&client_usage.ru_stime); break;
        case METHOD_OUTPUT: break;
        default:
            assert(0 && "Unknown measurement method.");
        }

        /* Update the performance result */
        if (harmony_report(hdesc, perf) < 0) {
            fprintf(stderr, "Failed to report performance to server.\n");
            return -1;
        }

        if (harmony_converged(hdesc))
            break;
    }

    /* Close the session */
    if (harmony_disconnect(hdesc) < 0) {
        fprintf(stderr, "Failed to disconnect from harmony server.\n");
        return -1;
    }

    return 0;
}

void usage(const char *me)
{
    fprintf(stderr, "Usage: %s tunable_vars [options] prog [prog_args]\n", me);
    fprintf(stderr, "\n"
"  Tunes an application by modifying its input parameters.  The tunable\n"
"  variables are specified using parameters described in the \"Tunable\n"
"  Variable Description\" section below.  After all options, the program\n"
"  binary to launch should be provided.  Optionally, additional arguments\n"
"  may be provided to control how the variables should be supplied to the\n"
"  client application.  The format of this string is described in the\n"
"  \"Optional Argument String\" section below.\n"
"\n"
"Tunable Variable Description\n"
"  -i=name,min,max,step    Describe an integer number variable called\n"
"                            <name> where valid values fall between <min>\n"
"                            and <max> with strides of size <step>.\n"
"  -r=name,min,max,step    Describe a real number variable called <name>\n"
"                            where valid values fall between <min> and\n"
"                            <max> with strides of size <step>.\n"
"  -e=name,val_1,..,val_n  Describe an enumerated variable called <name>\n"
"                            whose values must be <val_1> or <val_2> or ..\n"
"                            or <val_n>.\n"
"\n"
"Options\n"
"  -m=<metric>             Calculate performance of child process using\n"
"                            one of the following metrics:\n"
"                              wall   = Wall time. (default)\n"
"                              user   = Reported user CPU time.\n"
"                              sys    = Reported system CPU time.\n"
"                              output = Read final line of child output.\n"
"  -q                      Suppress client application output.\n"
"  -v                      Print additional informational output.\n"
"  -n=<num>                Run child program at most <num> times.\n"
"\n"
"Controlling Program Arguments\n"
"  If the tunable variables cannot be supplied directly as arguments to\n"
"  the client application, then you must provide additional parameters to\n"
"  describe the format of the argument vector.  Each argument (starting with\n"
"  and including the program binary) may include a percent sign (%%) which\n"
"  will be replaced by values from the tuning server, in the order they were\n"
"  specified on the command line.  Backslashes (\\) should be used as the\n"
"  escape character.  For example:\n"
"\n"
"    %s -i=tile,1,10,1 -i=unroll,1,10,1 ./matrix_mult -t %% -u %%`\n\n", me);
}

void parseArgs(int argc, char **argv)
{
    int i, used;
    size_t bufsize;
    char *arg;

    for (i = 1; i < argc && *argv[i] == '-'; ++i) {
        arg = argv[i] + 1;
        while (*arg != '\0') {
            switch (*arg) {
            case 'i': if (handle_int(i, arg)  != 0) exit(-1); break;
            case 'r': if (handle_real(i, arg) != 0) exit(-1); break;
            case 'e': if (handle_enum(i, arg) != 0) exit(-1); break;
            case 'm': if (handle_method(arg)  != 0) exit(-1); break;
            case 'q': quiet = 1; break;
            case 'v': verbose = 1; break;
            case 'n':
                errno = 0;
                max_loop = strtoul(arg, &arg, 0);
                if (errno != 0) {
                    fprintf(stderr, "Invalid -n value.\n");
                    exit(-1);
                }
                if (*arg != '\0') {
                    fprintf(stderr, "Trailing characters after n flag\n");
                    exit(-1);
                }
                break;
            default:
                fprintf(stderr, "Unknown flag: -%c\n", *arg);
                exit(-1);
            }
            if (*arg == 'q' || *arg == 'v')
                ++arg;
            else break;
        }
    }
    client_argc_template = argc - i;
    client_argv_template = &argv[i];

    /* Space for the tuned values */
    bufsize = param_count * MAX_VAL_STRLEN;

    /* Space for additional character strings. */
    used = 0;
    client_argc = 0;
    while (i < argc) {
        ++client_argc;
        for (arg = argv[i]; *arg != '\0'; ++arg) {
            if (*arg == '%') ++used;
            else if (*arg == '\\') ++arg;
            ++bufsize;
        }
        ++bufsize;
        ++i;
    }
    if (used > param_count) {
        fprintf(stderr, "Variables requested exceeds variables defined.\n");
        exit(-1);
    }

    /* Space for the argv array. */
    client_argc += param_count - used;
    bufsize += sizeof(char *) * (client_argc + 1);

    client_argv = (char **)malloc(bufsize);
    if (client_argv == NULL) {
        fprintf(stderr, "Error allocating memory for parameters.\n");
        exit(-1);
    }
    client_argv_buf = (char *)(&client_argv[client_argc + 1]);
    client_argv_bufend = ((char *)client_argv) + bufsize;
}

int handle_int(int i, char *arg)
{
    char *name;
    int i_min, i_max, i_step;
    void *data;

    assert(*arg == 'i');

    ++arg;
    if (*arg == '=') {
        ++arg;
    }
    name = arg;

    while (*arg != ',' && *arg != '\0') ++arg;
    if (*arg != ',') {
        fprintf(stderr, "Invalid description in parameter %d.\n", i);
        return -1;
    }
    *(arg++) = '\0';

    errno = 0;
    i_min = strtol(arg, &arg, 0);
    if (errno != 0) {
        fprintf(stderr, "Invalid min range value for variable '%s'.\n", name);
        return -1;
    }
    ++arg;

    errno = 0;
    i_max = strtol(arg, &arg, 0);
    if (errno != 0) {
        fprintf(stderr, "Invalid max range value for variable '%s'.\n", name);
        return -1;
    }
    ++arg;

    errno = 0;
    i_step = strtol(arg, &arg, 0);
    if (errno != 0) {
        fprintf(stderr, "Invalid step value for variable '%s'.\n", name);
        return -1;
    }

    if (*arg != '\0') {
        fprintf(stderr, "Trailing characters in definition of '%s'.\n", name);
        return -1;
    }
    
    data = malloc(sizeof(long));
    if (!data) {
        fprintf(stderr, "Error allocating memory for variable '%s'.\n", name);
        return -1;
    }

    if (harmony_register_int(hdesc, name, data, i_min, i_max, i_step) < 0) {
        fprintf(stderr, "Error registering variable '%s'.\n", name);
        return -1;
    }

    if (param_count >= MAX_PARAM) {
        fprintf(stderr, "Maximum number of tunable parameters"
                " exceeded (%d).\n", MAX_PARAM);
        return -1;
    }

    param_type[param_count] = HVAL_INT;
    param_data[param_count] = data;
    ++param_count;

    return 0;
}

int handle_real(int i, char *arg)
{
    char *name;
    int r_min, r_max, r_step;
    void *data;

    assert(*arg == 'r');

    ++arg;
    if (*arg == '=') {
        ++arg;
    }
    name = arg;

    while (*arg != ',' && *arg != '\0') ++arg;
    if (*arg != ',') {
        fprintf(stderr, "Invalid description in parameter %d.\n", i);
        return -1;
    }
    *(arg++) = '\0';

    errno = 0;
    r_min = strtod(arg, &arg);
    if (errno != 0) {
        fprintf(stderr, "Invalid min range value for variable '%s'.\n", name);
        return -1;
    }
    ++arg;

    errno = 0;
    r_max = strtod(arg, &arg);
    if (errno != 0) {
        fprintf(stderr, "Invalid max range value for variable '%s'.\n", name);
        return -1;
    }
    ++arg;

    errno = 0;
    r_step = strtod(arg, &arg);
    if (errno != 0) {
        fprintf(stderr, "Invalid step value for variable '%s'.\n", name);
        return -1;
    }

    if (*arg != '\0') {
        fprintf(stderr, "Trailing characters in definition of '%s'.\n", name);
        return -1;
    }
    
    data = malloc(sizeof(double));
    if (!data) {
        fprintf(stderr, "Error allocating memory for variable '%s'.\n", name);
        return -1;
    }

    if (harmony_register_real(hdesc, name, data, r_min, r_max, r_step) < 0) {
        fprintf(stderr, "Error registering variable '%s'.\n", name);
        return -1;
    }

    if (param_count >= MAX_PARAM) {
        fprintf(stderr, "Maximum number of tunable parameters"
                " exceeded (%d).\n", MAX_PARAM);
        return -1;
    }

    param_type[param_count] = HVAL_REAL;
    param_data[param_count] = data;
    ++param_count;

    return 0;
}

int handle_enum(int i, char *arg)
{
    char *name, *val;
    void *data;
 
    assert(*arg == 'e');

    ++arg;
    if (*arg == '=') {
        ++arg;
    }
    name = arg;

    while (*arg != ',' && *arg != '\0') ++arg;
    if (*arg != ',') {
        fprintf(stderr, "Invalid description in parameter %d.\n", i);
        return -1;
    }
    *(arg++) = '\0';

    data = malloc(sizeof(char *));
    if (!data) {
        fprintf(stderr, "Error allocating memory for variable '%s'.\n", name);
        return -1;
    }

    if (harmony_register_enum(hdesc, name, data) < 0) {
        fprintf(stderr, "Error registering variable '%s'.\n", name);
        return -1;
    }

    while (*arg != '\0') {
        val = arg;
        while (*arg != ',' && *arg != '\0') ++arg;
        if (*arg != '\0')
            *(arg++) = '\0';

        if (harmony_range_enum(hdesc, name, val) < 0) {
            fprintf(stderr, "Invalid value string for variable '%s'.\n", name);
            return -1;
        }
    }

    if (param_count >= MAX_PARAM) {
        fprintf(stderr, "Maximum number of tunable parameters"
                " exceeded (%d).\n", MAX_PARAM);
        return -1;
    }

    param_type[param_count] = HVAL_STR;
    param_data[param_count] = data;
    ++param_count;

    return 0;
}

int handle_method(char *arg)
{
    assert(*arg == 'm');
    ++arg;
    if (*arg == '=')
        ++arg;

    if (strcmp("wall", arg) == 0)        method = METHOD_WALL;
    else if (strcmp("user", arg) == 0)   method = METHOD_USER;
    else if (strcmp("sys", arg) == 0)    method = METHOD_SYS;
    else if (strcmp("output", arg) == 0) method = METHOD_OUTPUT;
    else {
        fprintf(stderr, "Unknown method choice.\n");
        return -1;
    }

    return 0;
}

int prepare_client_argv()
{
    unsigned int i, used;
    int addlen;
    char *arg, *ptr;

    used = 0;
    ptr = client_argv_buf;
    for (i = 0; i < client_argc; ++i) {
        client_argv[i] = ptr;

        if (i < client_argc_template)
            arg = client_argv_template[i];
        else
            arg = "%";

        while (*arg != '\0') {
            if (*arg == '%') {
                assert(used < param_count);
                switch(param_type[used]) {
                case HVAL_INT:
                    addlen = sprintf(ptr, "%ld", *(long *)param_data[used]);
                    break;
                case HVAL_REAL:
                    addlen = sprintf(ptr, "%lf", *(double *)param_data[used]);
                    break;
                case HVAL_STR:
                    addlen = sprintf(ptr, "%s", *(char **)param_data[used]);
                    break;
                default:
                    assert(0 && "Invalid parameter type.");
                }
                if (addlen < 0) {
                    perror("Error during sprintf()");
                    return -1;
                }
                ptr += addlen;
                ++used;
            }
            else {
                if (*arg == '\\')
                    ++arg;
                *(ptr++) = *arg;
            }
            ++arg;
        }
        *(ptr++) = '\0';
    }
    client_argv[i] = NULL;

    assert(ptr < client_argv_bufend);
    return 0;
}

FILE *tuna_popen(const char *prog, char **argv, pid_t *pid)
{
    int i, pipefd[2];
    FILE *fptr;

    if (pipe(pipefd) != 0) {
        perror("Could not create pipe");
        return NULL;
    }

    if (verbose) {
        printf("Launching %s", prog);
        for (i = 1; argv[i] != NULL; ++i) {
            printf(" %s", argv[i]);
        }
        printf("\n");
    }

    *pid = fork();
    if (*pid == 0) {
        /* Child Case */
        close(pipefd[0]); /* Close (historically) read side of pipe. */

        if (dup2(pipefd[1], STDOUT_FILENO) < 0 ||
            dup2(pipefd[1], STDERR_FILENO) < 0)
        {
            exit(-1);
        }

        execv(prog, argv);
        exit(-2);
    }
    else if (*pid < 0) {
        perror("Error on fork()");
        return NULL;
    }
    close(pipefd[1]); /* Close (historically) write side of pipe. */

    /* Convert raw socket to stream based FILE ptr. */
    fptr = fdopen(pipefd[0], "r");
    if (fptr == NULL) {
        perror("Cannot convert pipefd to FILE ptr");
        exit(-2);
    }

    return fptr;
}

double tv_to_double(struct timeval *tv)
{
    double retval = tv->tv_usec;
    retval /= 1000000;
    return retval + tv->tv_sec;
}
