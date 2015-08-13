/*
 * Copyright 2003-2015 Jeffrey K. Hollingsworth
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

/**
 * \file hclient.h
 * \brief Harmony client application function header.
 *
 * All clients must include this file to participate in a Harmony
 * tuning session.
 */

#ifndef __HCLIENT_H__
#define __HCLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DOXYGEN_SKIP

struct hdesc_t;
typedef struct hdesc_t hdesc_t;

#endif

/* -------------------------------------------------------------------
 * Harmony descriptor management functions.
 */
hdesc_t* harmony_init(void);
int      harmony_parse_args(hdesc_t* hdesc, int argc, char** argv);
void     harmony_fini(hdesc_t* hdesc);

/* -------------------------------------------------------------------
 * Session setup functions.
 */
int harmony_session_name(hdesc_t* hdesc, const char* name);
int harmony_int(hdesc_t* hdesc, const char* name,
                long min, long max, long step);
int harmony_real(hdesc_t* hdesc, const char* name,
                 double min, double max, double step);
int harmony_enum(hdesc_t* hdesc, const char* name, const char* value);
int harmony_strategy(hdesc_t* hdesc, const char* strategy);
int harmony_layers(hdesc_t* hdesc, const char* list);
int harmony_launch(hdesc_t* hdesc, const char* host, int port);

/* -------------------------------------------------------------------
 * Client setup functions.
 */
int harmony_id(hdesc_t* hdesc, const char* id);
int harmony_bind_int(hdesc_t* hdesc, const char* name, long* ptr);
int harmony_bind_real(hdesc_t* hdesc, const char* name, double* ptr);
int harmony_bind_enum(hdesc_t* hdesc, const char* name, const char** ptr);
int harmony_join(hdesc_t* hdesc, const char* host, int port, const char* name);
int harmony_leave(hdesc_t* hdesc);

/* -------------------------------------------------------------------
 * Client/Session interaction functions.
 */
long        harmony_get_int(hdesc_t* hdesc, const char* name);
double      harmony_get_real(hdesc_t* hdesc, const char* name);
const char* harmony_get_enum(hdesc_t* hdesc, const char* name);
char*       harmony_getcfg(hdesc_t* hdesc, const char* key);
char*       harmony_setcfg(hdesc_t* hdesc, const char* key, const char* val);
int         harmony_fetch(hdesc_t* hdesc);
int         harmony_report(hdesc_t* hdesc, double* perf);
int         harmony_report_one(hdesc_t* hdesc, int index, double value);
int         harmony_best(hdesc_t* hdesc);
int         harmony_converged(hdesc_t* hdesc);
const char* harmony_error_string(hdesc_t* hdesc);
void        harmony_error_clear(hdesc_t* hdesc);

#ifdef __cplusplus
}
#endif

#endif /* __HCLIENT_H__ */
