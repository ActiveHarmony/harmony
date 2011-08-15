#include "hglobal_config.h"
#include <iostream>
#include<string.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
using namespace std;

char global_cfg_path[256];
char *cfg_path=getenv("HARMONY_CONFIG");

FILE *global_cfg;

char delimiter[]="=\n";
char *first,*remainder,*context;
char data[256];

char *config_data[512];

const int MAX=20;

char *new_mem[MAX];

const int arr_size_max = 1000;

char server_port[512];

char hs_config_file[512];

char search_algo[512];

char appname_tuning[512];

char newcode_path[512];

char hs_pid_path[512];

char cgen[512];

char cgenhosts[512];

char Host_Name[512];

char userhomepath[512];

char confsdirpath[512];

char cgenbasepath[512];

char codegenloc[512];

char codedesthostname[512];

char codedestpath[512];

char codeflagdest[512];

char sopathprefix[512];

char sopathprefixdef[512];

char sopathprefixcode[512];


char *read_cfg_file()
{
  char item[512];
  char val[512];
  first = item; 
  context = val;
  
  if(cfg_path != NULL)
  {
    printf("The path set by the user for global_config file: %s\n", cfg_path);
  }
  else 
  {
    printf("The path has not been set for the global_config file \n");
    printf("The application uses the default PATH value for global_config file \n");
    cfg_path = "./global_config";
    printf("The PATH used by the application is: %s \n", cfg_path);
  } 

  sprintf(global_cfg_path, "%s", cfg_path);
  
  global_cfg = fopen(global_cfg_path, "r");

  while(!feof(global_cfg))
  {
    fgets(data, sizeof data, global_cfg);

    if( strlen(data) > 0 && data[strlen(data)-1] == '\n')
       data[strlen(data)-1] = 0;

    if(data[0]!='#' && data[0]!=0)
    {
       
      strcpy( item, strtok_r(data, delimiter, &remainder));
      strcpy( val, strtok_r(NULL, delimiter,  &remainder)); 
    }
    else
    {
      continue;
    }

    if(!strcmp(first,"Server_Port"))
    {
      strcpy(server_port,context);
    }
    else if(!strcmp(first,"hs_configuration_file"))
    {
      strcpy(hs_config_file,context);
    }
    else if(!strcmp(first,"search_algorithm"))
    {
      strcpy(search_algo,context);
    }
    else if(!strcmp(first,"app_name_tuning"))
    {
      strcpy(appname_tuning,context);
    }
    else if(!strcmp(first,"new_code_path"))
    {
      strcpy(newcode_path,context);      
    }
    else if(!strcmp(first,"hspid_path"))
    {
      strcpy(hs_pid_path,context); 
    }
    else if(!strcmp(first,"total_code_generators"))
    {
      strcpy(cgen,context); 
    }
    else if(!strcmp(first,"total_generator_hosts"))
    {
      strcpy(cgenhosts,context); 
    }
    else if(!strcmp(first,"hostname"))
    {
      strcpy(Host_Name,context); 
    }
    else if(!strcmp(first,"user_home"))
    {
     strcpy(userhomepath,context); 
    }
    else if(!strcmp(first,"confs_dir"))
    {
      strcpy(confsdirpath,context); 
    }
    else if(!strcmp(first,"code_generator_base"))
    {
      strcpy(cgenbasepath,context); 
    }
    else if(!strcmp(first,"num_code_gen_loc"))
    {
      strcpy(codegenloc,context); 
    }
    else if(!strcmp(first,"hostname"))
    {
      strcpy(codedesthostname,context); 
    }
    else if(!strcmp(first,"code_dest_path"))
    {
      strcpy(codedestpath,context); 
    }
    else if(!strcmp(first,"code_flag_destination_path"))
    {
      strcpy(codeflagdest,context); 
    }
    else if(!strcmp(first,"path_prefix_run_dir"))
    {
      strcpy(sopathprefix,context); 
    }
    else if(!strcmp(first,"path_prefix_def"))
    {
      strcpy(sopathprefixdef,context); 
    }
    else if(!strcmp(first,"path_prefix_code"))
    {
      strcpy(sopathprefixcode,context); 
    }
  
  } 

     
  fclose(global_cfg);

  /*
  printf("Server-Port %s\n", server_port);
  printf("HS_CFG_FILE %s\n", hs_config_file);
  printf("SEARCH ALGO %s\n", search_algo);
  printf("APNAME %s\n", appname_tuning);
  printf("NEWCODE PATH %s\n",newcode_path);
  printf("HSPID_PATH %s\n", hs_pid_path);
  printf("CGEN %s\n",cgen);
  printf("CGHOSTS %s\n",cgenhosts);
  printf("HOSTNAME :%s\n",Host_Name);
  printf("UHOMEPATH: %s\n",userhomepath);
  printf("CFSDIRPATH :%s\n",confsdirpath);
  printf("CGBP :%s\n", cgenbasepath);
  printf("CGLOC :%s\n",codegenloc);
  printf("CDHN :%s\n", codedesthostname);
  printf("CDP :%s\n", codedestpath);
  printf("CFD :%s\n", codeflagdest);
  printf("SPP :%s\n", sopathprefix);
  printf("SPPDEF :%s\n", sopathprefixdef);
  printf("SPPCODE :%s\n",sopathprefixcode);
  */

}


