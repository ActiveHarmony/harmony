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

#ifdef __cplusplus
extern "C" {
#endif


void init_ref_file();

void write_nodeinfo(char *nodeinfo, char *sysName, char *release, char *machine, int core_num, char *cpu_vendor, char *cpu_model, char *cpu_freq, char *cache_size);

void write_appName(const char *appName);

void write_param_info(char *paramInfo);

void write_conf_perf_pair(int clientID, const char *param_namelist, const char *config, double performance);


#ifdef __cplusplus 
}   
#endif

