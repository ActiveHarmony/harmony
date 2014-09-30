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

typedef struct keyval {
    char *key;
    char *val;
} keyval_t;

struct hcfg {
    keyval_t *hash;
    unsigned int count;
    unsigned int logsize;
};

/* Internal function prototypes */
int line_count(const char *buf);
unsigned int hash_function(const char *input, unsigned int logsize);
keyval_t *hash_init(unsigned int logsize);
keyval_t *hash_find(hcfg_t *cfg, const char *key);
int hash_resize(hcfg_t *cfg, unsigned int new_logsize);
int hash_insert(hcfg_t *cfg, const char *key, const char *val);
int hash_delete(hcfg_t *cfg, const char *key);
void hash_clear(hcfg_t *cfg);

/* Static global variables */
static const unsigned HASH_GROWTH_THRESH = 50;
static const unsigned CONFIG_INITIAL_LOGSIZE = 8;

hcfg_t *hcfg_alloc(void)
{
    hcfg_t *retval;

    retval = (hcfg_t *) malloc(sizeof(struct hcfg));
    if (retval == NULL)
        return NULL;

    retval->count = 0;
    retval->logsize = CONFIG_INITIAL_LOGSIZE;
    retval->hash = hash_init(CONFIG_INITIAL_LOGSIZE);
    if (retval->hash == NULL) {
        free(retval);
        return NULL;
    }

    return retval;
}

hcfg_t *hcfg_copy(const hcfg_t *src)
{
    unsigned i, size = 0;
    hcfg_t *retval;

    retval = (hcfg_t *)malloc(sizeof(struct hcfg));
    if (retval == NULL)
        return NULL;

    retval->count = src->count;
    retval->logsize = src->logsize;
    retval->hash = hash_init(src->logsize);
    if (retval->hash == NULL)
        goto cleanup;

    size = 1 << src->logsize;
    for (i = 0; i < size; ++i) {
        if (src->hash[i].key != NULL) {
            retval->hash[i].key = malloc(strlen(src->hash[i].key) + 1);
            retval->hash[i].val = malloc(strlen(src->hash[i].val) + 1);
            if (retval->hash[i].key == NULL || retval->hash[i].val == NULL)
                goto cleanup;
            strcpy(retval->hash[i].key, src->hash[i].key);
            strcpy(retval->hash[i].val, src->hash[i].val);
        }
    }
    return retval;

  cleanup:
    if (retval->hash != NULL) {
        for (i = 0; i < size; ++i) {
            free(retval->hash[i].key);
            free(retval->hash[i].val);
        }
        free(retval->hash);
    }
    free(retval);
    return NULL;
}

void hcfg_free(hcfg_t *cfg)
{
    hash_clear(cfg);
    free(cfg->hash);
    free(cfg);
}

const char *hcfg_get(hcfg_t *cfg, const char *key)
{
    keyval_t *entry;
    entry = hash_find(cfg, key);
    if (entry == NULL || entry->key == NULL)
        return NULL;

    return entry->val;
}

int hcfg_set(hcfg_t *cfg, const char *key, const char *val)
{
    if (val == NULL)
        return hash_delete(cfg, key);
    return hash_insert(cfg, key, val);
}

int hcfg_unset(hcfg_t *cfg, const char *key)
{
    return hash_delete(cfg, key);
}

/* Open the configuration file, and parse it line by line. */
int hcfg_load(hcfg_t *cfg, const char *filename)
{
    FILE *fd;
    char buf[4096];
    char *head, *next, *tail, *key, *val;
    int linenum = 1;

    if (strcmp(filename, "-") == 0)
        fd = stdin;
    else {
        fd = fopen(filename, "r");
        if (!fd) {
            perror("Error parsing configuration data");
            return -1;
        }
    }

    head = buf;
    tail = buf;
    while (!feof(fd)) {
        int count;

        tail += fread(tail, sizeof(char), sizeof(buf) - (tail - buf + 1), fd);
        *tail = '\0';

        while (head < tail) {
            count = line_count(head);
            if (count == 0 && !feof(fd))
                break;

            next = hcfg_parse(head, &key, &val);
            if (!next)
                break;

            if (key && val) {
                keyval_t *entry = hash_find(cfg, key);

                if (entry != NULL && entry->key != NULL) {
                    fprintf(stderr, "Warning: Line %d: Redefinition of"
                            " configuration key %s.\n", linenum, key);
                }

                if (hash_insert(cfg, key, val) != 0) {
                    fprintf(stderr, "Error: Internal hash table error.\n");
                    fclose(fd);
                    return -1;
                }
            }
            linenum += count;
            head = next;
        }

        /* Parse error detected. */
        if (!next)
            break;

        /* Full buffer with no newline. */
        if (tail - head == sizeof(buf) - 1)
            break;

        /* Move remaining data to front of buffer. */
        if (head != buf) {
            next = buf;
            while (head < tail)
                *(next++) = *(head++);
            tail = next;
            head = buf;
        }
    }
    if (fclose(fd) != 0) {
        perror("Error parsing configuration data");
        return -1;
    }

    if (head != tail) {
        fprintf(stderr, "Parse error in file %s line %d: ", filename, linenum);
        if (tail - head == sizeof(buf) - 1)
            fprintf(stderr, "Line buffer overflow.\n");
        else if (key == NULL)
            fprintf(stderr, "Invalid key string.\n");
        else if (val == NULL)
            fprintf(stderr, "No key/value separator character (=).\n");
        return -1;
    }

    return 0;
}

