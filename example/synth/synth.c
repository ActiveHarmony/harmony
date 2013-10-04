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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>

#include "testfunc.h"
#include "hclient.h"

#define MAX_ACCURACY 17

/* Function Prototypes */
int    parse_opts(int argc, char *argv[]);
int    parse_params(int idx, int argc, char *argv[]);
int    start_harmony(hdesc_t *hdesc);
double eval_func();
double quantize_value(double perf);
double random_value(double min, double max);
void   use_resources(double perf);

/* Option Variables (and their associated defaults). */
int accuracy = 6;
unsigned int seed;
int n;
unsigned int iterations;
int list_funcs;
int verbose;
double quantize;
double perturb;
char tuna_mode;

/* Global Variables */
finfo_t *finfo;
double *point;
double *options;
int single_eval;

void usage(const char *prog)
{
    fprintf(stdout, "Usage: %s [OPTIONS] <fname> <N> [x1..xN] [KEY=VAL ..]\n"
"\n"
"Test Active Harmony search strategies using multi-variate continuous\n"
"functions to simulate empirical test results.  Two parameters are\n"
"required: the name of a test function (funcname), and the\n"
"dimensionality of the search space (N).\n"
"\n"
"If N real values follow, they are considered to be a single point in\n"
"the search space to test.  The function is evaluated once, and the\n"
"program exits.  Otherwise, a tuning session is launched and the\n"
"function is tested repeatedly until the search session converges.\n"
"\n"
"Finally, any non-option parameter that contains an equals sign (=)\n"
"will be passed to the Active Harmony configuration system.\n"
"\n"
"OPTION SUMMARY\n"
"(Mandatory arguments to long options are also mandatory for short options.)\n"
"\n"
"  -a, --accuracy=(inf|INT) Allow INT decimal digits of accuracy to the\n"
"                            right of the decimal point for function input\n"
"                            variables.  If 'inf' is provided instead of an\n"
"                            integer, the full accuracy of an IEEE double\n"
"                            will be used. [Default: %d]\n"
"  -l, --list               List available test functions.  Full\n"
"                            descriptions will be printed when used with -v.\n"
"  -h, --help               Display this help message.\n"
"  -i, --iterations=INT     Perform at most INT search loop iterations.\n"
"  -o, --options=LIST       Pass a comma (,) separated list of real numbers\n"
"                            to the test function as optional parameters.\n"
"  -p, --perturb=REAL       Perturb the test function's output by a\n"
"                            uniformly random value in the range [0, REAL].\n"
"  -q, --quantize=INT       Quantize the underlying function by rounding\n"
"                            output values after INT decimal digits to the\n"
"                            right of the decimal point.\n"
"  -s, --seed=(time|INT)    Seed the random value generator with the\n"
"                            provided INT before perturbing output values.\n"
"                            If 'time' is provided instead of an integer,\n"
"                            the value will be taken from time(NULL).\n"
"                            [Default: time]\n"
"  -t, --tuna=TYPE          Run as a child of Tuna, where only a single\n"
"                            point is tested and the output is used to\n"
"                            consume a proportional amount of resources as\n"
"                            determined by TYPE:\n"
"                              w = Wall time.\n"
"                              u = User CPU time.\n"
"                              s = System CPU time.\n"
"                              o = Print value to stdout. (default)\n"
"  -v, --verbose            Verbose output.\n"
"\n", prog, accuracy);
}

int main(int argc, char *argv[])
{
    int index, i, retval, hresult, report;
    int width, maxwidth;
    double perf, best;
    hdesc_t *hdesc;

    hdesc = harmony_init(&argc, &argv);
    if (!hdesc) {
        perror("Error allocating/initializing a Harmony descriptor");
        return -1;
    }

    index = parse_opts(argc, argv);
    if (index < 0) {
        usage(argv[0]);
        return -1;
    }
    if (list_funcs) {
        flist_print(stdout, verbose);
        return 0;
    }
    parse_params(index, argc, argv);

    if (perturb)
        fprintf(stdout, "Seed value: %u\n", seed);
    srand(seed);

    if (single_eval) {
        perf = eval_func();
        if (!tuna_mode)
            tuna_mode = 'o';

        use_resources(perf);
        return 0;
    }

    if (start_harmony(hdesc) != 0) {
        retval = -1;
        goto cleanup;
    }

    report = 8;
    best = INFINITY;
    for (i = 0; !harmony_converged(hdesc); i++) {
        if (iterations && i >= iterations)
            break;

        hresult = harmony_fetch(hdesc);
        if (hresult < 0) {
            fprintf(stderr, "Error fetching values from session: %s\n",
                    harmony_error_string(hdesc));
            retval = -1;
            goto cleanup;
        }
        else if (hresult == 0) {
            continue;
        }

        perf = eval_func();
        if (perf < best)
            best = perf;

        if (harmony_report(hdesc, perf) < 0) {
            fprintf(stderr, "Error reporting performance to session: %s\n",
                    harmony_error_string(hdesc));
            retval = -1;
            goto cleanup;
        }

        if (i == report) {
            printf("       %6d evaluations, best value: %lf\n", i, best);
            report <<= 1;
        }
    }
    printf("Final: %6d evaluations, best value: %lf", i, best);
    if (finfo->optimal != -INFINITY)
        printf(" (Global optimal: %lf)\n", finfo->optimal);
    else
        printf(" (Global optimal: <Unknown>)\n");

    if (harmony_best(hdesc) != 0) {
        fprintf(stderr, "Error setting best input values: %s\n",
                harmony_error_string(hdesc));
        goto cleanup;
    }

    /* Initial pass through the point array to find maximum field width. */
    maxwidth = 0;
    for (i = 0; i < n; ++i) {
        width = snprintf(NULL, 0, "%lf", point[i]);
        if (maxwidth < width)
            maxwidth = width;
    }

    printf("\nBest point found:\n");
    for (i = 0; i < n; ++i)
        printf("x[%d] = %*lf\n", i, maxwidth, point[i]);

  cleanup:
    harmony_fini(hdesc);
    free(options);
    return retval;
}

