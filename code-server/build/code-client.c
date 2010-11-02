/***
 * include other user written headers
 ***/
#include <fcntl.h>
#include "code-client.h"
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
int code_client_socket=-1;

// id that identifies an application
int code_client_id = -1;


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
    return code_client_socket;
}


// the purpose of these double definition is the use of different
// calls for the same function. I will clean this in the next version.
int pget_local_sock() {
    return code_client_socket;
}

/***
 *
 * Exit and send a mesage to stderr
 *
 ***/
void cc_exit(char *errmesg){
    perror(errmesg);
    codeserver_end();
    exit(1);
}


char codeserver_host[256];
int codeserver_port;

void codeserver_startup(char* hostname, int sport) {

  //int sport = 0;
    char *shost = NULL;
    struct sockaddr_in address;
    struct hostent *hostaddr = NULL;
    //char *hostname;
    HRegistMessage *mesg;

    char temps[BUFFER_SIZE];

    int fileflags;

    /* create socket */
    if ((code_client_socket=socket(AF_INET,SOCK_STREAM,0))<0) {
        cc_exit("Could not create socket!");
    }

    /* connect to server */
    address.sin_family = AF_INET;

    // look for the CODESERVER_S_PORT environment variable
    // to set the port of connection to the machine where the codeserver
    // server is running

    /*
    if (sport == 0) {
        if (getenv("CODESERVER_S_PORT")==NULL)
        {
            cc_exit("CODESERVER_S_PORT variable not set!");
        }
        sport = atoi(getenv("CODESERVER_S_PORT"));
    }
    */
    address.sin_port=htons(sport);

    /*
    //look for the CODESERVER_S_HOST environment variable
    // to set the host where the codeserver server resides
    if (shost == NULL){
        if (getenv("CODESERVER_S_HOST")==NULL)
            cc_exit("CODESERVER_S_HOST variable not set!");
        shost=getenv("CODESERVER_S_HOST");
    }
    hostname = shost;
    */
    if ((hostaddr=gethostbyname(hostname)) == NULL)
        cc_exit("Host not found!");

    /* save server hostname and port if necessary to migrate */
    strcpy(codeserver_host, hostname);
    codeserver_port = sport;

    /* set up the host address we will connect to */
#ifdef Linux
    inet_aton(inet_ntop(hostaddr->h_addrtype,hostaddr->h_addr,temps,sizeof(temps)), &address.sin_addr);
#else
    bcopy((char *)hostaddr->h_addr, (char *)&address.sin_addr, hostaddr->h_length);
#endif

    int optVal = 1;
    int ret = setsockopt(code_client_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &optVal, sizeof(int));
    if (ret) {
        perror("setsockopt");
        cc_exit("<setsockopt>fd_create");
    }

    // try to connect to the server
    if (connect(code_client_socket,(struct sockaddr *)&address, sizeof(address))<0)
        cc_exit("Connection failed!");

    fprintf(stderr, "CONNECTED\n");

    /* send the registration message */
    mesg = new HRegistMessage(CMESG_CLIENT_REG, 1);

    mesg->print();

    //printf("client socket %d \n", code_client_socket);

    send_message(mesg,code_client_socket);

    delete mesg;

    /* wait for confirmation */

    mesg = (HRegistMessage *)receive_message(code_client_socket);
    code_client_id = mesg->get_id();
    fprintf(stderr, "code_client_id: %d\n", code_client_id);
    //assert(code_client_id > 0);
    delete mesg;

    int connected = 1;

    //fprintf(stderr, "handler installed\n");
}

