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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#include "hconfig.h"

typedef struct keyval {
    char *key;
    char *val;
} keyval_t;

struct cfg {
    keyval_t *hash;
    unsigned int count;
    unsigned int logsize;
};

/* Internal function prototypes */
unsigned int hash_function(const char *input, unsigned int logsize);
keyval_t *hash_init(unsigned int logsize);
keyval_t *hash_find(cfg_t *cfg, const char *key);
int hash_grow(cfg_t *cfg);
int hash_insert(cfg_t *cfg, const char *key, const char *val);
int hash_delete(cfg_t *cfg, const char *key);
void hash_clear(cfg_t *cfg);

/* Static global variables */
static const unsigned HASH_GROWTH_THRESH = 50;
static const unsigned CONFIG_INITIAL_LOGSIZE = 8;

cfg_t *cfg_init(void)
{
    cfg_t *retval;

    retval = (cfg_t *)malloc(sizeof(struct cfg));
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

cfg_t *cfg_copy(const cfg_t *src)
{
    unsigned i, size = 0;
    cfg_t *retval;

    retval = (cfg_t *)malloc(sizeof(struct cfg));
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

void cfg_free(cfg_t *cfg)
{
    hash_clear(cfg);
    free(cfg->hash);
    free(cfg);
}

const char *cfg_get(cfg_t *cfg, const char *key)
{
    keyval_t *entry;
    entry = hash_find(cfg, key);
    if (entry == NULL || entry->key == NULL)
        return NULL;

    return entry->val;
}

int cfg_set(cfg_t *cfg, const char *key, const char *val)
{
    if (val == NULL)
        return hash_delete(cfg, key);
    return hash_insert(cfg, key, val);
}

int cfg_unset(cfg_t *cfg, const char *key)
{
    return hash_delete(cfg, key);
}

/* Open the configuration file, and parse it line by line. */
int cfg_load(cfg_t *cfg, FILE *fd)
{
    char buf[4096];
    char *head, *curr, *tail, *key, *val;
    unsigned int retval, linenum = 0;
    keyval_t *entry;

    head = curr = buf;
    while (!feof(fd) || tail != head) {
        retval = fread(curr, sizeof(char), sizeof(buf) - (curr - buf), fd);
        tail = curr + retval;
        if (feof(fd) && tail < buf + sizeof(buf))
            *(tail++) = '\n';

        curr = memchr(head, '\n', tail - head);
        while (curr != NULL) {
            ++linenum;
            if (cfg_parse(head, &key, &val) == NULL) {
                fprintf(stderr, "Config parse error: Line %d: ", linenum);
                if (key == NULL)
                    fprintf(stderr, "Empty key string.\n");
                else if (val == NULL)
                    fprintf(stderr, "No key/value separator character (=).\n");

                fclose(fd);
                return -1;
            }

            if (key != NULL) {
                entry = hash_find(cfg, key);
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
            head = curr + 1;
            curr = memchr(head, '\n', tail - head);
        }

        /* Full buffer with no newline.  Read until one is found. */
        if (head == buf && tail == buf + sizeof(buf)) {
            ++linenum;
            fprintf(stderr, "Ignoring configuration file line %d:"
                    "  Line buffer overflow.\n", linenum);
            while (curr == NULL && !feof(fd)) {
                retval = fread(buf, sizeof(char), sizeof(buf), fd);
                tail = buf + retval;
                curr = memchr(head, '\n', tail - head);
            }

            if (curr != NULL)
                head = curr + 1;
            else
                head = tail = buf;
        }

        /* Move remaining data to front of buffer. */
        if (head != buf) {
            curr = buf;
            while (head < tail)
                *(curr++) = *(head++);
            tail = curr;
            head = buf;
        }
    }
    return 0;
}

void cfg_write(cfg_t *cfg, FILE *fd)
{
    int i;

    for (i = 0; i < (1 << cfg->logsize); ++i) {
        if (cfg->hash[i].key != NULL) {
            fprintf(fd, "%s=%s\n", cfg->hash[i].key, cfg->hash[i].val);
        }
    }
}

/* Parse the key and value from a memory buffer. */
char *cfg_parse(char *buf, char **key, char **val)
{
    char *next, *end;

    /* Initialize return values */
    *key = *val = NULL;

    /* Store a ptr to the next string. */
    next = buf + strcspn(buf, "\n");
    if (*next == '\n') ++next;

    /* Find the end of the line (Ignoring comments if they exist). */
    end = buf + strcspn(buf, "#\n");

    /* Skip leading and trailing whitespace. */
    while (buf < end && isspace(*buf)) ++buf;
    while (end > buf && isspace(*(end - 1)))
        *(--end) = '\0';

    /* Line is empty.  Skip it. */
    if (buf == end)
        return next;

    /* Find key boundaries. */
    if (*buf == '=')
        return NULL;

    *key = buf;
    while (*buf != '=' && !isspace(*buf)) {
        *buf = tolower(*buf);  /* Force key strings to be lowercase */
        ++buf;
    }

    /* Ensure an equal sign exists. */
    while (isspace(*buf))
        *(buf++) = '\0';

    if (*buf != '=')
        return NULL;
    *(buf++) = '\0';

    /* Find value boundaries. */
    while (buf < end && isspace(*buf)) ++buf;
    *val = buf;
    *end = '\0';

    return next;
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
        hash ^= *(input++);
        hash *= FNV_PRIME_32;
    }
    return ((hash >> logsize) ^ hash) & ((1 << logsize) - 1);
}

keyval_t *hash_init(unsigned int logsize)
{
    keyval_t *hash;
    hash = (keyval_t *)malloc(sizeof(keyval_t) * (1 << logsize));
    if (hash != NULL)
        memset(hash, 0, sizeof(keyval_t) * (1 << logsize));
    return hash;
}

keyval_t *hash_find(cfg_t *cfg, const char *key)
{
    unsigned int idx, orig_idx, capacity, count = 1;

    capacity = 1 << cfg->logsize;
    orig_idx = idx = hash_function(key, cfg->logsize);

    while (cfg->hash[idx].key != NULL &&
           strcmp(key, cfg->hash[idx].key) != 0)
    {
        ++count;

        idx = (idx + 1) % capacity;
        if (idx == orig_idx)
            return NULL;
    }
    return &cfg->hash[idx];
}

int hash_grow(cfg_t *cfg)
{
    unsigned int i, idx, orig_size, new_size, count;
    keyval_t *new_hash;

    orig_size = 1 << cfg->logsize;
    new_size = orig_size << 1;

    new_hash = (keyval_t *)malloc(sizeof(keyval_t) * new_size);
    if (new_hash == NULL)
        return -1;
    memset(new_hash, 0, sizeof(keyval_t) * new_size);

    for (i = 0; i < orig_size; ++i) {
        count = 1;
        if (cfg->hash[i].key != NULL) {
            idx = hash_function(cfg->hash[i].key, cfg->logsize + 1);
            while (new_hash[idx].key != NULL) {
                ++count;
                idx = (idx + 1) % new_size;
            }
            new_hash[idx].key = cfg->hash[i].key;
            new_hash[idx].val = cfg->hash[i].val;
        }
    }

    free(cfg->hash);
    cfg->hash = new_hash;
    ++cfg->logsize;
    if (cfg->logsize > 16)
        fprintf(stderr, "Warning: Internal hash grew beyond expectation.\n");

    return 0;
}

int hash_insert(cfg_t *cfg, const char *key, const char *val)
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
            hash_grow(cfg); /* Failing to grow the hash is not an error. */
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

int hash_delete(cfg_t *cfg, const char *key)
{
    keyval_t *entry;

    entry = hash_find(cfg, key);
    if (entry == NULL || entry->key == NULL)
        return -1;

    free(entry->key);
    free(entry->val);
    entry->key = NULL;
    entry->val = NULL;
    --cfg->count;
    return 0;
}

void hash_clear(cfg_t *cfg)
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