int parse_opts(int argc, char *argv[])
{
    int c, q, count;
    char *end;
    static struct option longopts[] = {
        {"accuracy",   required_argument, NULL, 'a'},
        {"help",       no_argument,       NULL, 'h'},
        {"list",       no_argument,       NULL, 'l'},
        {"iterations", required_argument, NULL, 'i'},
        {"options",    required_argument, NULL, 'o'},
        {"perturb",    required_argument, NULL, 'p'},
        {"quantize",   required_argument, NULL, 'q'},
        {"seed",       required_argument, NULL, 's'},
        {"tuna",       required_argument, NULL, 't'},
        {"verbose",    no_argument,       NULL, 'v'},
        {NULL, 0, NULL, 0}
    };

    seed = time(NULL);
    while (1) {
        c = getopt_long(argc, argv, "a:hli:o:p:q:s:t:v", longopts, NULL);
        if (c == -1)
            break;

        switch (c) {
        case 'a':
            if (strcmp(optarg, "inf") == 0) {
                accuracy = MAX_ACCURACY + 1;
            }
            else {
                accuracy = strtol(optarg, &end, 0);
                if (*end != '\0') {
                    fprintf(stderr, "Invalid accuracy value '%s'.\n", optarg);
                    exit(-1);
                }
            }
            break;

        case 'h':
            return -1;

        case 'l':
            list_funcs = 1;
            break;

        case 'i':
            iterations = strtoul(optarg, &end, 0);
            if (*end != '\0') {
                fprintf(stderr, "Invalid iteration value '%s'.\n", optarg);
                exit(-1);
            }
            break;

        case 'o':
            count = 1;
            for (end = optarg; *end; ++end)
                if (*end == ',')
                    ++count;

            options = (double *) malloc(sizeof(double) * count);
            if (!options) {
                fprintf(stderr, "Could not allocated options array.\n");
                exit(-1);
            }

            end = optarg;
            for (count = 0; *end; ++count) {
                options[count] = strtod(end, &end);
                if (*end && *end != ',') {
                    fprintf(stderr, "Invalid option value '%s'.\n", optarg);
                    exit(-1);
                }
                if (*end == ',')
                    ++end;
            }
            break;

        case 'p':
            perturb = strtod(optarg, &end);
            if (*end != '\0' || perturb < 0) {
                fprintf(stderr, "Invalid perturbation value '%s'.\n", optarg);
                exit(-1);
            }
            break;

        case 'q':
            q = strtol(optarg, &end, 0);
            if (*end != '\0') {
                fprintf(stderr, "Invalid quantization value '%s'.\n", optarg);
                exit(-1);
            }
            quantize = pow(10.0, q);
            break;

        case 's':
            if (strcmp(optarg, "time") != 0) {
                seed = strtoul(optarg, &end, 0);
                if (*end != '\0') {
                    fprintf(stderr, "Invalid seed value '%s'.\n", optarg);
                    exit(-1);
                }
            }
            break;

        case 't':
            if (optarg[1] == '\0')
                tuna_mode = optarg[0];

            if (tuna_mode != 'w' && tuna_mode != 'u' &&
                tuna_mode != 's' && tuna_mode != 'o')
            {
                fprintf(stderr, "Invalid Tuna mode '%s'.\n", optarg);
                exit(-1);
            }
            break;

        case 'v':
            verbose = 1;
            break;

        default:
            exit(-1);
        }
    }
    return optind;
}

