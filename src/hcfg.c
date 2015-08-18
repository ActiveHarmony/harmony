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
static char*   key_lookup(const hcfg_t* cfg, const char* key);
static char*   key_index(const hcfg_t* cfg, const char* key, int idx);
static int     key_check(const char* key, char end);
static int     val_to_bool(const char* val);
static long    val_to_int(const char* val);
static double  val_to_real(const char* val);
static char*   line_findend(char* buf);
static int     line_unquote(char* buf);
static char*   line_parse(char** ptr);

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
        char* var = strchr(environ[i], '=') + 1;
        if (key_check(environ[i], '=') && *var) {
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
    return key_lookup(cfg, key);
}

int hcfg_bool(const hcfg_t* cfg, const char* key)
{
    return val_to_bool( key_lookup(cfg, key) );
}

long hcfg_int(const hcfg_t* cfg, const char* key)
{
    return val_to_int( key_lookup(cfg, key) );
}

double hcfg_real(const hcfg_t* cfg, const char* key)
{
    return val_to_real( key_lookup(cfg, key) );
}

int hcfg_arr_len(const hcfg_t* cfg, const char* key)
{
    char* val = key_lookup(cfg, key);
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
    char* val = key_index(cfg, key, idx);
    if (!val) return -1;

    int n = strcspn(val, ",");
    while (n && isspace(val[n - 1])) --n;

    return snprintf(buf, len, "%.*s", n, val);
}

int hcfg_arr_bool(const hcfg_t* cfg, const char* key, int idx)
{
    return val_to_bool( key_index(cfg, key, idx) );
}

long hcfg_arr_int(const hcfg_t* cfg, const char* key, int idx)
{
    return val_to_int( key_index(cfg, key, idx) );
}

double hcfg_arr_real(const hcfg_t* cfg, const char* key, int idx)
{
    return val_to_real( key_index(cfg, key, idx) );
}

int hcfg_set(hcfg_t* cfg, const char* key, const char* val)
{
    int i;

    if (!key_check(key, '\0'))
        return -1;

    for (i = 0; i < cfg->len; ++i) {
        int n = strcspn(cfg->env[i], "=");
        if (strncasecmp(key, cfg->env[i], n) == 0 && key[n] == '\0') {
            free(cfg->env[i]);
            break;
        }
    }

    if (val && *val) {
        // Key add/replace case.
        if (i == cfg->cap)
            array_grow(&cfg->env, &cfg->cap, sizeof(*cfg->env));

        cfg->env[i] = sprintf_alloc("%s=%s", key, val);
        if (i == cfg->len)
            ++cfg->len;
    }
    else if (i < cfg->len) {
        // Key delete case.
        if (i < cfg->len - 1)
            cfg->env[i] = cfg->env[ cfg->len - 1 ];
        --cfg->len;
    }

    return 0;
}

