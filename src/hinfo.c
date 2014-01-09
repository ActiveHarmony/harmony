#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <limits.h>

#include "session-core.h"
#include "hsession.h"
#include "hcfg.h"
#include "defaults.h"
#include "hmesg.h"
#include "hsockutil.h"
#include "hutil.h"

#define my_name "hinfo"

static int verbose_flag, list_flag, home_flag;

/* count and list of expected symbols
   for a strategy */
static int strategy_symbol_count = 9;
const char * const strategy_symbols[] = {
  "strategy_generate",
  "strategy_rejected",
  "strategy_analyze",
  "strategy_best",
  "strategy_init",
  "strategy_join",
  "strategy_getcfg",
  "strategy_setcfg",
  "strategy_fini"
};

/* indicates which strategy hooks are 
   actually required for the .so file
   to be a valid strategy */
static int strategy_required[] = {
  1,  /* strategy_generate */
  1,  /* strategy_rejected */
  1,  /* strategy_analyze  */
  1,  /* strategy_best     */
  1,  /* strategy_init     */
  0,  /* strategy_join     */
  0,  /* strategy_getcfg   */
  0,  /* strategy_setcfg   */
  0   /* strategy_fini     */
};

static int layer_suffix_count = 7;
const char * const layer_suffixes[] = {
  "_init",
  "_join",
  "_generate",
  "_analyze",
  "_getcfg",
  "_setcfg",
  "_fini"
};

static int layer_required[] = {
  1,  /* _init      */
  0,  /* _join      */
  0,  /* _generate  */
  0,  /* _analyze   */
  0,  /* _getcfg    */
  0,  /* _setcfg    */
  0   /* _fini      */
};

/* attempts to figure out where libexec dir is 
   possibly returns incorrect path
   assumes that the calling program is 'hinfo'
*/
char *find_harmony_home(char *argv_str) {
  char *str = NULL;   /* used for temporary strings. I modify strings to parse them so yeah */
  char *cur_path = NULL; /* for current path being looked at in third case */
  DIR *cur_dir = NULL;/* look up one line ^ */
  struct dirent *entry;   /* current file in path being searched (third case */
  int colon_count = 0;/* what you think it is */
  int i, c;           /* one letter vars for loop stuff */
  int *path_indices;  /* array of ints indicating beginnings of individual paths */

  /* first check environment variable */
  char *home = getenv("HARMONY_HOME");
  if(home != NULL) {
    if(verbose_flag || home_flag) 
      printf("HARMONY_HOME is set.\n");
    str = malloc(strlen(home) + 8 + 1); /* home + "/libexec" + \0 */
    strcpy(str, home);
    return str;
  }
  /* see if argv has path in it */
  else if(strncmp(argv_str, "hinfo", 5) != 0) {
    printf("HARMONY_HOME not set. Using path hinfo was called with to locate HARMONY_HOME\n");
    str = malloc(strlen(argv_str) + 7 + 1);  /* ...hinfo, replace hinfo with "/../libexec" */
    strcpy(str, argv_str);
    strcpy(str + strlen(argv_str) - strlen(my_name), "/../");
    return str;
  } else { /* need to check path */
    char *path = getenv("PATH");
    printf("HARMONY_HOME not set. Searching path and inferring HARMONY_HOME from hinfo's location\n");
    str = malloc(strlen(path) + 1); 
    strcpy(str, path);
    cur_path = malloc(strlen(path) + 11 + 1); /* max single path + "/../libexec" */

    /* count colons in path */
    for(i = 0;i < strlen(path);i++) {
      if(str[i] == ':') colon_count++;
    }
    /* now allocate memory for an array of ints to mark
       beginnings of individual paths */
    path_indices = malloc(sizeof(int) * (colon_count + 1));
    path_indices[0] = 0;
    for(i = 0, c = 1;i < strlen(path);i++) {
      if(str[i] == ':') {
        path_indices[c++] = i + 1;
        str[i] = '\0';
      }
    }
    /* search each possibility for hinfo */
    for(i = 0;i < colon_count + 1;i++) {
      strcpy(cur_path, str + path_indices[i]);
      cur_dir = opendir(cur_path);
      if(cur_dir != NULL) {  /* read every directory entry looking for "hinfo" */
        while(1) {
          entry = readdir(cur_dir);
          if(entry == NULL) break;
          if(strncmp(entry->d_name, my_name, 256) == 0) {
            strcpy(cur_path + strlen(cur_path), "/../");
            free(str);
            free(path_indices);
            return cur_path;
          }
        }
      } 
    }
  }
  if(path_indices != NULL) free(path_indices);
  if(cur_path != NULL) free(cur_path);
  if(str != NULL) free(str);
  return NULL; /* no hinfo found in PATH??? */
}

/* Given a possibly null terminated string and a number,
   returns the length of the string up to the null terminator
   or the number if a null terminator isn't reached by then. */
