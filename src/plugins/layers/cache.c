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

/**
 * \page cache Point Caching/Replay layer (cache.so)
 *
 * This processing layer takes a log of point/performance pairs as generated
 * by log.so. If points requested by the strategy are present in the log
 * file, this layer returns the point/performance pair in the log to the strategy
 * instead of sending the point out to get tested by the client. Otherwise,
 * the requested point is sent out for normal testing by the client(s).
 * Note that the client and the Harmony server are not informed about points
 * found in the cache. 
 *
 * **Configuration Variables**
 * Key          | Type   | Default | Description
 * ------------ | ------ | ------- | -----------
 * CACHE_FILE   | String | (none)  | Name of point/performance log file.
 */

#include "session-core.h"
#include "hsignature.h"
#include "hpoint.h"
#include "hutil.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

const char harmony_layer_name[] = "cache";

unsigned int n_sections = 0;
unsigned int n_rows = 0;
double *data = NULL;               /* sections x n_rows 2d array */

/* Counts number of commas in string. Terminates either
   on null character or after maxlen characters 
   have been processed */
unsigned int count_commas(const char *str, int maxlen) {
  unsigned int count = 0, i = 0;
  for(i = 0;i < maxlen;i++) {
    if(str[i] == '\0') break;
    if(str[i] == ',') count++;
  }
  return count;
}

/* Takes a string, replaces all commas with \0,
   and populates the given integer array with starting
   indices of substrings */
void replace_commas(char *str, unsigned int maxlen, unsigned int *arr) {
  unsigned int i, c;
  c = 1;
  arr[0] = 0;
  for(i = 0;i < maxlen;i++) {
    if(str[i] == ','){
      str[i] = '\0';
      arr[c] = i + 1;
      c++;
    }
  }
}

/* init function: loads and parses file specified by CACHE_FILE */
int cache_init(hsignature_t *sig) {
  const char *filename;
  char line[1024];
  unsigned int sections = 0, *section_idx = NULL, i;
  unsigned int line_idx = 0;
  FILE *cache_file = NULL;
  
  filename = session_getcfg("CACHE_FILE");
  if(! filename) {
    session_error("CACHE_FILE config key empty");
  	return -1;
}
  
  cache_file = fopen(filename, "r");
  if(! cache_file) {
    session_error("Could not open file %s for reading\n");
  	return -1;
  }
  
  /* read everything into a huge array. */
  line_idx = 0;
  while(1) {
    memset(line, 0, sizeof(line));
    if(fgets(line, sizeof(line), cache_file) != NULL) {
      /* parse line. It must start with a number */
      if(line[0] >= 48 && line[0] <= 57) {
        sections = count_commas(line, sizeof(line));
        if(n_sections < 2) {   /* number of coordinates not yet known */
          /* figure out number of coordinates */
          if(sections  < 2) continue; /* not valid line */
          section_idx = malloc(sizeof(unsigned int) * sections);
          data = malloc(sizeof(double) * sections);
          n_sections = sections;
        }
        if(sections != n_sections) continue; /* wrong number of coords = invalid line */
        /* allocate additional memory, fill in data from line */
        if(line_idx == n_rows) {
          data = realloc(data, sizeof(double) * sections * (line_idx + 1));
          if(data == NULL) {
            session_error("Cannot allocate memory\n");
          }
          n_rows = line_idx + 1;
        }
        memset(section_idx, 0, sizeof(unsigned int) * sections);
        replace_commas(line, sizeof(line), section_idx);
        /* start at index 1, because first section is pt id */
        for(i = 1;i <= n_sections;i++) {
          data[line_idx * sections + (i - 1)] = strtod(line + section_idx[i], NULL);
        }
        line_idx++;
      } else continue;
  	} else break;
  }
  fclose(cache_file);

  free(section_idx);
  return 0;
}

/*
 * Generate function: look up point in array.
 * Sets status to HFLOW_RETURN with trial's performance set to retrieved value
 * if the point is found.
 * Otherwise, sets status to HFLOW_ACCEPT (pass point on in plugin workflow)
 */
int cache_generate(hflow_t *flow, htrial_t *trial) {
  unsigned int i, c;
  const hpoint_t *pt = &trial->point;
  
  if(pt->n != n_sections - 1) {
    /* wrong number of coordinates */
    session_error("Invalid number of coords (mismatch between file, point\n");
    return -1;
  }
  
  /* lookup in data */
  flow->status = HFLOW_ACCEPT;
  for(i = 0;i < n_rows;i++) {
    for(c = 0;c < pt->n;c++) {
      if(pt->val[c].type != HVAL_REAL) {
        session_error("Non-real coords not supported\n");
        return -1;
      }
      if(pt->val[c].value.r != data[i * n_sections + c]) break;
    }
    if(c == pt->n) {
      trial->perf = data[i * n_sections + c];
      flow->status = HFLOW_RETURN;
      return 0;
    }
  }
  /* Data point not found */
  printf("Data point not found!\n");
  flow->status = HFLOW_ACCEPT;
  return 0; 
}

int cache_fini(void) {
  if(data != NULL) free(data);
  return 0;
}
