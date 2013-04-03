#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include "taudb_api.h"


#include "session-core.h"
#include "hmesg.h"
#include "hpoint.h"
#include "hutil.h"
#include "hcfg.h"
#include "hsockutil.h"
#include "defaults.h"

#define CLIENTID_INIT 16
#define SECONDARY_METADATA_NUM 10
#define REG_STR_LEN 32
#define MAX_STR_LEN 1024
#define METADATA_ID_LEN 15
#define POINTS_PER_SAVE 20

char harmony_plugin_name[] = "tauDB";

static TAUDB_METRIC *metric;
static TAUDB_TRIAL * trial;
static TAUDB_CONNECTION * connection;
static TAUDB_THREAD * thread;
static TAUDB_TIMER_GROUP *timer_group;

static hsession_t session;
static int paramNum;
static int nodeNum;
static int *clientMap; //A table to manage clientID vs socketID
static int clientIdx = 0;
static int save_counter = 0;
static int *timer_id;
static int taudb_store_type; //1 for real time, 0 for one-time
static int total_record_num = 0;

/*Several Pre-declaration*/
int get_config_dim(char *config);
TAUDB_TIMER* save_timer_parameter(TAUDB_TIMER* timer, hmesg_t *mesg);
TAUDB_TRIAL* harmony_taudb_trial_init(const char *appName, const char *trialname);
void harmony_taudb_insert(hmesg_t *mesg);
TAUDB_THREAD* harmony_taudb_create_thread(int threadNum);
void harmony_taudb_get_secondary_metadata(TAUDB_THREAD* thread, char* opsys, char* machine, char* release, char* hostname, char* procnum, char* cpuvendor, char* cpumodel, char* clkfreq, char* cachesize);

/*Initialization of TauDB
 *Return 0 for success, -1 for error
 */
int tauDB_init(hmesg_t *mesg) {	
	char taudb_name[32];
	printf("Initializing session %s\n", mesg->data.session.sig.name);	

	/*Connecting to TauDB*/
	snprintf(taudb_name, sizeof(taudb_name), "%s", hcfg_get(mesg->data.session.cfg, "TAUDB_NAME"));
    if (taudb_name !=  NULL) {
		printf("Connecting to TauDB.\n");
		connection = taudb_connect_config(taudb_name);
    } else { 
		fprintf(stderr, "TAUdb config file not found! Connection failed.\n");
		return -1;
    }

    /*Check if the connection has been established*/
    taudb_check_connection(connection);
	printf("Connected to TauDB.\n");

	
	/*Initializing Trial*/
	trial = harmony_taudb_trial_init(mesg->data.session.sig.name, mesg->data.session.sig.name);
	if (trial == NULL) {
		fprintf(stderr, "Failed to create TauDB trial.\n");
		return -1;
	}

    /*Create a metric*/
    metric = taudb_create_metrics(1);
    metric->name = taudb_strdup("TIME");
    taudb_add_metric_to_trial(trial, metric);

	/*Initializing Timer group*/
	timer_group = taudb_create_timer_groups(1);
	timer_group->name = taudb_strdup("Harmony Perf");
	taudb_add_timer_group_to_trial(trial, timer_group);

	char nodeNumStr[10];
	paramNum = mesg->data.session.sig.range_cap;
	snprintf(nodeNumStr, 10, "%s", hcfg_get(mesg->data.session.cfg, CFGKEY_CLIENT_COUNT));

	nodeNum = atoi(nodeNumStr);
	trial->node_count = nodeNum;

	char taudb_store[10];
	snprintf(taudb_store, sizeof(taudb_store), "%s", hcfg_get(mesg->data.session.cfg, "TAUDB_STORE_METHOD"));
	if (!strcmp(taudb_store, "real_time")){
		taudb_store_type = 1;
		snprintf(taudb_store, sizeof(taudb_store), "%s", hcfg_get(mesg->data.session.cfg, "TAUDB_STORE_NUM"));
		total_record_num = atoi(taudb_store);
	}
	else if (!strcmp(taudb_store, "one_time")){
		taudb_store_type = 0;
		snprintf(taudb_store, sizeof(taudb_store), "%s", hcfg_get(mesg->data.session.cfg, "TAUDB_STORE_NUM"));
		total_record_num = atoi(taudb_store);
	}

	/*Socket id map to thread id*/
	thread = harmony_taudb_create_thread(nodeNum);
	printf("Number of threads is %d\n", nodeNum);
	clientMap = (int*)malloc(sizeof(int)*(2*nodeNum + CLIENTID_INIT));
	for (int i = 0; i < 2*nodeNum+CLIENTID_INIT; i++) {
		clientMap[i] = -1;
	}
	clientMap[mesg->dest] = clientIdx;

	/*Timer counter for each thread*/
	timer_id = (int*)malloc(sizeof(int) * nodeNum);
	for (int i = 0; i < nodeNum; i++) timer_id[i] = 0;

	/*Inserting metadata to the trial*/
	hsession_copy(&session, &mesg->data.session);
	return 0;
}

