/*
 * Copyright 2003-2012 Jeffrey K. Hollingsworth
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
#define DEFAULT_STRATEGY "brute.so"
#define DEFAULT_CLIENT_COUNT 1
#define DEFAULT_PREFETCH_LENGTH 0
#define DEFAULT_NO_WAIT 0

#define CFGKEY_HARMONY_ROOT        "HARMONY_ROOT"
#define CFGKEY_SERVER_PORT         "SERVER_PORT"
#define CFGKEY_SESSION_STRATEGY    "SESSION_STRATEGY"
#define CFGKEY_SESSION_PLUGINS     "SESSION_PLUGIN_LIST"
#define CFGKEY_SESSION_PREFETCH    "SESSION_PREFETCH"
#define CFGKEY_SESSION_NO_WAIT     "SESSION_NO_WAIT"
#define CFGKEY_CLIENT_COUNT        "NUM_CLIENTS"
#define CFGKEY_STRATEGY_CONVERGED  "STRATEGY_CONVERGED"
#define CFGKEY_BRUTE_PASSES        "BRUTE_PASSES"
#define CFGKEY_RANDOM_SEED         "RANDOM_SEED"
#define CFGKEY_NM_INITFILE         "NELDER_MEAD_INIT_FILE"
#define CFGKEY_PRO_INITFILE        "PRO_INIT_FILE"
#define CFGKEY_PRO_SIMPLEX_SIZE    "PRO_SIMPLEX_SIZE"

#define CFGKEY_CG_TYPE             "CODEGEN_TYPE"
#define CFGKEY_CG_SERVER_URL       "CODEGEN_SERVER_URL"
#define CFGKEY_CG_TARGET_URL       "CODEGEN_TARGET_URL"
#define CFGKEY_CG_REPLY_URL        "CODEGEN_REPLY_URL"
#define CFGKEY_CG_LOCAL_TMPDIR     "CODEGEN_LOCAL_TMPDIR"
#define CFGKEY_CG_SLAVE_LIST       "CODEGEN_SLAVE_LIST"
#define CFGKEY_CG_SLAVE_PATH       "CODEGEN_SLAVE_PATH"

#endif
