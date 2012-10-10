/*
 * Copyright 2003-2012 Jeffrey K. Hollingsworth
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
#include "hglobal_config.h"

typedef struct keyval {
    char *key;
    char *val;
} keyval_t;

/* Internal function prototypes */
unsigned int hash_function(const char *input, unsigned int logsize);
keyval_t *hash_init(unsigned int logsize);
keyval_t *hash_find(const char *key);
int hash_grow(void);
int hash_insert(const char *key, const char *val);
int hash_delete(const char *key);

/* Static global variables */
static const unsigned HASH_GROWTH_THRESHOLD = 50;
static const unsigned CONFIG_INITIAL_LOGSIZE = 8;

keyval_t *cfg_hash;
unsigned int cfg_count;
unsigned int cfg_logsize;

int cfg_init()
{
    cfg_count = 0;
    cfg_logsize = CONFIG_INITIAL_LOGSIZE;
    cfg_hash = hash_init(CONFIG_INITIAL_LOGSIZE);
    return (cfg_hash != NULL);
}

const char *cfg_get(const char *key)
{
    keyval_t *entry;
    entry = hash_find(key);
    if (entry == NULL || entry->key == NULL)
        return NULL;

    return entry->val;
}

int cfg_set(const char *key, const char *val)
{
    if (val == NULL)
        return hash_delete(key);
    return hash_insert(key, val);
}

int cfg_unset(const char *key)
{
    return hash_delete(key);
}

void cfg_write(FILE *fd)
{
    int i;

    for (i = 0; i < (1 << cfg_logsize); ++i) {
        if (cfg_hash[i].key != NULL) {
            fprintf(fd, "%s=%s\n", cfg_hash[i].key, cfg_hash[i].val);
        }
    }
}

/* Open the configuration file, and parse it line by line. */
int cfg_read(FILE *fd)
{
    char buf[4096];
    char *head, *curr, *tail, *key, *val;
    unsigned int retval, linenum = 0;
    keyval_t *entry;

    head = curr = buf;
    while (!feof(fd)) {
        retval = fread(curr, sizeof(char), sizeof(buf) - (curr - buf), fd);
        tail = curr + retval;

        curr = memchr(head, '\n', tail - head);
        while (curr != NULL) {
            ++linenum;
            *curr = '\0';
            if (cfg_parseline(head, &key, &val) != 0) {
                fprintf(stderr, "Error: Line %d: %s.\n", linenum, key);
                fclose(fd);
                return -1;
            }

            if (key != NULL) {
                entry = hash_find(key);
                if (entry != NULL && entry->key != NULL) {
                    fprintf(stderr, "Warning: Line %d: Redefinition of"
                            " configuration key %s.\n", linenum, key);
                }

                if (hash_insert(key, val) != 0) {
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

            if (curr != NULL) {
                head = curr + 1;
            }
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

/* Parse one line of the config file. */
int cfg_parseline(char *line, char **ret_key, char **ret_val)
{
    char *key, *val, *ptr;

    /* Ignore comments. */
    ptr = strchr(line, '#');
    if (ptr != NULL) *ptr = '\0';

    /* Trim leading key whitespace. */
    key = line;
    while (isspace(*key)) ++key;

    /* Line is empty.  Skip it. */
    if (*key == '\0') {
        *ret_key = NULL;
        return 0;
    }

    /* Find the equal sign. */
    val = strchr(key, '=');
    if (val == NULL) {
        *ret_key = "No key/value separator character (=)";
        return -1;
    }

    /* Trim trailing key whitespace. */
    ptr = val - 1;
    while (ptr > key && isspace(*ptr)) *(ptr--) = '\0';

    /* Trim leading value whitespace. */
    *(val++) = '\0';
    while (isspace(*val)) ++val;

    /* Trim trailing value whitespace. */
    ptr = val + strlen(val) - 1;
    while (ptr > val && isspace(*ptr)) *(ptr--) = '\0';

    /* Force key strings to be lowercase and have underscores instead
       of whitespace. */
    ptr = key;
    while (*ptr) {
        if (isspace(*ptr))
            *ptr = '_';
        else
            *ptr = tolower(*ptr);
        ++ptr;
    }

    if (*key == '\0') {
        *ret_key = "Empty key string";
        return -1;
    }

    *ret_key = key;
    *ret_val = val;
    return 0;
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

keyval_t *hash_find(const char *key)
{
    unsigned int idx, orig_idx, capacity, count = 1;

    capacity = 1 << cfg_logsize;
    orig_idx = idx = hash_function(key, cfg_logsize);

    while (cfg_hash[idx].key != NULL &&
           strcmp(key, cfg_hash[idx].key) != 0)
    {
        ++count;

        idx = (idx + 1) % capacity;
        if (idx == orig_idx)
            return NULL;
    }
    return &cfg_hash[idx];
}

int hash_grow(void)
{
    unsigned int i, idx, orig_size, new_size, count;
    keyval_t *new_hash;

    orig_size = 1 << cfg_logsize;
    new_size = orig_size << 1;

    new_hash = (keyval_t *)malloc(sizeof(keyval_t) * new_size);
    if (new_hash == NULL)
        return -1;
    memset(new_hash, 0, sizeof(keyval_t) * new_size);

    for (i = 0; i < orig_size; ++i) {
        count = 1;
        if (cfg_hash[i].key != NULL) {
            idx = hash_function(cfg_hash[i].key, cfg_logsize + 1);
            while (new_hash[idx].key != NULL) {
                ++count;
                idx = (idx + 1) % new_size;
            }
            new_hash[idx].key = cfg_hash[i].key;
            new_hash[idx].val = cfg_hash[i].val;
        }
    }

    free(cfg_hash);
    cfg_hash = new_hash;
    ++cfg_logsize;
    if (cfg_logsize > 16)
        fprintf(stderr, "Warning: Internal hash grew beyond expectation.\n");

    return 0;
}

int hash_insert(const char *key, const char *val)
{
    keyval_t *entry;
    char *new_val;

    entry = hash_find(key);
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

        if (((++cfg_count * 100) / (1 << cfg_logsize)) > HASH_GROWTH_THRESHOLD)
            hash_grow(); /* Failing to grow the hash is not an error. */
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

int hash_delete(const char *key)
{
    keyval_t *entry;

    entry = hash_find(key);
    if (entry == NULL || entry->key == NULL)
        return -1;

    free(entry->key);
    free(entry->val);
    entry->key = NULL;
    entry->val = NULL;
    --cfg_count;
    return 0;
}
