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
#ifndef __HGLOBAL_CONFIG__
#define __HGLOBAL_CONFIG__

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

int cfg_init();
const char *cfg_get(const char *key);
int cfg_set(const char *key, const char *value);
int cfg_unset(const char *key);
int cfg_parseline(char *line, char **key, char **val);

int cfg_read(FILE *fd);
void cfg_write(FILE *fd);

#ifdef __cplusplus
}
#endif

#endif
