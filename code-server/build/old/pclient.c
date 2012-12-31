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

/***
 * include other user written headers
 ***/
#include <fcntl.h>
#include "pclient.h"
#include <netinet/in.h>
#include <strings.h>
#include <sys/time.h>
#include <signal.h>

using namespace std;
/*
 * global variables
 */

// the purpose of this static variable is to define the debug mode
// when this value is set to 1, different values might be printed for
// debuginng purposes. However, the information that is outputed might
// not make sense (except for the author! :)
static int debug_mode=0;

int connected = 0;

// the client socket that will support the TCP connection to the server
int pclient_socket=-1;

// id that identifies an application (used for migration)
int pclient_id = -1;

// HACK ALERT:
char* global_projected;

// the timestamp used by the simplex minimization method to decide
// whether the performance reflects the changes of the parameters
unsigned long int hclient_timestamp = 0;


/***
 * prototypes of side functions
 ***/
void process_update(HUpdateMessage *m);

/**
 *
 * returns the local socket used for connection to server
 *
 **/
int get_local_sock() {
    return pclient_socket;
}


// the purpose of these double definition is the use of different
// calls for the same function. I will clean this in the next version.
int pget_local_sock() {
    return pclient_socket;
}

/***
 *
 * Exit and send a mesage to stderr
 *
 ***/
void pc_exit(char *errmesg){
    perror(errmesg);
    projection_end();
    exit(1);
}


/*
 * this variables are used for migration
 * we save the server and port of the server in case we have to migrate
 *
 */

char projection_host[256];
int projection_port;

void projection_startup() {

    //int return_val = 1;
    int sport = 0;
    char *shost = NULL;
    struct sockaddr_in address;
    struct hostent *hostaddr = NULL;
    char *hostname;
    HRegistMessage *mesg;

    char temps[BUFFER_SIZE];

    int fileflags;

    //fprintf(stderr, "hs:%d\n", __LINE__);

    /* set the flag that tells if we use signals */
    //huse_signals = use_sigs;

    /* create socket */
    if ((pclient_socket=socket(AF_INET,SOCK_STREAM,0))<0) {
        pc_exit("Could not create socket!");
    }

    /* connect to server */
    address.sin_family = AF_INET;

    // look for the PROJECTION_S_PORT environment variable
    // to set the port of connection to the machine where the projection
    // server is running

    if (sport == 0) {
        if (getenv("PROJECTION_S_PORT")==NULL)
        {
            pc_exit("PROJECTION_S_PORT variable not set!");
        }
        sport = atoi(getenv("PROJECTION_S_PORT"));
    }
    address.sin_port=htons(sport);

    //look for the PROJECTION_S_HOST environment variable
    // to set the host where the projection server resides
    if (shost == NULL){
        if (getenv("PROJECTION_S_HOST")==NULL)
            pc_exit("PROJECTION_S_HOST variable not set!");
        shost=getenv("PROJECTION_S_HOST");
    }
    hostname = shost;

    if ((hostaddr=gethostbyname(hostname)) == NULL)
        pc_exit("Host not found!");

    /* save server hostname and port if necessary to migrate */
    strcpy(projection_host, hostname);
    projection_port = sport;

    /* set up the host address we will connect to */
#ifdef Linux
    inet_aton(inet_ntop(hostaddr->h_addrtype,hostaddr->h_addr,temps,sizeof(temps)), &address.sin_addr);
#else
    bcopy((char *)hostaddr->h_addr, (char *)&address.sin_addr, hostaddr->h_length);
#endif

    int optVal = 1;
    int ret = setsockopt(pclient_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &optVal, sizeof(int));
    if (ret) {
        perror("setsockopt");
        pc_exit("<setsockopt>fd_create");
    }

    // try to connect to the server
    if (connect(pclient_socket,(struct sockaddr *)&address, sizeof(address))<0)
        pc_exit("Connection failed!");

    fprintf(stderr, "CONNECTED\n");

    /* send the registration message */
    mesg = new HRegistMessage(HMESG_CLIENT_REG, 1);

    mesg->print();

    //printf("client socket %d \n", pclient_socket);

    send_message(mesg,pclient_socket);

    delete mesg;

    /* wait for confirmation */

    mesg = (HRegistMessage *)receive_message(pclient_socket);
    pclient_id = mesg->get_id();
    fprintf(stderr, "pclient_id: %d\n", pclient_id);
    //assert(pclient_id > 0);
    delete mesg;

    int connected = 1;

    //fprintf(stderr, "handler installed\n");
}


/*
 * When a program announces the server that it will end it calls this function
 */
void projection_end(){

    //fprintf(stderr, "he:%d\n", __LINE__);

    HRegistMessage *m = new HRegistMessage(HMESG_CLIENT_UNREG,0);

    // send the unregister message to the server
    send_message(m,pclient_socket);


    //printf("Exit message sent!\n");

    delete m;

    /* wait for server confirmation */

    m =(HRegistMessage *) receive_message(pclient_socket);

    //printf("Message received!?!\n");
    //m->print();

    delete m;

    printf("Close socket!\n");

    // close the actual connection
    close(pclient_socket);

}

/*
 * Is given point admissible?
 */