/*Invoked upon client reports performance
 *This routine stores the reported performance to TauDB
 */
int tauDB_report(hmesg_t *mesg) {
	if (clientMap[mesg->dest] == -1) {
		clientMap[mesg->dest] = ++clientIdx;
	}
	harmony_taudb_insert(mesg);
	return 0;
}

int tauDB_inform(hmesg_t *mesg) {
	char *inform_key;
	char *inform_val;
	char inform_compare_key[METADATA_ID_LEN];

	char nodeName[REG_STR_LEN];
	char sysName[REG_STR_LEN];
	char release[REG_STR_LEN];
	char machine[REG_STR_LEN];
	char core_num[REG_STR_LEN];
	char cpu_vendor[REG_STR_LEN];
	char cpu_model[REG_STR_LEN];
	char cpu_freq[REG_STR_LEN];
	char cache_size[REG_STR_LEN];
	int procNum;

    if (clientMap[mesg->dest] == -1) {
		clientMap[mesg->dest] = ++clientIdx;
    }

	printf("Inform is Invoked %s.\n", mesg->data.string);
	hcfg_parse((char *)mesg->data.string, &inform_key, &inform_val);
	if (strlen(inform_key) >= METADATA_ID_LEN) {
		snprintf(inform_compare_key, METADATA_ID_LEN+1, "%s", inform_key);
		printf("Metadata compare key is %s\n", inform_compare_key);
		if (!strcmp(inform_compare_key, "metadata_client")) {
			sscanf(inform_val, "%[^$$]$$%[^$$]$$%[^$$]$$%[^$$]$$%d$$%[^$$]$$%[^$$]$$%[^$$]$$%[^$$]\n", nodeName, sysName, release, machine, &procNum, cpu_vendor, cpu_model, cpu_freq, cache_size);
			snprintf(core_num, sizeof(core_num), "%d", procNum);
			printf("Metadata is %s %s %s %s\n", nodeName, sysName, machine, release);
			harmony_taudb_get_secondary_metadata(&thread[clientMap[mesg->dest]], sysName, machine, release, nodeName, core_num, cpu_vendor, cpu_model, cpu_freq, cache_size);
		}
	}
	return 0;
}
/*Invoked upon client requests configuration
 *This routine is only used for mapping sockfd with clientID
 *Returns 1 as success
 */
/*int tauDB_fetch(hmesg_t *mesg) {
	if (mesg->dest > -1) {
		if (clientMap[mesg->dest] == -1) {
			clientMap[mesg->dest] = ++clientIdx;
		}
	}
	printf("ClientMap index %d, value %d\n", mesg->dest, clientMap[mesg->dest]);
	return 1;
}*/


/*----------------------------------------------------------
 *All functions below are internal functions
 */

