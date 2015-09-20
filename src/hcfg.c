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
#include "hcfg.h"
#include "hmesg.h"
#include "hutil.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <math.h> /* For NAN */

const hcfg_t hcfg_zero = HCFG_INITIALIZER;

/* -------------------------------------------
 * Default values for configuration variables.
 */
const hcfg_info_t hcfg_global_keys[] = {
    { CFGKEY_HARMONY_HOME, NULL,
      "Filesystem path to base of Active Harmony installation."},
    { CFGKEY_HARMONY_HOST, NULL,
      "Filesystem path to base of Active Harmony installation."},
    { CFGKEY_HARMONY_PORT, "1979",
      "Filesystem path to base of Active Harmony installation."},
    { CFGKEY_RANDOM_SEED, NULL,
      "Seed used to initialize the random number generator for the entire "
      "session.  If not defined, the seed is taken from the system time." },
    { CFGKEY_PERF_COUNT, "1",
      "Number of output dimensions of the search space." },
    { CFGKEY_GEN_COUNT, "1",
      "Number of trials to prepare for each expected client." },
    { CFGKEY_CLIENT_COUNT, "1",
      "Number of expected clients." },
    { CFGKEY_STRATEGY, NULL,
      "Search strategy to use as the driver for this session."
      "If left unset, the default strategy then depends upon the "
      CFGKEY_CLIENT_COUNT " configuration variable.  If it is greater "
      "than 1, the PRO strategy will be used.  Otherwise, the Nelder-Mead "
      "strategy will be used." },
    { CFGKEY_LAYERS, NULL,
      "Colon (:) separated list of plugin layer objects to load." },
    { NULL }
};

/* Internal helper function prototypes. */
static int    key_find(const hcfg_t* cfg, const char* key, char** val);
static char*  key_val(const hcfg_t* cfg, const char* key);
static char*  key_val_index(const hcfg_t* cfg, const char* key, int idx);
static int    key_add(hcfg_t* cfg, char* pair);
static void   key_del(hcfg_t* cfg, const char* key);
static int    val_to_bool(const char* val);
static long   val_to_int(const char* val);
static double val_to_real(const char* val);
static int    copy_keyval(const char* buf, char** keyval, const char** errptr);

/* Incorporate environment variables into current configuration. */
int hcfg_init(hcfg_t* cfg)
{
    extern char** environ;

    cfg->len = 0;
    cfg->cap = 32;
    cfg->env = malloc(cfg->cap * sizeof(*cfg->env));
    if (!cfg->env)
        return -1;

    for (int i = 0; environ[i]; ++i) {
        if (valid_id(environ[i], strcspn(environ[i], "="))) {
            if (cfg->len == cfg->cap)
                array_grow(&cfg->env, &cfg->cap, sizeof(char*));

            cfg->env[ cfg->len ] = stralloc( environ[i] );
            ++cfg->len;
        }
    }
    return hcfg_reginfo(cfg, hcfg_global_keys);
}

int hcfg_reginfo(hcfg_t* cfg, const hcfg_info_t* info)
{
    while (info->key) {
        if (info->val && !hcfg_get(cfg, info->key)) {
            if (hcfg_set(cfg, info->key, info->val) != 0) {
                fprintf(stderr, "Error: Could not register default value "
                                "for configuration key '%s'.\n", info->key);
                return -1;
            }
        }
        ++info;
    }
    return 0;
}

int hcfg_copy(hcfg_t* dst, const hcfg_t* src)
{
    dst->len = src->len;
    dst->cap = src->len;
    dst->env = malloc(dst->len * sizeof(*dst->env));
    if (!dst->env) return -1;

    for (int i = 0; i < dst->len; ++i) {
        dst->env[i] = stralloc( src->env[i] );
        if (!dst->env[i]) return -1;
    }
    dst->owner = NULL;

    return 0;
}

void hcfg_fini(hcfg_t* cfg)
{
    hcfg_scrub(cfg);
}

char* hcfg_get(const hcfg_t* cfg, const char* key)
{
    return key_val(cfg, key);
}

int hcfg_bool(const hcfg_t* cfg, const char* key)
{
    return val_to_bool( key_val(cfg, key) );
}