int is_a_valid_point(char* point) {
    int validity = 0;

    HDescrMessage *mesg_desr=new HDescrMessage(HMESG_PROJECTION,point,strlen(point));
    send_message(mesg_desr, pclient_socket);
    delete mesg_desr;

    /* wait for confirmation */
    HMessage *mesg;
    mesg = receive_message(pclient_socket);

    if (mesg->get_type() != HMESG_CONFIRM) {
        // something went wrong on the server side !
        fprintf(stderr," Unexpected message type: %d\n",mesg->get_type());
        delete mesg;
        pc_exit("Application description is not correct!\n");
    } else {
    }
    HRegistMessage *m_casted=(HRegistMessage *)mesg;
    validity = m_casted->get_param();

    //printf("parameter : %d \n", validity);
    delete m_casted;

    return validity;

}

/*
 * simplex construction:: returns the simplex
 */
char* simplex_construction(char* request) {

    HDescrMessage *mesg_desr=new HDescrMessage(HMESG_SIM_CONS_REQ,request,strlen(request));
    send_message(mesg_desr, pclient_socket);
    delete mesg_desr;

    /* wait for confirmation */
    HMessage *mesg;
    mesg = receive_message(pclient_socket);

    if (mesg->get_type() != HMESG_SIM_CONS_RESULT) {
        // something went wrong on the server side !
        fprintf(stderr," projection_sim_construction : %d\n",mesg->get_type());
        delete mesg;
        pc_exit("Simplex Construction failed ... \n");
    }

    HDescrMessage *m_casted=(HDescrMessage *)mesg;
    char* simplex = m_casted->get_descr();

    //printf("simplex : %s \n", simplex);
    delete m_casted;

    return simplex;
}

// temp_
char* projection_sim_construction_2(char* request, int mesg_type) {

    HDescrMessage *mesg_desr=new HDescrMessage(mesg_type,request,strlen(request));
    send_message(mesg_desr, pclient_socket);
    delete mesg_desr;

    /* wait for confirmation */
    HMessage *mesg;
    mesg = receive_message(pclient_socket);

    if ((mesg->get_type() != HMESG_SIM_CONS_RESULT) && (mesg->get_type() != HMESG_PROJ_RESULT)) {
        // something went wrong on the server side !
        fprintf(stderr," projection_sim_construction : %d\n",mesg->get_type());
        delete mesg;
        pc_exit("Simplex Construction failed ... \n");
    }

    HDescrMessage *m_casted=(HDescrMessage *)mesg;
    char* simplex = m_casted->get_descr();

    //printf("simplex : %s \n", simplex);
    delete m_casted;

    return simplex;
}

/*
 * projection: project a point to the admissible region
 */

string do_projection(char *request) {
    HDescrMessage *mesg_desr=new HDescrMessage(HMESG_PROJ_REQ,request,strlen(request));
    send_message(mesg_desr, pclient_socket);
    delete mesg_desr;

    /* wait for confirmation */
    HMessage *mesg;
    mesg = receive_message(pclient_socket);

    if (mesg->get_type() != HMESG_PROJ_RESULT) {
        // something went wrong on the server side !
        fprintf(stderr," do_projection : %d\n",mesg->get_type());
        delete mesg;
        pc_exit("Projection failed ... \n");
    } else {
    }

    HDescrMessage *m_casted = (HDescrMessage *)mesg;
    char* projected = m_casted->get_descr();

    //printf("projection of [%s] to %s \n", request, projected);
    //delete m_casted;

    string temp_projected(projected);
    //global_projected = projected;
    //printf("projection of [%s] to %s \n", request, global_projected);
    //cout << "temp_projected::: " << temp_projected;
    return temp_projected;
}

char* string_to_char_star(string str) 
{
    //cout << "received:: " << str;
    char *char_star;
    char_star = new char[str.length() + 1];
    strcpy(char_star, str.c_str());
    return char_star;
}


#ifdef TEST
int main() {
    projection_startup();
    char point[24];
    sprintf(point,"100 10 20 1 19");

    //printf("point_valid? --> %d \n ", is_a_valid_point(point));


    //sprintf(point,"100 10 20 1 19");

    //    printf("point_valid? --> %d \n ", is_a_valid_point(point));

    //sprintf(point,"100 10 20 1 19");

    //printf("point_valid? --> %d \n ", is_a_valid_point(point));

    //sprintf(point,"100 10 20 1 19");

    //printf("point_valid? --> %d \n ", is_a_valid_point(point));

    //char point_init[80];
    //sprintf(point_init,"5:4 100 20 20 4 4");
    //simplex_construction(point_init);

    //char point_init[24];
    //sprintf(point_init,"1:1 100 10 20 5 3");
    //printf(" initial simplex construction:: %s \n", simplex_construction(point_init));

    
    char point_[80];
    sprintf(point_,"10 1 : 1 4 : 1 2 : 1 4 | 2 4 : 4 5 : 5 6 : 7 8 : 9 10 : 11 12 : 12 13");
    do_projection(point_);

    sleep(4);

    char point_2[80];
    sprintf(point_2, "2 4 : 3 4 : 4 5 : 1 1 : 12 13 : 1 4 | 1 15");
    do_projection(point_2);
    sleep(6);

    //printf("%s \n", do_projection(point_));
    //projection_end();
    /*
    sprintf(point_,"100 13 20 1 19");
    do_projection(point_);
    sprintf(point_,"100 14 20 1 19");
    do_projection(point_);
    sprintf(point_,"100 15 20 1 19");
    do_projection(point_);
    */
    projection_end();
    
    //while (true);
}
#endif
