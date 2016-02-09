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

/*
 * Harmony descriptor management interface.
 */
hdesc_t* ah_init(void);
int      ah_args(hdesc_t* hd, int* argc, char** argv);
void     ah_fini(hdesc_t* hd);

/*
 * Session setup interface.
 */
int ah_load(hdesc_t* hd, const char* filename);
int ah_int(hdesc_t* hd, const char* name,
           long min, long max, long step);
int ah_real(hdesc_t* hd, const char* name,
            double min, double max, double step);
int ah_enum(hdesc_t* hd, const char* name, const char* value);
int ah_strategy(hdesc_t* hd, const char* strategy);
int ah_layers(hdesc_t* hd, const char* list);
int ah_launch(hdesc_t* hd, const char* host, int port, const char* name);

/*
 * Client setup interface.
 */
int ah_join(hdesc_t* hd, const char* host, int port, const char* name);
int ah_id(hdesc_t* hd, const char* id);
int ah_bind_int(hdesc_t* hd, const char* name, long* ptr);
int ah_bind_real(hdesc_t* hd, const char* name, double* ptr);
int ah_bind_enum(hdesc_t* hd, const char* name, const char** ptr);
int ah_leave(hdesc_t* hd);

/*
 * Client/Session interaction interface.
 */
long        ah_get_int(hdesc_t* hd, const char* name);
double      ah_get_real(hdesc_t* hd, const char* name);
const char* ah_get_enum(hdesc_t* hd, const char* name);
char*       ah_get_cfg(hdesc_t* hd, const char* key);
char*       ah_set_cfg(hdesc_t* hd, const char* key, const char* val);
int         ah_fetch(hdesc_t* hd);
int         ah_report(hdesc_t* hd, double* perf);
int         ah_report_one(hdesc_t* hd, int index, double value);
int         ah_best(hdesc_t* hd);
int         ah_converged(hdesc_t* hd);
const char* ah_error_string(hdesc_t* hd);
void        ah_error_clear(hdesc_t* hd);

/*
 * Deprecated API.
 *
 * These functions are slated for removal in a future release.
 */
#ifndef DOXYGEN_SKIP

#if defined(__GNUC__)
#define DEPRECATED(message) __attribute__((__deprecated__(message)))
#else
#define DEPRECATED(message)
#endif

DEPRECATED("Use ah_init() instead")
hdesc_t* harmony_init(void);

DEPRECATED("Use ah_args() instead")
int harmony_parse_args(hdesc_t* hd, int argc, char** argv);

DEPRECATED("Use ah_fini() instead")
void harmony_fini(hdesc_t* hd);

DEPRECATED("Use ah_int() instead")
int harmony_int(hdesc_t* hd, const char* name,
                long min, long max, long step);

DEPRECATED("Use ah_real() instead")
int harmony_real(hdesc_t* hd, const char* name,
                 double min, double max, double step);

DEPRECATED("Use ah_enum() instead")
int harmony_enum(hdesc_t* hd, const char* name, const char* value);

DEPRECATED("Pass name to ah_launch() instead")
int harmony_session_name(hdesc_t* hd, const char* name);

DEPRECATED("Use ah_strategy() instead")
int harmony_strategy(hdesc_t* hd, const char* strategy);

DEPRECATED("Use ah_layers() instead")
int harmony_layers(hdesc_t* hd, const char* list);

DEPRECATED("Use ah_launch() or ah_start() instead")
int harmony_launch(hdesc_t* hd, const char* host, int port);

DEPRECATED("Use ah_id() instead")
int harmony_id(hdesc_t* hd, const char* id);

DEPRECATED("Use ah_bind_int() instead")
int harmony_bind_int(hdesc_t* hd, const char* name, long* ptr);

DEPRECATED("Use ah_bind_real() instead")
int harmony_bind_real(hdesc_t* hd, const char* name, double* ptr);

DEPRECATED("Use ah_bind_enum() instead")
int harmony_bind_enum(hdesc_t* hd, const char* name, const char** ptr);

DEPRECATED("Use ah_join() instead")
int harmony_join(hdesc_t* hd, const char* host, int port, const char* name);

DEPRECATED("Use ah_leave() instead")
int harmony_leave(hdesc_t* hd);

DEPRECATED("Use ah_get_int() instead")
long harmony_get_int(hdesc_t* hd, const char* name);

DEPRECATED("Use ah_get_real() instead")
double harmony_get_real(hdesc_t* hd, const char* name);

DEPRECATED("Use ah_get_enum() instead")
const char* harmony_get_enum(hdesc_t* hd, const char* name);

DEPRECATED("Use ah_get_cfg() instead")
char* harmony_getcfg(hdesc_t* hd, const char* key);

DEPRECATED("Use ah_set_cfg() instead")
char* harmony_setcfg(hdesc_t* hd, const char* key, const char* val);

DEPRECATED("Use ah_fetch() instead")
int harmony_fetch(hdesc_t* hd);

DEPRECATED("Use ah_report() instead")
int harmony_report(hdesc_t* hd, double* perf);

DEPRECATED("Use ah_report_one() instead")
int harmony_report_one(hdesc_t* hd, int index, double value);

DEPRECATED("Use ah_best() instead")
int harmony_best(hdesc_t* hd);

DEPRECATED("Use ah_converged() instead")
int harmony_converged(hdesc_t* hd);

DEPRECATED("Use ah_error_string() instead")
const char* harmony_error_string(hdesc_t* hd);

DEPRECATED("Use ah_error_clear() instead")
void harmony_error_clear(hdesc_t* hd);

#endif

#ifdef __cplusplus
}
#endif

#endif /* __HCLIENT_H__ */
