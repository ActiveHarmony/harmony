#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "hclient.h"
#include "hsockutil.h"

#define PERF_MAX INT_MAX

//helpers
int getoneline(FILE *in, char *buf);
int getline(FILE *in, char *buf);

// app variables
int *param1=NULL;

// change this to reflect the application name.
// For example, for smg2000, this will be smg2000
char *appname="APPTest";


// driver starts here.
int main(int argc, char** argv) {

    FILE *timing_result_file,*done_file_handle;
    int iter, num_samples;
    int perf;
    char s_buf[256], tcl_file[128];
    char timings_filename[20],done_filename[20];
    int i;

    /* read in the unique ID for this run - this ID must
     * provided at the command line during qsub
     * the process gets the id from the
     * command line.
     */
    if(argc != 2)
    {
        printf("Error: You need to provide an id for this process.\n");
        printf("usage: ./schedular <id> \n");
        exit(EXIT_FAILURE);
    }
    char *id = argv[1];
    if(id == NULL)
    {
        printf("Error retrieving APPID");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("ID for this run: %s \n ", id);
    }

    /* Connect to the Harmony server */
    printf("Starting the server ... \n");
    harmony_startup();

    // create some appid specific strings
    // this is the timings file that you want to monitor
    sprintf(timings_filename, "timings.%s", id);
    // indication that job in the cluster is done
    sprintf(done_filename, "done.iter.%s", id);

    /* create the application setup file */
    sprintf(s_buf, "source create_tcl_file.sh %s > %s_tuning.tcl.%s", id, appname, id);
    printf(s_buf);
    printf("\n");
    system(s_buf);

    /* input file for this run */
    sprintf(tcl_file, "%s_tuning.tcl.%s", appname, id);
    printf("APP Input file: %s \n", tcl_file);
    harmony_application_setup_file(tcl_file);

    /* register variables */
    // make sure the appname matches the name in the .tcl file
    // also the name of the param should match with the
    param1 = (int *)harmony_add_variable(appname,"param1",VAR_INT);

    /* first dummy iteration to initialize the PRO simplex */
    printf("Sending the default performance %d \n", PERF_MAX);
    harmony_performance_update(PERF_MAX);

    int db_value=PERF_MAX;
    int return_val=0;
    num_samples=3;
    /* begin the optimization iterations */
    // how many iterations?
    iter=20;

    for(i=0;i<iter;i++) {

        // get the new set of parameters
        harmony_request_all();
        printf("Got the new parameter from the server: %d \n", *param1);

        // first check with the server to see if we have already seen this
        // configuration before. If we have there is no need to schedule a
        // job on the cluster for this configuration.
        db_value=atoi((char*)harmony_database_lookup());
        printf("got the db_value: %d \n", db_value);

        if(db_value==PERF_MAX)
        {
            // remove the old timings file, old done indicators
            sprintf(s_buf, "\\rm -f timings.%s done.iter.%s ", id, id);
            system(s_buf);

            // just in case we have some previous results file lingering
            // around, clean up first
            sprintf(s_buf, "\\rm -f APP.tmp.%s APP.result.%s APP.error.%s", id ,id, id);
            system(s_buf);

            //create the submission script
            sprintf(s_buf, "source create_submission_script.sh %d %s %d > submit_script.%s.sh", *param1, id, num_samples, id);
            printf("create submit script command: %s \n", s_buf);
            return_val=system(s_buf);

            // qsub the application
            // create the qsub string
            sprintf(s_buf,"qsub submit_script.%s.sh", id);
            printf("%s \n", s_buf);
            return_val=system(s_buf);
            printf("job submission done for id: %s \n", id);
            printf("Waiting for the job completion \n");

            // monitor the done.iter.id file
            done_file_handle = fopen(done_filename, "r");
            // waiting for the qsub to be done
            // this is a simple way to wait for the job to be done
            while(done_file_handle == NULL ) {
                sleep(5);
                done_file_handle = fopen(done_filename, "r");
            }
            fclose(done_file_handle);

            timing_result_file=fopen(timings_filename, "r");
            /* get the result */
            // I have not done any error checking here. If the timings_file is
            // empty and/or does not contain an integer value (could be due to
            // application failure), no checks have been made here.
            getline(timing_result_file, s_buf);
            printf("Performance for this configuration: %s \n", s_buf);
            perf=atoi(s_buf);
            // close the results file
            fclose(timing_result_file);
        } else
        {
            // we have already seen this configuration before
            printf("We have seen <%d> configuration before. The perf is %d.\n", *param1, db_value);
            perf=db_value;
        }
        // send the performance to the server
        harmony_performance_update(perf);

        // let everyone else catch up
        harmony_psuedo_barrier();
    }

    // if there are other cleanups required, do them here.

    harmony_end();

    return 0;
}
// helper functions
int getoneline(FILE *in, char *buf) {
    int i;
    char c;
    if((c=fgetc(in))==EOF)
        return EOF;
    else
        ungetc(c,in);
    for(i=0;;i++) {
        buf[i]=fgetc(in);
        if(buf[i]=='\n')
            break;
        else if(buf[i]==EOF) {
            ungetc(EOF,in);
            break;
        }
    }
    buf[i]='\0';
    return 0;
}

int getline(FILE *in, char *buf) {
    int r;
    while(1) {
        r=getoneline(in,buf);
        if(r==EOF)
            return r;
        if(buf[0]=='\0')        /* empty line */
            continue;
        else if(buf[0]=='#')    /* comment line */
            continue;
        return r;
    }
}