void codeserver_startup_no_params() {

    int sport = 0;
    char *shost = NULL;
    struct sockaddr_in address;
    struct hostent *hostaddr = NULL;
    char *hostname;
    HRegistMessage *mesg;

    char temps[BUFFER_SIZE];

    int fileflags;

    /* create socket */
    if ((code_client_socket=socket(AF_INET,SOCK_STREAM,0))<0) {
        cc_exit("Could not create socket!");
    }

    /* connect to server */
    address.sin_family = AF_INET;

    // look for the CODESERVER_S_PORT environment variable
    // to set the port of connection to the machine where the codeserver
    // server is running

    if (sport == 0) {
        if (getenv("CODESERVER_S_PORT")==NULL)
        {
            cc_exit("CODESERVER_S_PORT variable not set!");
        }
        sport = atoi(getenv("CODESERVER_S_PORT"));
    }
    address.sin_port=htons(sport);

    //look for the CODESERVER_S_HOST environment variable
    // to set the host where the codeserver server resides
    if (shost == NULL){
        if (getenv("CODESERVER_S_HOST")==NULL)
            cc_exit("CODESERVER_S_HOST variable not set!");
        shost=getenv("CODESERVER_S_HOST");
    }
    hostname = shost;

    if ((hostaddr=gethostbyname(hostname)) == NULL)
        cc_exit("Host not found!");

    /* save server hostname and port if necessary to migrate */
    strcpy(codeserver_host, hostname);
    codeserver_port = sport;

    /* set up the host address we will connect to */
#ifdef Linux
    inet_aton(inet_ntop(hostaddr->h_addrtype,hostaddr->h_addr,temps,sizeof(temps)), &address.sin_addr);
#else
    bcopy((char *)hostaddr->h_addr, (char *)&address.sin_addr, hostaddr->h_length);
#endif

    int optVal = 1;
    int ret = setsockopt(code_client_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &optVal, sizeof(int));
    if (ret) {
        perror("setsockopt");
        cc_exit("<setsockopt>fd_create");
    }

    // try to connect to the server
    if (connect(code_client_socket,(struct sockaddr *)&address, sizeof(address))<0)
        cc_exit("Connection failed!");

    fprintf(stderr, "CONNECTED\n");

    /* send the registration message */
    mesg = new HRegistMessage(CMESG_CLIENT_REG, 1);

    mesg->print();

    //printf("client socket %d \n", code_client_socket);

    send_message(mesg,code_client_socket);

    delete mesg;

    /* wait for confirmation */

    mesg = (HRegistMessage *)receive_message(code_client_socket);
    code_client_id = mesg->get_id();
    fprintf(stderr, "code_client_id: %d\n", code_client_id);
    //assert(code_client_id > 0);
    delete mesg;

    int connected = 1;

    //fprintf(stderr, "handler installed\n");
}


/*
 * When a program announces the server that it will end it calls this function
 */
void codeserver_end(){


    HRegistMessage *m = new HRegistMessage(CMESG_CLIENT_UNREG,0);

    // send the unregister message to the server
    send_message(m,code_client_socket);

    delete m;

    /* wait for server confirmation */

    m =(HRegistMessage *) receive_message(code_client_socket);

    //printf("Message received!?!\n");
    //m->print();

    delete m;

    printf("Close socket!\n");

    // close the actual connection
    close(code_client_socket);

}

/*
 * codeserver: generate code
 */

void generate_code(char *request) {
    HDescrMessage *mesg_desr=new HDescrMessage(CMESG_CODE_GENERATION_REQ,request,strlen(request));
    send_message(mesg_desr, code_client_socket);
    delete mesg_desr;

    /* wait for confirmation */
    HMessage *mesg;
    mesg = receive_message(code_client_socket);

    if (mesg->get_type() != CMESG_CONFIRM) {
        // something went wrong on the server side !
        fprintf(stderr," code generation failed : %d\n",mesg->get_type());
        delete mesg;
        cc_exit("Codeserver failed ... \n");
    } else {
    }

    //HDescrMessage *m_casted = (HDescrMessage *)mesg;
    delete mesg;
    
    /*
    char* projected = m_casted->get_descr();

    string temp_projected(projected);
    return temp_projected;
    */
}