int parse_params(int idx, int argc, char *argv[])
{
    int i;
    char *end;

    if (idx < argc) {
        finfo = flist_find(argv[idx]);
        if (!finfo) {
            fprintf(stderr, "Invalid test function '%s'.\n"
                    "  Use `%s --list` to show available functions.\n",
                    argv[idx], argv[0]);
            exit(-1);
        }
        ++idx;
    }
    else {
        fprintf(stderr, "Missing test function name.\n"
                "  Use `%s --list` to show available functions.\n", argv[0]);
        exit(-1);
    }

    if (idx < argc) {
        n = strtol(argv[idx], &end, 0);
        if (*end != '\0') {
            fprintf(stderr, "Invalid dimension size '%s'.\n", optarg);
            exit(-1);
        }

        if (n < 2) {
            fprintf(stderr, "Dimension size must be >= 2.\n");
            exit(-1);
        }

        if (finfo->n_max && n > finfo->n_max) {
            fprintf(stderr, "Test function '%s' may have at most"
                    " %d dimensions.\n", finfo->name, finfo->n_max);
            exit(-1);
        }
        ++idx;
    }
    else {
        fprintf(stderr, "Search space dimensionality not specified.\n"
                "Use `%s --help` for usage information.\n", argv[0]);
        exit(-1);
    }

    point = (double *) malloc(sizeof(double) * n);
    if (!point) {
        fprintf(stderr, "Error allocating test point memory.\n");
        exit(-1);
    }

    if (tuna_mode || idx < argc)
        single_eval = 1;

    i = 0;
    for (; idx < argc; ++idx) {
        point[i] = strtod(argv[idx], &end);
        if (!end) {
            fprintf(stderr, "Invalid test point value '%s'.\n", argv[idx]);
            exit(-1);
        }
        ++i;
    }

    if (single_eval && i != n) {
        fprintf(stderr, "Expected %d values for test point. Got %d.\n", n, i);
        exit(-1);
    }

    return 0;
}

int start_harmony(hdesc_t *hdesc)
{
    char *session_name;
    char var_name[16];
    double step;
    int i, len;

    /* Build the session name. */
    len = snprintf(NULL, 0, "test_%s_%d.%d", finfo->name, n, getpid());
    session_name = (char *) malloc(len + 1);
    if (!session_name) {
        perror("Error allocating session name");
        goto error;
    }
    sprintf(session_name, "test_%s_%d.%d", finfo->name, n, getpid());

    if (harmony_session_name(hdesc, session_name) != 0) {
        fprintf(stderr, "Could not set session name: %s\n",
                harmony_error_string(hdesc));
        goto error;
    }

    if (accuracy <= MAX_ACCURACY)
        step = pow(10.0, -accuracy);
    else
        step = 0;

    for (i = 0; i < n; ++i) {
        snprintf(var_name, sizeof(var_name), "x_%d", i + 1);
        if (harmony_real(hdesc, var_name,
                         finfo->b_min, finfo->b_max, step) != 0)
        {
            fprintf(stderr, "Error defining '%s' tuning variable: %s\n",
                    var_name, harmony_error_string(hdesc));
            goto error;
        }

        if (harmony_bind_real(hdesc, var_name, &point[i]) != 0) {
            fprintf(stderr, "Failed to register variable %s: %s\n",
                    var_name, harmony_error_string(hdesc));
            goto error;
        }
    }

    if (harmony_launch(hdesc, NULL, 0) != 0) {
        fprintf(stderr, "Error launching tuning session: %s\n",
                harmony_error_string(hdesc));
        goto error;
    }

    if (harmony_join(hdesc, NULL, 0, session_name) != 0) {
        fprintf(stderr, "Error joining tuning session '%s': %s\n",
                session_name, harmony_error_string(hdesc));
        goto error;
    }
    free(session_name);
    return 0;

  error:
    free(session_name);
    return -1;
}

double eval_func()
{
    int i;
    double perf, nudge;

    if (verbose) {
        fprintf(stdout, "(%lf", point[0]);
        for (i = 1; i < n; ++i)
            fprintf(stdout, ", %lf", point[i]);
        fprintf(stdout, ") = ");
    }

    perf = finfo->f(n, point, options);

    if (verbose)
        fprintf(stdout, "%lf", perf);

    if (quantize) {
        perf = quantize_value(perf);
        if (verbose)
            fprintf(stdout, " => %lf", perf);
    }

    if (perturb) {
        nudge = random_value(0, perturb);
        if (verbose)
            fprintf(stdout, " (%+lf)", nudge);
        perf += nudge;
    }

    if (verbose && !single_eval)
        fprintf(stdout, "\n");
    return perf;
}

double quantize_value(double perf)
{
    return round(perf * quantize) / quantize;
}

double random_value(double min, double max)
{
    return min + ((double)rand()/(double)RAND_MAX) * (max - min);
}

void use_resources(double perf)
{
    unsigned int i, stall;

    stall = perf * 1000;

    switch (tuna_mode) {
    case 'w': /* Wall time */
        if (verbose) {
            printf(" ==> usleep(%d)\n", stall);
            fflush(stdout);
        }
        usleep(stall);
        break;

    case 'u': /* User time */
        if (verbose) {
            printf(" ==> perform %d flops\n", stall * 2);
            fflush(stdout);
        }
        for (i = 0; i < stall; ++i)
            perf = perf * ((double)17/perf);
        break;

    case 's': /* System time */
        if (verbose) {
            printf(" ==> perform %d syscalls\n", stall * 2);
            fflush(stdout);
        }
        for (i = 0; i < stall; ++i)
            close(open("/dev/null", O_RDONLY));
        break;

    case 'o': /* Output method */
        printf("%lf\n", perf);
        fflush(stdout);
        break;
    }
}
