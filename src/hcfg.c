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

typedef struct keyval {
    const char* key;
    int         klen;
    const char* val;
    int         vlen;
} keyval_t;

const hcfg_t HCFG_INITIALIZER = {0};

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
static int    key_check(const char* key, int len);
static int    key_add(hcfg_t* cfg, keyval_t* pair);
static int    val_to_bool(const char* val);
static long   val_to_int(const char* val);
static double val_to_real(const char* val);
static int    val_unquote(char* buf, int len);
static int    buf_parse(const char* buf, keyval_t* pair);
static int    string_trim(const char** str, int len);

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
        if (key_check(environ[i], strcspn(environ[i], "="))) {
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
    return 0;
}

void hcfg_fini(hcfg_t* cfg)
{
    for (int i = 0; i < cfg->len; ++i)
        free(cfg->env[i]);
    free(cfg->env);
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
    if (!key_check(key, strlen(key)))
        return -1;

    keyval_t pair = {key, -1, val, -1};
    return key_add(cfg, &pair);
}

int hcfg_parse(hcfg_t* cfg, const char* buf)
{
    keyval_t pair;
    if (buf_parse(buf, &pair) != 0) {
        return -1;
    }
    return key_add(cfg, &pair);
}

/* Incorporate file into the current environment. */
int hcfg_loadfile(hcfg_t* cfg, const char* filename)
{
    FILE* fp = stdin;
    int   retval = 0;

    if (strcmp(filename, "-") != 0) {
        fp = fopen(filename, "r");
        if (!fp) {
            fprintf(stderr, "Error: Could not open '%s' for reading: %s\n",
                    filename, strerror(errno));
            return -1;
        }
    }

    char* end = NULL;
    char* line = NULL;
    int   cap = 0, linecount = 1;
    do {
        int count = file_read_line(fp, &line, &cap, &end);
        keyval_t pair = {NULL, 0, NULL, 0};

        if (buf_parse(line, &pair) == 0) {
            if (val_unquote((char*)pair.val, pair.vlen) != 0) {
                fprintf(stderr, "Error: Non-terminated quote detected.\n");
                goto error;
            }
            pair.vlen = strlen(pair.val);

            if (key_add(cfg, &pair) != 0) {
                fprintf(stderr, "Error adding configuration directive.\n");
                goto error;
            }
        }
        else if (pair.val) {
            fprintf(stderr, "Error: No separator character (=) found.\n");
            goto error;
        }
        else if (pair.key) {
            fprintf(stderr, "Error: Configuration key '%.*s' is not a "
                            "valid identifier.\n", pair.klen, pair.key);
            goto error;
        }
        linecount += count;
    }
    while (end != line);
    goto cleanup;

  error:
    fprintf(stderr, "Error parsing %s:%d.\n", filename, linecount);
    retval = -1;

  cleanup:
    if (fclose(fp) != 0) {
        perror("Warning: Could not close configuration file");
    }
    free(line);

    return retval;
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
        const char* line;
        int count = scanstr_serial((const char**)&line, buf + total);
        if (count < 0) goto invalid;
        total += count;

        cfg->env[i] = stralloc(line);
    }
    return total;

  invalid:
    errno = EINVAL;
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

#define ALPHA_CHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz"
#define NUM_CHARS "0123456789"
int key_check(const char* key, int len)
{
    int span = strspn(key, ALPHA_CHARS NUM_CHARS);
    return span == len && strspn(key, ALPHA_CHARS) > 0;
}

int key_add(hcfg_t* cfg, keyval_t* pair)
{
    int i = key_find(cfg, pair->key, NULL);
    if (i < cfg->len)
        free(cfg->env[i]);

    if (pair->val) {
        // Key add/replace case.
        if (i == cfg->cap) {
            if (array_grow(&cfg->env, &cfg->cap, sizeof(*cfg->env)) != 0)
                return -1;
        }

        cfg->env[i] = sprintf_alloc("%.*s=%.*s",
                                    pair->klen, pair->key,
                                    pair->vlen, pair->val);
        if (!cfg->env[i])
            return -1;

        if (i == cfg->len)
            ++cfg->len;
    }
    else if (i < cfg->len) {
        // Key delete case.
        if (i < --cfg->len)
            cfg->env[i] = cfg->env[cfg->len];
    }
    return 0;
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

int val_unquote(char* buf, int len)
{
    char* ptr = buf;
    char quote = '\0';

    buf[len] = '\0';
    while (*ptr) {
        if (*ptr == '\\' && *(ptr + 1)) { ++ptr; }
        else if (!quote) {
            if (*ptr == '\'') { quote = '\''; ++ptr; continue; }
            if (*ptr ==  '"') { quote =  '"'; ++ptr; continue; }
            if (*ptr ==  '#') { break; }
            if (*ptr == '\n') { break; }
        }
        else if (*ptr == quote) { quote = '\0'; ++ptr; continue; }

        *(buf++) = *(ptr++);
    }
    if (quote != '\0')
        return -1;

    *buf = '\0';
    return 0;
}

int buf_parse(const char* buf, keyval_t* pair)
{
    const char* sep = buf + strcspn(buf, "=");
    int len = string_trim(&buf, sep - buf);

    if (*buf == '\0')
        return -1;

    pair->key = buf;
    pair->klen = len;
    if (!key_check(buf, len))
        return -1;

    if (*sep != '=') {
        pair->val = sep;
        return -1;
    }

    ++sep;
    len = string_trim(&sep, strlen(sep));
    if (sep[len] && sep[len-1] == '\\')
        ++len; // The final character was backslash-escaped whitespace.

    pair->val = sep;
    pair->vlen = len;
    return 0;
}

int string_trim(const char** str, int len)
{
    const char* head = *str;
    const char* tail = head + len;

    while (isspace(*head)) ++head;
    while (head < tail && isspace(*(tail - 1))) --tail;

    *str = head;
    return tail - head;
}