/* Incorporate file into the current environment. */
int hcfg_loadfile(hcfg_t* cfg, const char* filename)
{
    FILE* fp = stdin;
    char* buf = NULL;
    int   buf_cap = 1024;
    char* ptr;
    int   linecount = 1;
    int   retval = 0;

    if (strcmp(filename, "-") != 0) {
        fp = fopen(filename, "r");
        if (!fp) {
            fprintf(stderr, "Error: Could not open '%s' for reading: %s\n",
                    filename, strerror(errno));
            return -1;
        }
    }

    buf = malloc(buf_cap * sizeof(*buf));
    if (!buf) {
        perror("Error: Could not allocate configuration parse buffer");
        goto error;
    }
    buf[0] = '\0';

    ptr = buf;
    while (!feof(fp)) {
        size_t len = strlen(ptr);
        if (ptr != buf)
            memmove(buf, ptr, len);

        if (len + 1 == (size_t)buf_cap) {
            if (array_grow(&buf, &buf_cap, sizeof(char)) != 0) {
                perror("Error: Could not grow config parsing buffer");
                goto error;
            }
        }
        len += fread(buf + len, sizeof(char), buf_cap - len - 1, fp);
        buf[len] = '\0';

        ptr = buf;
        while (*ptr != '\0') {
            char* next = line_findend(ptr);
            if (*next == '\n' || feof(fp)) {
                int more = (*next == '\n');
                int count = line_unquote(ptr);
                if (!count) goto error;

                char* key = ptr;
                char* val = line_parse(&key);
                if (!val) goto error;

                if (key) {
                    if (!key_check(key, '\0')) {
                        fprintf(stderr, "Error: Configuration key '%s' "
                                "contains invalid characters.\n", key);
                        goto error;
                    }

                    if (hcfg_set(cfg, key, val) != 0) {
                        fprintf(stderr, "Error setting configuration key.\n");
                        goto error;
                    }
                }

                linecount += count;
                ptr = more ? next + 1 : next;
            }
            else break;
        }
    }
    goto cleanup;

  error:
    fprintf(stderr, "Error parsing %s:%d.\n", filename, linecount);
    retval = -1;

  cleanup:
    if (fclose(fp) != 0) {
        perror("Warning: Could not close configuration file");
    }
    free(buf);

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
char* key_lookup(const hcfg_t* cfg, const char* key)
{
    for (int i = 0; i < cfg->len; ++i) {
        int n = strcspn(cfg->env[i], "=");
        if (strncasecmp(key, cfg->env[i], n) == 0 && key[n] == '\0')
            return cfg->env[i] + n + 1;
    }
    return NULL;
}

char* key_index(const hcfg_t* cfg, const char* key, int idx)
{
    char* val = key_lookup(cfg, key);
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

int key_check(const char* key, char end)
{
    int n = 0;
    sscanf(key, "%*[0-9a-zA-Z_]%n", &n);
    return (key[n] == end);
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

/* Find the end of a configuration key/val string. */
char* line_findend(char* buf)
{
    buf += strcspn(buf, "=#\n");
    if (*buf == '#') {
        buf += strcspn(buf, "\n");
    }
    else if (*buf == '=') {
        char quote = '\0';
        while (*buf) {
            if (*buf == '\\') ++buf;
            else if (!quote) {
                if      (*buf == '\'') quote = '\'';
                else if (*buf == '"')  quote = '"';
                else if (*buf == '#')  buf += strcspn(buf, "\n") - 1;
                else if (*buf == '\n') break;
            }
            else if (*buf == quote) quote = '\0';

            if (*buf) ++buf;
        }
    }
    return buf;
}

int line_unquote(char* buf)
{
    char* ptr;
    int linecount = 1;

    buf += strcspn(buf, "=#\n");
    if (*buf == '#') {
        /* '#' found before '=' or '\n'. */
        ptr = buf + strcspn(buf, "\n");
    }
    else if (*buf == '=') {
        /* Uncommented '=' found.  Begin unquoting. */
        char* end = buf;
        char quote = '\0';
        ptr = ++buf;
        while (isspace(*ptr) && *ptr != '\n') ++ptr;

        while (*ptr) {
            if (*ptr == '\\') ++ptr;
            else if (!quote) {
                if      (*ptr == '\'') { quote = *(ptr++); continue; }
                else if (*ptr == '"')  { quote = *(ptr++); continue; }
                else if (*ptr == '#')  { ptr += strcspn(ptr, "\n"); break; }
                else if (*ptr == '\n') break;
            }
            else if (*ptr == quote) { quote ^= *(ptr++); end = buf; continue; }

            if (*ptr) {
                if (*ptr == '\n') ++linecount;
                *(buf++) = *(ptr++);
            }
        }

        if (quote) {
            fprintf(stderr, "Error: Non-terminated quote detected.\n");
            return 0;
        }

        while (buf > end && isspace(*(buf - 1))) --buf;
    }
    *buf = '\0';

    return linecount;
}

char* line_parse(char** ptr)
{
    char* key = *ptr;
    while (isspace(*key)) ++key;
    if (*key == '\0') {
        *ptr = NULL;
        return key;
    }

    char* sep = strchr(key, '=');
    if (!sep) {
        fprintf(stderr, "Error: No separator character (=) found.\n");
        return NULL;
    }

    char* val = sep + 1;
    while (key < sep && isspace(*(sep - 1))) --sep;
    *sep = '\0';

    *ptr = key;
    return val;
}