long hcfg_int(const hcfg_t* cfg, const char* key)
{
    return val_to_int( key_val(cfg, key) );
}

double hcfg_real(const hcfg_t* cfg, const char* key)
{
    return val_to_real( key_val(cfg, key) );
}

int hcfg_arr_len(const hcfg_t* cfg, const char* key)
{
    char* val = key_val(cfg, key);
    int retval = 0;

    if (val) {
        do {
            ++retval;
            val += strcspn(val, ",");
        } while (*(val++));
    }
    return retval;
}

int hcfg_arr_get(const hcfg_t* cfg, const char* key, int idx,
                 char* buf, int len)
{
    char* val = key_val_index(cfg, key, idx);
    if (!val) return -1;

    int n = strcspn(val, ",");
    while (n && isspace(val[n - 1])) --n;

    return snprintf(buf, len, "%.*s", n, val);
}

int hcfg_arr_bool(const hcfg_t* cfg, const char* key, int idx)
{
    return val_to_bool( key_val_index(cfg, key, idx) );
}

long hcfg_arr_int(const hcfg_t* cfg, const char* key, int idx)
{
    return val_to_int( key_val_index(cfg, key, idx) );
}

double hcfg_arr_real(const hcfg_t* cfg, const char* key, int idx)
{
    return val_to_real( key_val_index(cfg, key, idx) );
}

int hcfg_set(hcfg_t* cfg, const char* key, const char* val)
{
    if (!valid_id(key, strlen(key)))
        return -1;

    if (val) {
        char* pair = sprintf_alloc("%s=%s", key, val);
        return key_add(cfg, pair);
    }
    else {
        key_del(cfg, key);
        return 0;
    }
}

int hcfg_write(const hcfg_t* cfg, const char* filename)
{
    FILE* fp = stdout;

    if (strcmp(filename, "-") != 0) {
        fp = fopen(filename, "w");
        if (!fp) {
            perror("Error opening file for write");
            return -1;
        }
    }

    for (int i = 0; i < cfg->len; ++i) {
        char* ptr = strchr(cfg->env[i], '=');
        int   end = strlen(ptr) - 1;
        char* quote = cfg->env[i] + strcspn(cfg->env[i], "#'\"\n\\");

        if (isspace(ptr[1]) || isspace(ptr[end]) || *quote) {
            fprintf(fp, "%.*s=\"", (int)(ptr - cfg->env[i]), cfg->env[i]);
            ++ptr;
            while (*ptr) {
                int span = strcspn(ptr, "\"\\");
                fprintf(fp, "%.*s", span, ptr);
                ptr += span;
                if (*ptr) {
                    fprintf(fp, "\\%c", *ptr);
                    ++ptr;
                }
            }
            fprintf(fp, "\"\n");
        }
        else {
            fprintf(fp, "%s\n", cfg->env[i]);
        }
    }

    if (fp != stdout && fclose(fp) != 0) {
        fprintf(stderr, "Warning: Ignoring error on close(%s): %s\n",
                filename, strerror(errno));
    }
    return 0;
}

int hcfg_serialize(char** buf, int* buflen, const hcfg_t* cfg)
{
    int count, total;

    count = snprintf_serial(buf, buflen, "hcfg: %d ", cfg->len);
    if (count < 0) goto invalid;
    total = count;

    for (int i = 0; i < cfg->len; ++i) {
        count = printstr_serial(buf, buflen, cfg->env[i]);
        if (count < 0) goto invalid;
        total += count;
    }
    return total;

  invalid:
    errno = EINVAL;
    return -1;
}

int hcfg_deserialize(hcfg_t* cfg, char* buf)
{
    int total = 0;

    sscanf(buf, " hcfg: %d%n", &cfg->len, &total);
    if (!total)
        goto invalid;

    cfg->cap = cfg->len;
    cfg->env = malloc(cfg->len * sizeof(*cfg->env));
    for (int i = 0; i < cfg->len; ++i) {
        int count = scanstr_serial((const char**)&cfg->env[i], buf + total);
        if (count < 0) goto invalid;
        total += count;
    }
    return total;

  invalid:
    errno = EINVAL;
    return -1;
}

void hcfg_scrub(hcfg_t* cfg)
{
    for (int i = 0; i < cfg->len; ++i) {
        if (!hmesg_owner(cfg->owner, cfg->env[i]))
            free(cfg->env[i]);
    }
    free(cfg->env);
}

