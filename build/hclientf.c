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
#include <string.h>
#include "hclient.h"
#include "hmesgs.h"

#ifdef __cplusplus
extern "C" {
#endif

void harmony_startup_f(int *socketIndex)
{
    *socketIndex = harmony_startup();
}

void harmony_get_client_id_f(int *socketIndex, int *id)
{
    *id = harmony_get_client_id(*socketIndex);
}

void harmony_startup_args_f(int *sport, char *shost, int *use_sigs,
                            int *relocated, int *socketIndex)
{
    *socketIndex = harmony_startup(*sport, shost, *use_sigs, *relocated);
}

void harmony_end_f(int *socketIndex)
{
    harmony_end(*socketIndex);
}

void harmony_application_setup_f(char *description, int *socketIndex)
{
    harmony_application_setup(description, *socketIndex);
}

void harmony_application_setup_file_f(char *fname, int *socketIndex)
{
    harmony_application_setup_file(fname, *socketIndex);
}

void harmony_add_variable_int_f(int *socketIndex, char *appName,
                                char *bundleName, int *local, int *var)
{
    void *tmp = harmony_add_variable(appName, bundleName, VAR_INT,
                                     *socketIndex, *local);
    *var = *((int *)tmp);
}

void harmony_add_variable_str_f(int *socketIndex, char *appName,
                                char *bundleName, int *local,
                                char *var, int len)
{
    void *tmp = harmony_add_variable(appName, bundleName, VAR_INT,
                                     *socketIndex, *local);
    var = (char *)tmp;
    len = strlen(var);
}

void harmony_set_variable_int_f(int *socketIndex, int *variable)
{
    harmony_set_variable(variable, *socketIndex);
}

void harmony_set_all_f(int *socketIndex)
{
    harmony_set_all(*socketIndex);
}

void harmony_request_variable_f(int *socketIndex, char *name, int *var)
{
    void *tmp = harmony_request_variable(name, *socketIndex);
    *var = *((int *)tmp);
}

void harmony_request_all_f(int *socketIndex, int *pull, int *var)
{
    *var = harmony_request_all(*socketIndex, *pull);
}

void harmony_performance_update_int_f(int *socketIndex, int *value)
{
    harmony_performance_update(*value, *socketIndex);
}

void harmony_performance_update_double_f(int *socketIndex, double *value)
{
    harmony_performance_update(*value, *socketIndex);
}

void harmony_request_tcl_variable_f(int *socketIndex, char *name, int *result)
{
    void *tmp = harmony_request_tcl_variable(name, *socketIndex);
    *result = *((int *)tmp);
}

/* These functions are not fortran compatible.  Find another way.
void harmony_get_best_configuration_f(int *socketIndex, char *ret)
{
    return harmony_get_best_configuration(*socketIndex);
}

void *harmony_database_lookup_f(int socketIndex=0)
{
    return harmony_database_lookup(socketIndex);
}

char *harmony_get_config_variable_f(const char *key, int socketIndex=0)
{
    return harmony_get_config_variable(key, socketIndex);
}
*/

void harmony_check_convergence_f(int *socketIndex, int *result)
{
    *result = harmony_check_convergence(*socketIndex);
}

void code_generation_complete_f(int *socketIndex, int *result)
{
    *result = code_generation_complete(*socketIndex);
}

void harmony_code_generation_complete_f(int *socketIndex, int *result)
{
    *result = harmony_code_generation_complete(*socketIndex);
}

void harmony_psuedo_barrier_f(int *socketIndex)
{
    harmony_psuedo_barrier(*socketIndex);
}

#ifdef __cplusplus
}
#endif
