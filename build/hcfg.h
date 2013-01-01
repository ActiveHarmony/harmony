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
#ifndef __HCFG_H__
#define __HCFG_H__

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

struct hcfg;
typedef struct hcfg hcfg_t;

hcfg_t *hcfg_alloc(void);
hcfg_t *hcfg_copy(const hcfg_t *);
void hcfg_free(hcfg_t *cfg);

const char *hcfg_get(hcfg_t *cfg, const char *key);
int hcfg_set(hcfg_t *cfg, const char *key, const char *val);
int hcfg_unset(hcfg_t *cfg, const char *key);
int hcfg_load(hcfg_t *cfg, FILE *fd);
int hcfg_merge(hcfg_t *dst, const hcfg_t *src);
void hcfg_write(hcfg_t *cfg, FILE *fd);

char *hcfg_parse(char *buf, char **key, char **val);

int hcfg_serialize(char **buf, int *buflen, const hcfg_t *cfg);
int hcfg_deserialize(hcfg_t *cfg, char *buf);

#ifdef __cplusplus
}
#endif

#endif
