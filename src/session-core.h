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

#ifndef __SESSION_CORE_H__
#define __SESSION_CORE_H__

#include "hpoint.h"
#include "hperf.h"
#include "hspace.h"
#include "hcfg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum hflow_status {
    HFLOW_UNKNOWN,
    HFLOW_ACCEPT,
    HFLOW_RETURN,
    HFLOW_WAIT,
    HFLOW_REJECT,
    HFLOW_RETRY,

    HFLOW_MAX
} hflow_status_t;

typedef struct hflow {
    hflow_status_t status;
    hpoint_t point;
} hflow_t;

typedef struct htrial {
    const hpoint_t point;
    hperf_t perf;
} htrial_t;

// Generic plug-in event-hook signatures.
typedef void* (*hook_alloc_t)(void);
typedef int   (*hook_init_t)(void* data, hspace_t* space);
typedef int   (*hook_join_t)(void* data, const char* client);
typedef int   (*hook_setcfg_t)(void* data, const char* key, const char* val);
typedef int   (*hook_fini_t)(void* data);

// Strategy plug-in function signatures.
typedef int (*strategy_generate_t)(void* data, hflow_t* flow, hpoint_t* point);
typedef int (*strategy_rejected_t)(void* data, hflow_t* flow, hpoint_t* point);
typedef int (*strategy_analyze_t)(void* data, htrial_t* trial);
typedef int (*strategy_best_t)(void* data, hpoint_t* point);

// Layer plug-in function signatures.
typedef int (*layer_generate_t)(void* data, hflow_t* flow, htrial_t* trial);
typedef int (*layer_analyze_t)(void* data, hflow_t* flow, htrial_t* trial);

// Callback function signatures.
typedef int (*cb_func_t)(int fd, void* data,
                         hflow_t* flow, int n, htrial_t** trial);

/*
 * Interface for plug-in modules to access their associated search.
 */
int  search_best(hpoint_t* best);
int  search_callback_generate(int fd, cb_func_t func);
int  search_callback_analyze(int fd, cb_func_t func);
void search_error(const char* msg);
int  search_restart(void);
int  search_setcfg(const char* key, const char* val);

extern const hcfg_t* search_cfg;

#ifdef __cplusplus
}
#endif

#endif
