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
#include <sys/mman.h>
#include <sys/stat.h>
#include "hglobal_config.h"

/* Extern'ed variable declaration */
keyval_t *cfg_pair = NULL;
unsigned cfg_pairlen = 0;

/* Internal function prototypes */
int cfg_load(const char *);
int cfg_parse();

/* Static global variables */
static const char CONFIG_DEFAULT_FILENAME[] = "harmony.cfg";
static const unsigned CONFIG_INITIAL_CAPACITY = 32;

static char *cfg_buf = NULL;
static unsigned cfg_buflen;

int cfg_init(const char *filename)
{
  static const char* error_prefix = "<Error: cfg_init @ hserver>";

  if (filename == NULL) 
    {
      filename = getenv("HARMONY_CONFIG");
      if (filename == NULL) 
	{
	  printf("HARMONY_CONFIG environment variable empty."
		 "  Using default.\n");
	  filename = CONFIG_DEFAULT_FILENAME;
	}
    }
    
  if (cfg_load(filename) < 0)
    {
      fprintf(stderr, "%s: harmony config file not found\n", error_prefix);
      return -1;
    }
  if (cfg_parse() < 0)
    {
      fprintf(stderr, "%s: failed to parse harmony config file\n", error_prefix);
      return -1;
    }
  
  return 0;
}

char *cfg_get(const char *key)
{
  int i, cmp;

  i = 0;
  cmp = -1;
  while (i < cfg_pairlen && cfg_pair[i].key != NULL && cmp < 0) 
    {
      cmp = strcmp(cfg_pair[i].key, key);
      if (cmp == 0) 
	return cfg_pair[i].val;	
      ++i;
    }
  return NULL;
}

char *cfg_getlower(const char *key)
{
  char *str, *ptr;

  str = cfg_get(key);
  if (str == NULL) {
    return NULL;
  }

  for (ptr = str; *ptr != '\0'; ++ptr) {
    *ptr = tolower(*ptr);
  }
  return str;
}

/* Load the entire configturation file into memory. */
int cfg_load(const char *filename)
{
  static const char* error_prefix = "<Error: cfg_load @ hglobal_config.c>";
  
  unsigned count = 0;
  int fd, retval;
  struct stat sb;

  if (cfg_buf != NULL) 
    {
      free(cfg_buf);
    }
  
  printf("Loading config file '%s'\n", filename);
  fd = open(filename, O_RDONLY);
  if (fd == -1) 
    {
      fprintf(stderr, "%s: opening file: %s\n", error_prefix, strerror(errno));
      return -1;
    }

  /* Obtain file size */
  if (fstat(fd, &sb) == -1) 
    {
      fprintf(stderr, "%s: during fstat(): %s\n", error_prefix, strerror(errno));
      return -1;
    }
  cfg_buflen = sb.st_size;

  cfg_buf = (char *)malloc(cfg_buflen + 1); /* Make room for an extra char */
  if (cfg_buf == NULL) 
    {
      fprintf(stderr, "%s: during malloc(): %s\n", error_prefix, strerror(errno));
      return -1;
    }

  do {
    retval = read(fd, cfg_buf + count, cfg_buflen - count);
    if (retval < 0) 
      {
	fprintf(stderr, "%s: during read(): %s\n", error_prefix, strerror(errno));
	return -1;
      }
    count += retval;
  } 
  while (count < cfg_buflen);

  *(cfg_buf + cfg_buflen) = '\n';  /* Add a newline at the end of the file */

  if (close(fd) != 0) 
    {
      fprintf(stderr, "%s: during close(): %s. Ignoring.\n", error_prefix, strerror(errno));
    }

  return 0;
}

/* Parse the config file. */
int cfg_parse()
{
  unsigned i, size, linenum;
  char *eof, *head, *tail, *key, *val, *hash, *ptr;
  void *alloc;

  eof = cfg_buf + cfg_buflen;
  size = 0;
  linenum = 0;
  head = cfg_buf;

  /* Parse the pairs inside cfg_buf */
  while (head < eof) {
    ++linenum;

    /* Find the next line boundary. */
    tail = (char *)memchr(head, '\n', eof - head);
    if (tail == NULL) {
      tail = eof;
    }
    *tail = '\0';

    /* Ignore comments. */
    hash = (char *)memchr(head, '#', tail - head);
    if (hash == NULL) {
      hash = tail;
    }

    /* Trim the whitespace on the left side of the line. */
    key = head;
    while (key < hash && (*key == '\0' || isspace(*key)))
      ++key;
    if (key == hash) {
      /* All whitespace line. Ignore it. */
      head = tail + 1;
      continue;
    }

    /* Find the equal sign. */
    val = (char *)memchr(key, '=', hash - key);
    if (val == NULL || val == key) {
      printf("Configuration error line %d: %.*s\n",
	     linenum, tail - head, head);
      return -1;
    }

    /* Trim the whitespace on the right side of the key string. */
    ptr = val;
    *(val++) = '\0';
    while (*ptr == '\0' || isspace(*ptr))
      *(ptr--) = '\0';

    /* Force key strings to be lower-case and have underscores instead
       of whitespace, */
    while (ptr >= key) {
      if (*ptr == '\0' || isspace(*ptr)) {
	*(ptr--) = '_';
      } else {
	*(ptr--) = tolower(*ptr);
      }
    }

    /* Trim the whitespace on the left side of the value string. */
    while (val < hash && (*val == '\0' || isspace(*val)))
      ++val;

    /* Trim the whitespace on the right side of the value string. */
    ptr = hash;
    *ptr = '\0';
    while (ptr > val && (*ptr == '\0' || isspace(*ptr)))
      *(ptr--) = '\0';

    /* Increase the capacity of the cfg_pair array, if needed. */
    if (size >= cfg_pairlen) {
      cfg_pairlen *= 2;
      if (cfg_pairlen == 0) {
	cfg_pairlen = CONFIG_INITIAL_CAPACITY;
      }
      alloc = realloc(cfg_pair, sizeof(keyval_t) * cfg_pairlen);
      if (alloc == NULL) {
	printf("Error on realloc(): %s\n", strerror(errno));
	return -1;
      }
      cfg_pair = (keyval_t *)alloc;
    }

    /* Insert the new configuration pair in sorted order. */
    for (i = size; i > 0; --i) {
      if (strcmp(cfg_pair[i - 1].key, key) < 0) {
	break;
      }
      cfg_pair[i] = cfg_pair[i - 1];
    }

    if (i < size && strcmp(cfg_pair[i+1].key, key) == 0) {
      printf("Warning: Configuration key '%s'"
	     " overwritten on line %d.\n", key, linenum);
    }
    cfg_pair[i].key = key;
    cfg_pair[i].val = val;
    ++size;

    head = tail + 1;
  }

  /* Mark the end of valid data within the cfg_pair array. */
  if (size < cfg_pairlen) {
    cfg_pair[size].key = NULL;
  }

  return size;
}