/*Insert a config-pair to TauDB*/
void harmony_taudb_insert(hmesg_t *mesg) {
	int clientID;
	char timer_name[REG_STR_LEN];
	/*Get client info*/
	clientID = clientMap[mesg->dest];
	
	printf("Incoming socket is %d %d\n", mesg->dest, clientID);	

	//taudb_save_metrics(connection, trial, 1);

    /*create a timer*/
    TAUDB_TIMER* timer = taudb_create_timers(1);
    TAUDB_TIMER_VALUE* timer_value = taudb_create_timer_values(1);
    TAUDB_TIMER_CALLPATH* timer_callpath = taudb_create_timer_callpaths(1);
    TAUDB_TIMER_CALL_DATA* timer_call_data = taudb_create_timer_call_data(1);
	//TAUDB_TIMER_GROUP *timer_group = taudb_create_timer_groups(1);

    /*parse input string and get param information*/
    printf("Parsing input string and get param info.\n");
    
	//timer_group->name = taudb_strdup("Harmony Perf");

	++timer_id[clientID];
	printf("Client ID is %d, timer id is %d\n", clientID, timer_id[clientID]);
	snprintf(timer_name, sizeof(timer_name), "Runtime_%d_%d", clientID, timer_id[clientID]);
		
    timer->name = taudb_strdup(timer_name);

	taudb_add_timer_to_trial(trial, timer);
	
	taudb_add_timer_to_timer_group(timer_group, timer);
    

    timer_callpath->timer = timer;
    timer_callpath->parent = NULL;
    taudb_add_timer_callpath_to_trial(trial, timer_callpath);

    timer_call_data->key.timer_callpath = timer_callpath;
    timer_call_data->key.thread = &thread[clientID];
    timer_call_data->calls = 1;
    timer_call_data->subroutines = 0;
    taudb_add_timer_call_data_to_trial(trial, timer_call_data);

    timer_value->metric = metric;
	timer_value->inclusive = mesg->data.report.perf;
    timer_value->exclusive = mesg->data.report.perf;
	timer_value->inclusive_percentage = 100.0;
	timer_value->exclusive_percentage = 100.0;
	timer_value->sum_exclusive_squared = 0.0;
    taudb_add_timer_value_to_timer_call_data(timer_call_data, timer_value);
    


    //printf("Loading data.\n");
    timer = save_timer_parameter(timer, mesg);

    /*Save the trial*/
    printf("Saving timer in the trial.\n");
    boolean update = 1;
    boolean cascade = 1;
	
	if (taudb_store_type == 0){
		if (save_counter < total_record_num) {
			save_counter++;
		} else {
			taudb_compute_statistics(trial);
		    taudb_save_trial(connection, trial, update, cascade);
			taudb_store_type = -1;	
		}
	} else if (taudb_store_type == 1){
		if (save_counter < total_record_num)
			save_counter++;
		else {
			taudb_save_trial(connection, trial, update, cascade);
			save_counter = 0;
		}
	}
}

TAUDB_TIMER* save_timer_parameter(TAUDB_TIMER* timer, hmesg_t *mesg) {
	char value[32];
	long min_i, step_i, val_i;
	double min_r, step_r, val_r;
	int index;
	TAUDB_TIMER_PARAMETER* timer_parameter = taudb_create_timer_parameters(paramNum);
	for (int i = 0; i < paramNum; i++) {
		/*Save name*/
		timer_parameter[i].name = taudb_strdup(session.sig.range[i].name);
		/*get value*/
		index = mesg->data.report.cand.idx[i];
		switch (session.sig.range[i].type) {
			case HVAL_INT:
				min_i = session.sig.range[i].bounds.i.min;
				step_i = session.sig.range[i].bounds.i.step;
				val_i = min_i + step_i * index;
				snprintf(value, sizeof(value), "%ld", val_i);
				break;

			case HVAL_REAL:
				min_r = session.sig.range[i].bounds.r.min;
				step_r = session.sig.range[i].bounds.r.step;
				val_r = min_r + step_r * index;
				snprintf(value, sizeof(value), "%f", val_r);
				break;
			
			case HVAL_STR:
				 snprintf(value, sizeof(value), "%s", session.sig.range[i].bounds.e.set[index]);
				 break;

			case HVAL_UNKNOWN:
			case HVAL_MAX:
				break;
		}
		timer_parameter[i].value = taudb_strdup(value);
		taudb_add_timer_parameter_to_timer(timer, (&timer_parameter[i]));
	}

	return timer;
}