int clam_strnlen(char *cupcake, int max_sprinkles) {
  int sugar;
  for(sugar = 0;sugar <= max_sprinkles;sugar++) {
    if(cupcake[sugar] == '\0') break;
  }
  return sugar;
}

int main(int argc, char *argv[]) {
  int option_index = 0, c = 0;
  char *harmony_home_path = NULL, *so_filename = NULL;
  DIR *libexec_dir = NULL;
  struct dirent *entry = NULL;
  void *dl_handle = NULL;
  char layer_symbol[1000], real_path[PATH_MAX];  
  char *layer_prefix = NULL;
  int layer_prefix_len = 0;
  int valid_strategy = 0, valid_layer = 0;

  static struct option long_options[] = {
    {"list", 0, &list_flag, 'l'},
    {"home", 0, &home_flag, 'h'},
    {"verbose", 0, &verbose_flag, 'v'},
    {0, 0, 0, 0}
  };

  while(1) { 
    c = option_index = getopt_long(argc, argv, "lhv", long_options, &option_index);
    if(c == -1) break;
    switch(c) { 
      case 'v':
        verbose_flag = 1;
        break;
      case 'l':
        list_flag = 1;
        break;
      case 'h':
        home_flag = 1;
        break;
      default: 
        continue;
    }
  }
  
  if(argc < 2 || (! list_flag && ! home_flag)) {
    printf("Usage: hinfo [options]\n");
    printf("Options\n\t--list\tList .so files (potential strategies and layers)\n");
    printf("\t--home\tJust verify presence of and print out libexec directory\n");
    printf("\t--verbose\tPrint info about which hooks are present\n");
    return 0;
  }

  harmony_home_path = find_harmony_home(argv[0]);
  
  if(harmony_home_path == NULL) {
    printf("Unable to locate libexec path\n");
    goto end;  
  } 
  
  realpath(harmony_home_path, real_path);

  //if(verbose_flag || home_flag)                      
    printf("Harmony home: %s\n", real_path); 

  /* append libexec to end of real path */
  strcpy(real_path + strlen(real_path), "/libexec");
  
  libexec_dir = opendir(real_path);
  
  if(libexec_dir == NULL) {
    printf("Error opening libexec dir\n");
    goto end;
  }

  if(! list_flag) goto end;
                        
  /* filename max length = 255. + 2 for \0, slash*/
  so_filename = malloc(strlen(real_path) + 257); 
  while(1) {
    entry = readdir(libexec_dir);
    if(entry == NULL) break;
    if(strstr(entry->d_name, ".so") != NULL) {
      /* attempt to load shared library */
      strcpy(so_filename, real_path);
      so_filename[strlen(real_path)] = '/';
      strcpy(so_filename + strlen(real_path) + 1, entry->d_name);
      printf("%s:", entry->d_name);
      if(verbose_flag) printf("\n");
      dl_handle = dlopen(so_filename, RTLD_LAZY | RTLD_LOCAL);
      if(dl_handle == NULL) {
        printf("Error opening %s: %s\n", entry->d_name, dlerror());
        continue;
      }
      /* see if it's a layer (if it has a layer name) */
      layer_prefix = (char *) dlsym(dl_handle, "harmony_layer_name");
      if(layer_prefix != NULL) {
        if(verbose_flag) printf("\tharmony_layer_name present, assuming this a layer\n");
        layer_prefix_len = clam_strnlen(layer_prefix, 1000);

        if(layer_prefix_len == 1000) {
          printf("Layer name too long\n");
          continue;
        }

        valid_layer = 1;
        strcpy(layer_symbol, layer_prefix);
        for(c = 0;c < layer_suffix_count;c++) {
          strcpy(layer_symbol + layer_prefix_len, layer_suffixes[c]); 
          if(dlsym(dl_handle, layer_symbol) != NULL) {
            if(verbose_flag) printf("\t%s present\n", layer_symbol);
          } else {
            if(verbose_flag) printf("\t%s not present\n", layer_symbol);
            if(layer_required[c]) {
              if(verbose_flag) printf("This hook is required\n");
              valid_layer = 0;
            }
          }
        }
        if(valid_layer) {
          printf("\tvalid layer\n");
        }
        continue;
      }
      if(verbose_flag) printf("\tAssuming this is a strategy\n");
      valid_strategy = 1;
      for(c = 0;c < strategy_symbol_count;c++) {
        if(dlsym(dl_handle, strategy_symbols[c]) != NULL) {
          if(verbose_flag) printf("\t%s present\n", strategy_symbols[c]);
        } else {
          if(verbose_flag) printf("\t%s not present\n", strategy_symbols[c]);
          if(strategy_required[c]) {
            if(verbose_flag) printf("This hook is required\n");
            valid_strategy = 0;
          }
        }
      }
      if(valid_strategy) {
        printf("\tvalid strategy\n");
      }
      dlclose(dl_handle);
    }
  }

  end:
  if(harmony_home_path != NULL) free(harmony_home_path);
  if(so_filename != NULL) free(so_filename);
  return 0;
}
