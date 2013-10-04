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
#include <math.h>

#include "testfunc.h"

double f_dejong(int n, double x[], double option[]);
double f_axispar(int n, double x[], double option[]);
double f_axisrot(int n, double x[], double option[]);
double f_rosenbrock(int n, double x[], double option[]);
double f_ackley(int n, double x[], double option[]);
double f_michalewicz(int n, double x[], double option[]);

finfo_t flist[] = {
    {"dejong", "De Jong’s first function",
     0, -64.0, 64.0, 0.0, f_dejong,
     "    De Jong's first function is continuous, convex, and unimodal.\n"},

    {"axispar", "Axis parallel hyper-ellipsoid function",
     0, -64.0, 64.0, 0.0, f_axispar,
     "    The axis parallel hyper-ellipsoid is similar the first De Jong\n"
     "    function.  It is also known as the weighted sphere model.  This\n"
     "    function is continuous, convex and unimodal.\n"},

    {"axisrot", "Rotated hyper-ellipsoid function",
     0, -64.0, 64.0, 0.0, f_axisrot,
     "    The rotated hyper-ellipsoid function.\n"},

    {"rosenbrock", "Rosenbrock's Valley",
     0, -2, 2, 0.0, f_rosenbrock,
     "    De Jong's second function, Rosenbrock's valley, is a classic\n"
     "    optimization problem, also known as Rosenbrock's banana function.\n"
     "    The global optimum is inside a long, narrow, parabolic shaped\n"
     "    flat valley.  To find the valley is trivial, however convergence\n"
     "    to the global optimum is difficult.\n"},

    {"ackley", "Ackley's Function",
     0, -32.0, 32.0, 0.0, f_ackley,
     "    Ackley's Function is a continuous, multimodal function obtained\n"
     "    by modulating an exponential function with a cosine wave of\n"
     "    moderate amplitude.  Its topology is characterized by an almost\n"
     "    flat (due to the dominating exponential) outer region and a\n"
     "    central hole or peak where the modulations by the cosine wave\n"
     "    become more and more influential.\n"},

    {"michalewicz", "Michalewicz's Function",
     0, 0, 4.0, -INFINITY, f_michalewicz,
     "    Michalewicz's Function is a multimodal function with N! local\n"
     "    optima.  It takes one parameter that controls the slope of its\n"
     "    valleys and edges.  As the parameter grows larger, the function\n"
     "    becomes more difficult and effectively becomes a needle in the\n"
     "    haystack problem.  10 is used by default.\n"},

    {NULL, NULL, 0, 0.0, 0.0, 0.0, NULL, NULL}
};

void flist_print(FILE *fd, int verbose)
{
    finfo_t *ptr;
    int len;

    len = 0;
    if (!verbose) {
        for (ptr = flist; ptr->name; ++ptr) {
            if (len < strlen(ptr->name))
                len = strlen(ptr->name);
        }
    }

    for (ptr = flist; ptr->name; ++ptr) {
        fprintf(fd, "%-*s - %s - [%.3lf, %.3lf]",
                len, ptr->name, ptr->title,
                ptr->b_min, ptr->b_max);
        if (ptr->n_max)
            fprintf(fd, " (N<=%d)", ptr->n_max);
        fprintf(fd, "\n");

        if (verbose)
            fprintf(fd, "%s\n", ptr->description);
    }
}

finfo_t *flist_find(const char *name)
{
    finfo_t *ptr;

    for (ptr = flist; ptr->name; ++ptr) {
        if (strcmp(ptr->name, name) == 0)
            break;
    }
    if (ptr->name)
        return ptr;
    return NULL;
}

double f_dejong(int n, double x[], double option[])
{
    int i;
    double d;

    d = 0.0;
    for (i = 0; i < n; ++i)
        d += x[i] * x[i];

    return d;
}

double f_axispar(int n, double x[], double option[])
{
    int i;
    double d;

    d = 0.0;
    for (i = 0; i < n; ++i)
        d += i * (x[i] * x[i]);

    return d;
}

double f_axisrot(int n, double x[], double option[])
{
    int i, j;
    double d;

    d = 0.0;
    for (i = 0; i < n; ++i)
        for (j = 0; j < i; ++j)
            d += x[j] * x[j];

    return d;
}

double f_rosenbrock(int n, double x[], double option[])
{
    int i;
    double d, d1, d2;

    if (n == 2) {
        d1 = x[1] - (x[0] * x[0]);
        d2 = 1.0 - x[0];
        d  = (100.0 * d1 * d1) + (d2 + d2);
    }
    else {
        d = 0.0;
        for (i = 1; i < (n-1); ++i) {
            d1 = x[i+1] - (x[i-1] * x[i-1]);
            d2 = 1.0 - x[i-1];
            d += (100.0 * d1 * d1) + (d2 * d2);
        }
    }
    return d;
}

double f_ackley(int n, double x[], double option[])
{
    int i;
    double a, b, c;
    double d1, d2;

    if (option) {
        a = option[0];
        b = option[1];
        c = option[2];
    }
    else {
        a = 20.0;
        b = 0.2;
        c = 2.0 * M_PI;
    }

    d1 = d2 = 0.0;
    for (i = 0; i < n; ++i) {
        d1 += x[i] * x[i];
        d2 += cos(c * x[i]);
    }

    return -a * exp(-b * sqrt(d1 / n)) - exp(d2 / n) + a + M_E;
}

double f_michalewicz(int n, double x[], double option[])
{
    int i;
    double m = 10.0;
    double sum = 0.0;

    if (option)
        m = option[0];

    for (i = 0; i < n; ++i) {
        double d1 = i * x[i] * x[i];
        double d2 = sin(d1 / M_PI);

        sum += sin(x[i]) * pow(d2, 2 * m);
    }
    return -sum;
}