/*get the number of parameters*/
int get_config_dim(char *config) {
    int i = 0;
    int count = 0;
    while (i < strlen(config)) {
	i++;
	if (isspace(config[i])) count++;
    }
    return count;
}

/*Check if a duplicate clientID-config pair is existing in the db*/
//int taudb_check_duplicate();

/*Update the duplicate*/
//int taudb_update();

/*TBD*/
//int taudb_query();

/*Connect to database*/
TAUDB_CONNECTION* harmony_taudb_connect(char *config) {
    //TAUDB_CONNECTION* connection = NULL;
    if (config !=  NULL) {
	connection = taudb_connect_config(config);
    } else { 
	fprintf(stderr, "TAUdb config file not found! Connection failed.\n");
	return NULL;
    }

    /*Check if the connection has been established*/
    taudb_check_connection(connection);
    return connection;
}

/*Initialize a trial*/
TAUDB_TRIAL* harmony_taudb_trial_init(const char *appName, const char *trialname) {
    
    char startTime[32];
    struct tm *current;
    time_t now;

    /*Check if the connection has been established*/
    taudb_check_connection(connection); 

    /*Create a new trial*/
    printf("Creating new trial.\n");
    trial = taudb_create_trials(1);
    trial->name = taudb_strdup(trialname);
    
    /*set the data source to other*/
    trial->data_source = taudb_get_data_source_by_id(taudb_query_data_sources(connection), 999);
    printf("New trial created!\n");

    /*Create metadata*/
    //num_metadata = count_num_metadata();
    printf("Creating primary metadata.\n");
    TAUDB_PRIMARY_METADATA* pm = taudb_create_primary_metadata(2);
    pm[0].name = taudb_strdup("Application");
    pm[0].value = taudb_strdup(appName);
    taudb_add_primary_metadata_to_trial(trial, &(pm[0]));

    //get the start time of the task
    now = time(0);
    current = localtime(&now);
    snprintf(startTime, 64, "%d%d%d", (int)current->tm_hour, (int)current->tm_min, (int)current->tm_sec);
    //pm = taudb_create_primary_metadata(5);
    pm[1].name = taudb_strdup("StartTime");
    pm[1].value = taudb_strdup(startTime);
    taudb_add_primary_metadata_to_trial(trial, &(pm[1]));
    printf("Primary metadata created!\n");

    boolean update = 0;
    boolean cascade = 1;
    //printf("Datasource_init id is %d\n", trial->data_source->id);
    taudb_save_trial(connection, trial, update, cascade);
    printf("Trial saved, initialization complete!\n");

    return trial;
}


TAUDB_THREAD* harmony_taudb_create_thread(int threadNum) {
    //int ctr = 0;
    thread = taudb_create_threads(threadNum);
    for (int i = 0; i < threadNum; i++) {
    	thread[i].node_rank = i;
    	thread[i].thread_rank = 1;
    	thread[i].context_rank = 1;
    	thread[i].index = 1;
    	taudb_add_thread_to_trial(trial, &thread[i]); 
	}
    return thread;
}

