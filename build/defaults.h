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
#ifndef __DEFAULTS_H__
#define __DEFAULTS_H__

#define DEFAULT_HOST "localhost"
#define DEFAULT_PORT 1979
#define DEFAULT_CONFIG_FILENAME "harmony.cfg"
#define DEFAULT_STRATEGY "pro.so"
#define DEFAULT_CLIENT_COUNT 1
#define DEFAULT_PER_CLIENT 1

#define CFGKEY_HARMONY_ROOT       "HARMONY_ROOT"
#define CFGKEY_SERVER_PORT        "SERVER_PORT"
#define CFGKEY_SESSION_STRATEGY   "SESSION_STRATEGY"
#define CFGKEY_SESSION_LAYERS     "SESSION_LAYER_LIST"
#define CFGKEY_CLIENT_COUNT       "CLIENT_COUNT"
#define CFGKEY_PER_CLIENT_STORAGE "PER_CLIENT_STORAGE"
#define CFGKEY_STRATEGY_CONVERGED "STRATEGY_CONVERGED"
#define CFGKEY_BRUTE_PASSES       "BRUTE_PASSES"
#define CFGKEY_RANDOM_SEED        "RANDOM_SEED"

#define CFGKEY_SIMPLEX_SIZE       "SIMPLEX_SIZE"
#define CFGKEY_INIT_METHOD        "INIT_METHOD"
#define CFGKEY_INIT_PERCENT       "INIT_PERCENT"
#define CFGKEY_INIT_POINT         "INIT_POINT"
#define CFGKEY_REFLECT            "REFLECT"
#define CFGKEY_EXPAND             "EXPAND"
#define CFGKEY_CONTRACT           "CONTRACT"
#define CFGKEY_SHRINK             "SHRINK"
#define CFGKEY_FVAL_TOL           "FVAL_TOL"
#define CFGKEY_SIZE_TOL           "SIZE_TOL"

#define CFGKEY_CG_TYPE            "CODEGEN_TYPE"
#define CFGKEY_CG_SERVER_URL      "CODEGEN_SERVER_URL"
#define CFGKEY_CG_TARGET_URL      "CODEGEN_TARGET_URL"
#define CFGKEY_CG_REPLY_URL       "CODEGEN_REPLY_URL"
#define CFGKEY_CG_LOCAL_TMPDIR    "CODEGEN_LOCAL_TMPDIR"
#define CFGKEY_CG_SLAVE_LIST      "CODEGEN_SLAVE_LIST"
#define CFGKEY_CG_SLAVE_PATH      "CODEGEN_SLAVE_PATH"

#endif