int hcfg_merge(hcfg_t *dst, const hcfg_t *src)
{
    int i;
    keyval_t *entry;

    for (i = 0; i < (1 << src->logsize); ++i) {
        if (!src->hash[i].key)
            continue;

        entry = hash_find(dst, src->hash[i].key);
        if (entry && entry->key)
            continue;

        if (hash_insert(dst, src->hash[i].key, src->hash[i].val) < 0)
            return -1;
    }
    return 0;
}

int hcfg_write(hcfg_t *cfg, const char *filename)
{
    FILE *fd;
    int i;

    if (strcmp(filename, "-") == 0)
        fd = stdout;
    else {
        fd = fopen(filename, "w");
        if (!fd) {
            perror("Error opening file for write");
            return -1;
        }
    }

    for (i = 0; i < (1 << cfg->logsize); ++i) {
        if (cfg->hash[i].key != NULL) {
            fprintf(fd, "%s=%s\n", cfg->hash[i].key, cfg->hash[i].val);
        }
    }

    if (fclose(fd) != 0) {
        perror("Error parsing configuration data");
        return -1;
    }
    return 0;
}

/* Parse the key and value from a memory buffer. */
char *hcfg_parse(char *buf, char **key, char **val)
{
    char *ptr = NULL;

    *key = NULL;
    *val = NULL;

    /* Skip leading key whitespace. */
    while (isspace(*buf) && *buf != '\n')
        ++buf;

    /* Skip empty lines and comments. */
    if (*buf == '\n' || *buf == '#')
        goto endline;

    if (!isalnum(*buf) && *buf != '_')
        return NULL;

    /* Force key strings to be uppercase. */
    *key = buf;
    while (isalnum(*buf) || *buf == '_') {
        *buf = toupper(*buf);
        ++buf;
    }

    /* Check that key string was valid. */
    ptr = buf;
    while (isspace(*buf) && *buf != '\n')
        ++buf;

    if (*(buf++) != '=')
        return NULL;

    while (isspace(*buf) && *buf != '\n')
        ++buf;

    /* Kill whitespace and separator between key and value. */
    while (ptr < buf)
        *(ptr++) = '\0';

    /* Check for empty value, used for deleting a key/value pair. */
    if (*buf == '\n' || *buf == '\0')
        goto endline;

    /* Unquote the value string. */
    *val = buf;
    while (*buf && *buf != '\n') {
        if (*buf ==  '#') goto endline;
        if (*buf == '\\') ++buf;
        *(ptr++) = *(buf++);
    }
    if (**val == '\\')
        ++(*val);

  endline:
    buf += strcspn(buf, "\n");
    if (*buf == '\n')
        ++buf;

    /* Kill trailing value whitespace. */
    if (ptr) {
        do *(ptr--) = '\0';
        while (isspace(*ptr));
    }
    return buf;
}

int hcfg_is_cmd(const char *buf)
{
    while (isspace(*buf)) ++buf;
    while (isalnum(*buf) || *buf == '_') ++buf;
    while (isspace(*buf)) ++buf;

    return (*buf == '=');
}

int hcfg_serialize(char **buf, int *buflen, const hcfg_t *cfg)
{
    int i, count, total;

    count = snprintf_serial(buf, buflen, "hcfg:%u %u ",
                            cfg->count, cfg->logsize);
    if (count < 0) goto invalid;
    total = count;

    for (i = 0; i < (1 << cfg->logsize); ++i) {
        if (cfg->hash[i].key) {
            count = printstr_serial(buf, buflen, cfg->hash[i].key);
            if (count < 0) goto invalid;
            total += count;

            count = printstr_serial(buf, buflen, cfg->hash[i].val);
            if (count < 0) goto invalid;
            total += count;
        }
    }
    return total;

  invalid:
    errno = EINVAL;
    return -1;
}

int hcfg_deserialize(hcfg_t *cfg, char *buf)
{
    int count, total;
    unsigned int i, kcount, logsize;
    const char *key, *val;

    if (sscanf(buf, " hcfg:%u %u%n", &kcount, &logsize, &count) < 2)
        goto invalid;
    total = count;

    if (hash_resize(cfg, logsize) < 0)
        goto error;

    for (i = 0; i < kcount; ++i) {
        count = scanstr_serial(&key, buf + total);
        if (count < 0) goto invalid;
        total += count;

        count = scanstr_serial(&val, buf + total);
        if (count < 0) goto invalid;
        total += count;

        if (hcfg_set(cfg, key, val) < 0)
            goto error;
    }
    return total;

  invalid:
    errno = EINVAL;
  error:
    return -1;
}