/*Get per client metadata*/
void harmony_taudb_get_secondary_metadata(TAUDB_THREAD* thread, char* opsys, char* machine, char* release, char* hostname, char* procnum, char* cpuvendor, char* cpumodel, char* clkfreq, char* cachesize) {
    //TAUDB_SECONDARY_METADATA* cur = taudb_create_secondary_metadata(1);
    TAUDB_SECONDARY_METADATA* sm = taudb_create_secondary_metadata(1);

    /*Loading os info*/
    printf("Loading OS information.\n");
    sm->key.thread = thread;
    sm->key.name = taudb_strdup("OS");
    sm->value = (char**)malloc(sizeof(char*));
    sm->value[0] = taudb_strdup(opsys);
    taudb_add_secondary_metadata_to_trial(trial, sm);
    printf("OS name loaded.\n");

    sm = taudb_create_secondary_metadata(1);
    sm->key.thread = thread;
    sm->key.name = taudb_strdup("Machine");
    sm->value = (char**)malloc(sizeof(char*));
    sm->value[0] = taudb_strdup(machine);
    taudb_add_secondary_metadata_to_trial(trial, sm);
    printf("Machine name loaded.\n");

    sm = taudb_create_secondary_metadata(1);
    sm->key.thread = thread;
    sm->key.name = taudb_strdup("Release");
    sm->value = (char**)malloc(sizeof(char*));
    sm->value[0] = taudb_strdup(release);
    taudb_add_secondary_metadata_to_trial(trial, sm);
    printf("Release name loaded.\n");

    sm = taudb_create_secondary_metadata(1);
    sm->key.thread = thread;
    sm->key.name = taudb_strdup("HostName");
    sm->value = (char**)malloc(sizeof(char*));
    sm->value[0] = taudb_strdup(hostname);
    taudb_add_secondary_metadata_to_trial(trial, sm);
    printf("Host name loaded.\n");

    /*Loading CPU info*/
    printf("Loading CPU information.\n");
    sm = taudb_create_secondary_metadata(1);
    sm->key.thread = thread;
    sm->key.name = taudb_strdup("ProcNum");
    sm->value = (char**)malloc(sizeof(char*));
    sm->value[0] = taudb_strdup(procnum);
    taudb_add_secondary_metadata_to_trial(trial, sm);
    printf("Processor num loaded.\n");

    sm = taudb_create_secondary_metadata(1);
    sm->key.thread = thread;
    sm->key.name = taudb_strdup("CPUVendor");
    sm->value = (char**)malloc(sizeof(char*));
    sm->value[0] = taudb_strdup(cpuvendor);   
    taudb_add_secondary_metadata_to_trial(trial, sm);
    printf("CPU vendor loaded.\n");

    sm = taudb_create_secondary_metadata(1);
    sm->key.thread = thread;
    sm->key.name = taudb_strdup("CPUModel");
    sm->value = (char**)malloc(sizeof(char*));
    sm->value[0] = taudb_strdup(cpumodel);
    taudb_add_secondary_metadata_to_trial(trial, sm);
    printf("CPU model loaded.\n");

    sm = taudb_create_secondary_metadata(1);
    sm->key.thread = thread;
    sm->key.name = taudb_strdup("ClockFreq");
    sm->value = (char**)malloc(sizeof(char*));
    sm->value[0] = taudb_strdup(clkfreq);
    taudb_add_secondary_metadata_to_trial(trial, sm);
    printf("Clock frequency loaded.\n");

    sm = taudb_create_secondary_metadata(1);
    sm->key.thread = thread;
    sm->key.name = taudb_strdup("CacheSize");
    sm->value = (char**)malloc(sizeof(char*));
    sm->value[0] = taudb_strdup(cachesize);
    taudb_add_secondary_metadata_to_trial(trial, sm);
    printf("Cache size loaded.\n");

    //taudb_add_secondary_metadata_to_secondary_metadata(root, &cur);
    printf("Saving secondary metadata...\n");
    boolean update = 1;
    boolean cascade = 1;
    //taudb_add_secondary_metadata_to_trial(trial, &(sm[0]));
    //taudb_save_secondary_metadata(connection, trial, update);
    taudb_save_trial(connection, trial, update, cascade);
    printf("Secondary metadata saving complete.\n");
}

/*Finalize a trial*/
void taudb_fini() {
    printf("Saving trial to taudb\n");
    boolean update = 1;
    boolean cascade = 1;
	taudb_compute_statistics(trial);
    taudb_save_trial(connection, trial, update, cascade);
    printf("Disconnecting from TAUdb.\n");
    /*Disconnect from taudb*/
    taudb_disconnect(connection);
}