int code_generation_complete(){

  HRegistMessage *mesg_send=new HRegistMessage(CMESG_CODE_COMPLETE_QUERY,1);

  send_message(mesg_send, code_client_socket);
  delete mesg_send;

  /* wait for confirmation */
  HRegistMessage *mesg;
  mesg = (HRegistMessage *) receive_message(code_client_socket);

  if (mesg->get_type() != CMESG_CODE_COMPLETE_RESPONSE) {
        // something went wrong on the server side !
        fprintf(stderr," code generation complete request failed : %d\n",mesg->get_type());
        delete mesg;
        cc_exit("Codeserver failed ... \n");
    } else {
    }

  int return_val=mesg->get_param();
  delete mesg;
  

  return return_val;

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
  codeserver_startup("spoon", 2002);
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

    
    char point_[2048];
    //sprintf(point_,"3 12 :  9 2 :  4 10 :  14 8 :  2 11 :  14 14 :  2 6 :  15 10 :  12 13 :  13 14 :  6 13 :  12 9 :  6 2 :  15 15 :  1 2 :  5 4 :  5 6 :  12 15 :  8 11 :  7 9 :  6 13 :  14 14 :  4 14 :  9 15 :  4 14 :  6 1 :  3 2 :  1 9 :  10 8 :  8 3 :  9 5 :  13 5 :  2 10 :  12 9 :  12 11 :  3 9 :  8 9 :  8 11 :  7 9 :  3 2 :  7 8 :  3 10 :  2 3 :  3 3 :  11 3 :  6 11 :  15 4 :  8 2 :  5 4 :  10 8 :  15 4 :  2 7 :  13 2 :  10 11 :  10 12 :  13 2 :  12 15 :  11 13 :  10 6 :  8 5 :  1 5 :  8 15 :  15 15 :  1 5 :  4 3 :  13 10 :  7 7 :  9 4 :  8 4 :  7 3 :  15 4 :  11 4 :  11 14 :  8 6 :  11 15 :  2 11:  5 9 :  11 5 :  9 3 :  1 4 :  6 6 :  6 12 :  12 15 :  7 11 :  10 14 :  5 2 :  9 7 :  5 5 :  5 13 :  10 1 :  13 12 :  4 9 :  6 14 :  6 6 :  2 6 :  2 7 :  3 8 :  11 15 :  15 10 :  2 9 :  15 14 :  11 9 :  6 8 :  13 11 :  5 15 :  3 10 :  3 7 :  10 1 :  12 15 :  6 14 :  13 15 :  13 1 :  8 15 :  7 14 :  9 9 :  8 15 :  14 10 :  8 4 :  10 13 :  7 14 :  12 1 :  15 15 :  7 10 :  15 11 :  2 13 :  10 15 :  13 14 :  8 13  | 5 15 :  11 6 :  15 11 :  5 14 :  5 13 :  10 14 :  3 1 :  5 15 :  1 12 :  7 15 :  13 13 :  11 15 :  3 13 :  6 1 :  3 14 :  13 8 :  5 15 :  13 11 :  11 9 :  1 8 :  14 10 :  14 2 :  2 3 :  8 3 :  7 14 :  10 5 :  4 6 :  4 6 :  3 2 :  6 5 :  15 11 :  4 11 :  3 8 :  6 13 :  2 7 :  12 8 :  9 10 :  9 10 :  4 1 :  13 11 :  6 7 :  15 2 :  5 3 :  7 7 :  4 5 :  3 11 :  7 14 :  6 9 :  14 4 :  13 7 :  310 :  14 11 :  4 14 :  6 8 :  6 11 :  3 12 :  10 2 :  13 14 :  12 12 :  12 7 :  2 15 :  3 1 :  6 1 :  10 4 ");


    //sprintf(point_, " 1 1 1 1 1 :  1 1 1 1 3 :  1 1 1 3 1 :  1 1 1 3 3 :  1 1 6 1 1 :  1 1 6 1 3 :  1 1 6 3 1 :  1 1 6 3 3 :  1 1 11 1 1 :  1 1 11 1 3 :  1 1 11 3 1 :  1 1 11 3 3 :  1 6 1 1 1 :  1 6 1 1 3 :  1 6 1 3 1:  1 6 1 3 3 :  1 6 6 1 1 :  1 6 6 1 3 :  1 6 6 3 1 :  1 6 6 3 3 :  1 6 11 1 1 :  1 6 11 1 3 :  1 6 11 3 1 :  1 6 11 3 3 :  1 11 1 1 1 :  1 11 1 1 3 :  1 11 1 3 1 :  1 11 1 3 3 :  1 11 6 1 1 :  1 11 6 1 3 :  1 11 6 3 1 :  1 11 6 3 3 :  1 11 11 1 1 :  1 11 11 1 3 :  1 11 11 3 1 :  1 11 11 3 3 :  6 1 1 1 1 :  6 1 1 1 3 :  6 1 1 3 1 :  6 1 1 3 3 :  6 1 6 1 1 :  6 1 6 1 3 :  6 1 6 3 1 :  6 1 6 3 3 :  6 1 11 1 1 :  6 1 11 1 3 :  6 1 11 3 1 :  6 1 11 3 3 :  6 6 1 1 1 :  6 6 1 1 3 :  6 6 1 3 1 :  6 6 1 3 3 :  6 6 6 1 1 :  6 6 6 1 3 :  6 6 6 3 1 :  6 6 6 3 3 :  6 6 11 1 1 :  6 6 11 1 3 :  6 6 11 3 1 :  6 6 11 3 3 :  6 11 1 1 1 :  6 11 1 1 3 :  6 11 1 3 1 :  6 11 1 3 3 :  6 11 6 1 1 :  6 11 6 1 3 :  6 11 6 3 1 :  6 11 6 3 3 :  6 11 11 1 1 :  6 11 11 1 3 :  6 11 11 3 1 :  6 11 11 3 3 :11 1 1 1 1 :  11 1 1 1 3 :  11 1 1 3 1 :  11 1 1 3 3 :  11 1 6 1 1 :  11 1 6 1 3 :  11 1 6 3 1 :  11 1 6 3 3 :  11 1 11 1 1 :  11 1 11 1 3 :  11 1 11 3 1 :  11 1 11 3 3 :  11 6 1 1 1 :  11 6 1 1 3:  11 6 1 3 1 :  11 6 1 3 3 :  11 6 6 1 1 :  11 6 6 1 3 :  11 6 6 3 1 :  11 6 6 3 3 :  11 6 11 1 1 :  11 6 11 1 3 :  11 6 11 3 1 :  11 6 11 3 3 :  11 11 1 1 1 :  11 11 1 1 3 :  11 11 1 3 1 :  11 11 1 3 3 :  11 11 6 1 1 :  11 11 6 1 3 :  11 11 6 3 1 :  11 11 6 3 3 :  11 11 11 1 1 :  11 11 11 1 3 :  11 11 11 3 1 :  11 11 11 3 3 | 16 11 11 1 1 :  11 11 16 1 3 :  11 11 11 3 1 :  11 11 11 3 3");
    sprintf(point_, " 1 1 1 1 1 :  1 1 1 1 3 |  1 1 6 1 1 :  1 1 6 1 3");
    
    generate_code(point_);


    int code_flag=code_generation_complete();
    while(code_flag!=1){
      sleep(4);
      code_flag=code_generation_complete();
    }
    printf("Code generation is complete? : %d \n", code_generation_complete());

    sprintf(point_,"  1 1 6 1 1 :  11 11 11 3 3  | 12 12 12 3 3 : 1 2 3 4 5 : 1 4 3 5 1 : 1 4 2 5 1: 1 2 3 2 1" );

    generate_code(point_);
    code_flag=code_generation_complete();
    while(code_flag!=1){
      sleep(1);
      code_flag=code_generation_complete();
    }
    printf("Code generation is complete? : %d \n", code_generation_complete());

    sprintf(point_,"  1 2 3 4 5 :  1 1 6 1 1 :  1 2 3 2 1  | 12 12 12 3 3" );

    generate_code(point_);
    sleep(4);
    sleep(10);

    // ask if the code is complete

    //printf("Code generation is complete? : %d \n", code_generation_complete());

    //char point_2[80];
    //sprintf(point_2, "2 4 : 3 4 : 4 5 : 1 1 : 12 13 : 1 4 | 1 15");
    //generate_code(point_2);
    sleep(6);

    //printf("%s \n", do_codeserver(point_));
    //codeserver_end();
    /*
    sprintf(point_,"100 13 20 1 19");
    do_codeserver(point_);
    sprintf(point_,"100 14 20 1 19");
    do_codeserver(point_);
    sprintf(point_,"100 15 20 1 19");
    do_codeserver(point_);
    */
    sleep(3);
    printf("Code generation is complete? : %d \n", code_generation_complete());


    codeserver_end();
    
    //while (true);
}
#endif