/* Find the end of the string, returning a count of the newlines. */
int line_count(const char *buf)
{
    const char *stop = "=#\\";
    const char *ptr = buf;
    int linecount = 0;

    while (*ptr && *ptr != '\n') {
        ptr += strcspn(ptr, stop);

        if (*ptr == '\\') {
            ++ptr;
            if (*stop == '#') {
                if      (*ptr == '\n') ++linecount;
                else if (*ptr == '\0') break;
                ++ptr;
            }
        }
        else if (*ptr == '=') stop = "#\n";
        else if (*ptr == '#') stop = "\n";
    }

    if (*ptr == '\n')
        ++linecount;

    return linecount;
}

/*
 * Simple implementation of an FNV-1a 32-bit hash.
 */
#define FNV_PRIME_32     0x01000193
#define FNV_OFFSET_BASIS 0x811C9DC5
unsigned int hash_function(const char *input, unsigned int logsize)
{
    unsigned int hash = FNV_OFFSET_BASIS;
    while (*input != '\0') {
        hash ^= tolower(*(input++));
        hash *= FNV_PRIME_32;
    }
    return ((hash >> logsize) ^ hash) & ((1 << logsize) - 1);
}

keyval_t *hash_init(unsigned int logsize)
{
    keyval_t *hash;
    hash = (keyval_t *) malloc(sizeof(keyval_t) * (1 << logsize));
    if (hash != NULL)
        memset(hash, 0, sizeof(keyval_t) * (1 << logsize));
    return hash;
}

keyval_t *hash_find(hcfg_t *cfg, const char *key)
{
    unsigned int idx, orig_idx, capacity;

    capacity = 1 << cfg->logsize;
    orig_idx = idx = hash_function(key, cfg->logsize);

    while (cfg->hash[idx].key != NULL &&
           strcasecmp(key, cfg->hash[idx].key) != 0)
    {
        idx = (idx + 1) % capacity;
        if (idx == orig_idx)
            return NULL;
    }
    return &cfg->hash[idx];
}

int hash_resize(hcfg_t *cfg, unsigned int new_logsize)
{
    unsigned int i, idx, orig_size, new_size;
    keyval_t *new_hash;

    if (new_logsize == cfg->logsize)
        return 0;

    orig_size = 1 << cfg->logsize;
    new_size = 1 << new_logsize;

    new_hash = (keyval_t *) malloc(sizeof(keyval_t) * new_size);
    if (new_hash == NULL)
        return -1;
    memset(new_hash, 0, sizeof(keyval_t) * new_size);

    for (i = 0; i < orig_size; ++i) {
        if (cfg->hash[i].key != NULL) {
            idx = hash_function(cfg->hash[i].key, new_logsize);
            while (new_hash[idx].key != NULL)
                idx = (idx + 1) % new_size;

            new_hash[idx].key = cfg->hash[i].key;
            new_hash[idx].val = cfg->hash[i].val;
        }
    }

    free(cfg->hash);
    cfg->hash = new_hash;
    cfg->logsize = new_logsize;
    if (cfg->logsize > 16)
        fprintf(stderr, "Warning: Internal hash grew beyond expectation.\n");

    return 0;
}

int hash_insert(hcfg_t *cfg, const char *key, const char *val)
{
    keyval_t *entry;
    char *new_val;

    entry = hash_find(cfg, key);
    if (entry == NULL)
        return -1;

    if (entry->key == NULL)
    {
        /* New entry case. */
        entry->key = malloc(strlen(key) + 1);
        entry->val = malloc(strlen(val) + 1);

        if (entry->key == NULL || entry->val == NULL) {
            free(entry->key);
            free(entry->val);
            entry->key = NULL;
            entry->val = NULL;
            return -1;
        }

        strcpy(entry->key, key);
        strcpy(entry->val, val);

        if (((++cfg->count * 100) / (1 << cfg->logsize)) > HASH_GROWTH_THRESH)
            hash_resize(cfg, cfg->logsize + 1); /* Failing to grow the
                                                   hash is not an error. */
    }
    else {
        /* Value replacement case. */
        new_val = malloc(strlen(val) + 1);
        if (new_val == NULL)
            return -1;
        strcpy(new_val, val);
        free(entry->val);
        entry->val = new_val;
    }
    return 0;
}

int hash_delete(hcfg_t *cfg, const char *key)
{
    keyval_t *entry;

    entry = hash_find(cfg, key);
    if (!entry)
        return -1;

    if (entry->key) {
        free(entry->key);
        free(entry->val);
        entry->key = NULL;
        entry->val = NULL;
        --cfg->count;
    }
    return 0;
}

void hash_clear(hcfg_t *cfg)
{
    int i;

    for (i = 0; i < (1 << cfg->logsize); ++i) {
        if (cfg->hash[i].key) {
            free(cfg->hash[i].key);
            cfg->hash[i].key = NULL;
        }
        if (cfg->hash[i].val) {
            free(cfg->hash[i].val);
            cfg->hash[i].val = NULL;
        }
    }
    cfg->count = 0;
}
