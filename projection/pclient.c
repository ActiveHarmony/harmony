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

/***
 * include other user written headers
 ***/
#include "pclient.h"

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
    //exit(1);
}


/*
 * this variables are used for migration
 * we save the server and port of the server in case we have to migrate
 *
 */

char projection_host[256];
int projection_port;

int projection_startup(char* hostname, int sport) {

    int return_val = 1;
    //int sport = 0;
    //char *shost = NULL;
    struct sockaddr_in address;
    struct hostent *hostaddr = NULL;
    //char *hostname;
    HRegistMessage *mesg;

    char temps[BUFFER_SIZE];

    int fileflags;

    //fprintf(stderr, "hs:%d\n", __LINE__);

    /* set the flag that tells if we use signals */
    //huse_signals = use_sigs;

    /* create socket */
    if ((pclient_socket=socket(AF_INET,SOCK_STREAM,0))<0) {
        pc_exit("Could not create socket!");
        return 0;
    }

    /* connect to server */
    address.sin_family = AF_INET;
    /* assign port */
    address.sin_port=htons(sport);

    if ((hostaddr=gethostbyname(hostname)) == NULL)
    {
        pc_exit("Host not found!");
        return 0;
    }

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
        return 0;
    }

    // try to connect to the server
    if (connect(pclient_socket,(struct sockaddr *)&address, sizeof(address))<0)
    {
        pc_exit("Connection failed!");
        return 0;
    }

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

    return 1;

    //fprintf(stderr, "handler installed\n");
}

// seeks HOST and PORT from environment
void projection_startup_2() {

    //int return_val = 1;
    int sport = 0;
    char *shost = NULL;
    struct sockaddr_in address;
    struct hostent *hostaddr = NULL;
    char *hostname;
    HRegistMessage *mesg;

    char temps[BUFFER_SIZE];

    int fileflags;

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

    HRegistMessage *m = new HRegistMessage(HMESG_CLIENT_UNREG,0);

    // send the unregister message to the server
    send_message(m,pclient_socket);

    delete m;

    /* wait for server confirmation */

    m =(HRegistMessage *) receive_message(pclient_socket);

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

    HDescrMessage *mesg_desr=new HDescrMessage(HMESG_IS_VALID,point,strlen(point));
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

    delete m_casted;

    return validity;

}

/*
 * simplex construction:: returns the simplex
 */
void simplex_construction(char* request, char* filename) {

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

    /* store the simplex to the file */
    
    FILE * pFile;
    pFile = fopen (filename,"w");
    if (pFile!=NULL)
    {
        fputs (simplex,pFile);
        fclose (pFile);
    }
    else {
        pc_exit("Saving simplex to a file failed ...");
    }
    

    printf("simplex : %s \n", simplex);
    delete m_casted;
    //return simplex;
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

char* do_projection_one_point(char *request) {
    HDescrMessage *mesg_desr=new HDescrMessage(HMESG_PROJ_ONE_REQ,request,strlen(request));
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
    return projected;
}


char* do_projection_entire_simplex(char *request) {
    HDescrMessage *mesg_desr=new HDescrMessage(HMESG_PROJ_ENT_REQ,request,strlen(request));
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
    return projected;
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
    projection_startup("brood00",2077);
    char point[24]; 
    sprintf(point,"100 5 2 2 2");

    printf("point_valid? --> %d \n ", is_a_valid_point(point));

    char point_init[24];
    sprintf(point_init,"10:1 250 250 250 4 4");
    simplex_construction(point_init,"temp.tcl");

    char point_[24];
    sprintf(point_,"10 3 4 5 3 : 100 5 19 44 11 : 13 2 120 120 -1 : 1 1 1 1 1");
    printf("%s \n",do_projection_entire_simplex(point_));
    projection_end();
}
#endif