int hcfg_parse(hcfg_t* cfg, const char* buf, const char** errptr)
{
    const char* key = buf;
    const char* val = buf + strcspn(buf, "=");
    const char* errstr;

    if (*key == '\0')
        return 0;

    if (!valid_id(key, val - key)) {
        errstr = "Invalid key string";
        goto error;
    }

    if (*val != '=') {
        errstr = "Missing separator character (=)";
        goto error;
    }

    char* keyval;
    if (copy_keyval(buf, &keyval, errptr) != 0)
        return -1;

    if (key_add(cfg, keyval)) {
        errstr = "Could not insert key/val pair";
        goto error;
    }
    return 1;

  error:
    if (errptr) *errptr = errstr;
    return -1;
}

/*
 * Internal helper functions.
 */
int key_find(const hcfg_t* cfg, const char* key, char** val)
{
    int i;
    for (i = 0; i < cfg->len; ++i) {
        int n = strcspn(key, "=");
        if (strncmp(key, cfg->env[i], n) == 0 && cfg->env[i][n] == '=') {
            if (val) *val = cfg->env[i] + n + 1;
            break;
        }
    }
    return i;
}

char* key_val(const hcfg_t* cfg, const char* key)
{
    char* val = NULL;
    key_find(cfg, key, &val);
    return val;
}

char* key_val_index(const hcfg_t* cfg, const char* key, int idx)
{
    char* val = key_val(cfg, key);
    if (!val)
        return NULL;

    while (idx--) {
        int n = 0;
        sscanf(val, " %*[^,], %n", &n);
        if (!n)
            return NULL;
        val += n;
    }
    return val;
}

int key_add(hcfg_t* cfg, char* pair)
{
    int i = key_find(cfg, pair, NULL);

    if (i < cfg->len) {
        if (!hmesg_owner(cfg->owner, cfg->env[i]))
            free(cfg->env[i]);
    }
    else if (i == cfg->cap) {
        if (array_grow(&cfg->env, &cfg->cap, sizeof(*cfg->env)) != 0)
            return -1;
    }

    cfg->env[i] = pair;
    if (i == cfg->len)
        ++cfg->len;

    return 0;
}

void key_del(hcfg_t* cfg, const char* key)
{
    int i = key_find(cfg, key, NULL);

    if (i < cfg->len) {
        if (!hmesg_owner(cfg->owner, cfg->env[i]))
            free(cfg->env[i]);
        --cfg->len;
    }
}

int val_to_bool(const char* val)
{
    return (val && (val[0] == '1' ||
                    val[0] == 't' || val[0] == 'T' ||
                    val[0] == 'y' || val[0] == 'Y'));
}

long val_to_int(const char* val)
{
    return val ? strtol(val, NULL, 0) : -1;
}

double val_to_real(const char* val)
{
    double retval;
    if (val && sscanf(val, "%lf", &retval) == 1)
        return retval;
    return NAN;
}

int copy_keyval(const char* buf, char** keyval, const char** errptr)
{
    const char* errstr;
    const char* src;
    int cap = 0;

    while (1) {
        src = buf;
        int   len = cap;
        char* dst = *keyval;
        char  quote = '\0';
        while (*src) {
            if (!quote) {
                if      (*src == '\'') { quote = '\''; ++src; continue; }
                else if (*src ==  '"') { quote =  '"'; ++src; continue; }
            }
            else {
                if      (*src == '\\')  { ++src; }
                else if (*src == quote) { quote = '\0'; ++src; continue; }
            }

            if (len-- > 0)
                *(dst++) = *(src++);
            else
                ++src;
        }
        if (len-- > 0)
            *dst = '\0';

        if (quote != '\0') {
            errstr = "Non-terminated quote detected";
            goto error;
        }

        if (len < -1) {
            // Keyval buffer size is -len;
            cap += -len;
            *keyval = malloc(cap * sizeof(**keyval));
            if (!*keyval) {
                errstr = "Could not allocate a new configuration key/val pair";
                goto error;
            }
        }
        else break; // Loop exit.
    }
    return 0;

  error:
    if (errptr)
        *errptr = errstr;
    return -1;
}