char *server_portnum()
{
  return server_port;
}

/*****Extract the corresponding configuration file from "global_config" file.*****/
char *get_config_file()
{
  return hs_config_file;
}

/*****Extract the search algorithm used to harmonize the application from the "global_config" file.*****/
char *get_search_algorithm()
{
  return search_algo;
}

/*****Extract the application name used for tuning from the "global_config" file.*****/
char *get_application_name()
{
   return appname_tuning;
}

/*********Extract the path to the new codes that are being generated***********/
/************* by the code generators from the "global_config" file.***********/
char *get_new_code_path()
{
   return newcode_path;
}

/*****Extract the harmony server's PID path from the "global_config" file.*****/
char *get_hspid_path()
{ 
  return hs_pid_path;
}

/*****Extract the "total_code_generators" from the "global_config" file.*****/
char *code_generators()
{
  return cgen;
}

/*****Extract the "code_generator_hosts" from the "global_config" file.*****/
char *code_generator_hosts()
{
  return cgenhosts;
}

/*****Extract the "total_code_generators" from the "global_config" file.*****/
char *host_name()
{
   return Host_Name;
}

/*****Extract the "user_home" path from the "global_config" file.*****/
char *user_home_path()
{
  return userhomepath;
}

/*****Extract the "confs_dir" path from the "global_config" file.*****/
char *confs_dir_path()
{
  return confsdirpath;
}

/*****Extract the "code_generator_base" path from the "global_config" file.*****/
char *code_generator_base_path()
{
  return cgenbasepath;
}

/*****Extract the "num_code_generators" from the "global_config" file.*****/
char *num_code_gen_loc_path()
{
  return codegenloc;
}

/*****Extract the "num_code_generators" from the "global_config" file.*****/
char *code_dest_host_name()
{
  return Host_Name;
}

/*****Extract the "num_code_generators" from the "global_config" file.*****/
char *code_destination_path()
{
  return codedestpath;
}

/*****Extract the "code flags destination path" from the "global_config" file.*****/
char *code_flag_dest_path()
{
  return codeflagdest;
}

/*****Extract all the code-generator related paths.*****/
char *all_code_gen_paths()
{
  char p1[512], p2[512], p3[512], p4[512], p5[512], p6[512];

  strcpy(p1, Host_Name);
  strcpy(p2, confsdirpath);
  strcpy(p3, codedestpath);
  strcpy(p4, codeflagdest);
  strcpy(p5, server_port);

  sprintf(p6, "%s \n%s \n%s \n%s \n%s", p1, p2, p3, p4, p5);
  
  return p6;
}


/*****Extract the "path_prefix_run_dir" path from the "global_config" file.*****/
char *so_path_prefix_run()
{
  return sopathprefix;
}

/*****Extract the "path_prefix_def" path from the "global_config" file.*****/
char *so_path_prefix_def()
{
  return sopathprefixdef;
}

/*****Extract the "path_prefix_code" path from the "global_config" file.*****/
char *so_path_prefix_code()
{
  return sopathprefixcode;
}

















