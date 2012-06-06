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
#ifndef __HCONFIG_H__
#define __HCONFIG_H__

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

struct cfg;
typedef struct cfg cfg_t;

cfg_t *cfg_init(void);
cfg_t *cfg_copy(const cfg_t *);
void cfg_free(cfg_t *cfg);

const char *cfg_get(cfg_t *cfg, const char *key);
int cfg_set(cfg_t *cfg, const char *key, const char *val);
int cfg_unset(cfg_t *cfg, const char *key);
int cfg_load(cfg_t *cfg, FILE *fd);
void cfg_write(cfg_t *cfg, FILE *fd);

char *cfg_parse(char *buf, char **key, char **val);

#ifdef __cplusplus
}
#endif

#endif
