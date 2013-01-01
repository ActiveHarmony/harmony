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

/*
  Written by Ananta Tiwari.
  University of Maryland at College Park.

  Based on hserver.c from Active Harmony project.
*/


#include "pserver.h"
// ANN include file
#include "ANN_init.h"
#include "StringTokenizer.h"

using namespace std;

static int debug_mode=0;

/*
 * the socket waiting for connections
 */
int listen_socket;

#ifdef LINUX
struct in_addr listen_addr;	/* Address on which the server listens. */
#endif

#define TRUE 1
#define FALSE 0

// some ANN declarations
int maxpts;
double eps = 0;

/*
 * the port waiting for connections
 */
int listen_port;
fd_set listen_set;
list<int> socket_set;
int highest_socket;
list<int>::iterator socketIterator;

list<int> id_set;

/* the X file descriptor */
int xfd;


/*
  the map containing client AppName and Socket
*/
map<string, int> clientInfo;
map<int, char *> clientName;
map<int, int> clientSignals;
map<int, int> clientSocket;
map<int, int> clientId;

/***
 *
 * Check the arguments given to the server
 *
 ***/
void check_parameters(int argc, char **argv) {

}


/***
 * This function is called each time a operation requested by the client fails
 * It will return a FAIL message to the client
 ***/
void operation_failed(int sock) {
  HRegistMessage *m = new HRegistMessage(HMESG_FAIL,0);
  send_message(m,sock);
  delete m;
}

int update_sent = FALSE;
char *application_trigger = "";


/***
 *
 * Start the server by initializing sockets and everything else that is needed
 *
 ***/

void server_startup() {
    /* temporary variable */
    char s[10];

    for(int i = 1; i <= 100000; i++)
        id_set.push_back(i);

    /* used address */
    struct sockaddr_in sin;
    int optVal;
    int err;

    listen_port = SERVER_PORT;

    /* create a listening socket */
    if ((listen_socket = socket (AF_INET, SOCK_STREAM,0)) < 0) {
        h_exit("Error initializing socket!");
    }

    /* set socket options. We try to make the port reusable and have it
       close as fast as possible without waiting in unnecessary wait states
       on close. */
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &optVal, sizeof(optVal))) {
        h_exit("Error setting socket options!");
    }

    /* Initialize the socket address. */
    sin.sin_family = AF_INET;
#ifdef LINUX
    sin.sin_addr = listen_addr;
#else
    unsigned int listen_addr = INADDR_ANY;
    sin.sin_addr = *(struct  in_addr*)&listen_addr;
#endif /*LINUX*/
    sin.sin_port = htons(listen_port);

    /* Bind the socket to the desired port. */
    if (bind(listen_socket, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        h_exit("Error binding socket!");
    }

    /*
      Set the file descriptor set
    */
    FD_ZERO(&listen_set);
    FD_SET(listen_socket,&listen_set);
    socket_set.push_back(listen_socket);
    highest_socket=listen_socket;
    FD_SET(0, &listen_set);
}


/***
 *
 * Client registration adds the client information into the database
 *
 ***/
void client_registration(HRegistMessage *m, int client_socket){

    printf("Client registration! (%d)\n", client_socket);

    /* store information about the client into the local database */

    FD_SET(client_socket,&listen_set);
    socket_set.push_back(client_socket);
    if (client_socket>highest_socket)
        highest_socket=client_socket;

    clientSignals[client_socket]=m->get_param();
    int id = m->get_id();

    printf("id :: %d \n", id);

    if (id) {
        /* the client is relocating its communicating agent */
        /* the server dissacociates the old socket with the client, and
         * uses the new one instead */
        int cs = clientSocket[id];
        clientSocket[id] = client_socket;
        char *appName = clientName[cs];
        printf("appName:: %s \n", appName);
        clientInfo[appName]=client_socket;
        clientName.erase(cs);
        clientName[client_socket] = appName;

        FD_CLR(cs, &listen_set);
        if (cs==*socketIterator)
            socketIterator++;
        socket_set.remove(cs);
        if (highest_socket==cs)
            highest_socket--;
    } else {
        /* generate id for the new client */
        id = id_set.front();
        id_set.pop_front();
        clientSocket[id] = client_socket;
    }
    clientId[client_socket] = id;
    printf("Send registration (id %d) confimation !\n", id);

    /* send the confirmation message with the client_id */
    HRegistMessage *mesg=new HRegistMessage(HMESG_CLIENT_REG,client_socket);
    mesg->set_id(id);
    send_message(mesg, client_socket);
    delete mesg;
}

/*
 * Is the given point valid?
 */
void projection_is_valid(HDescrMessage *mesg, int client_socket) {

    // first get the query string
    char* temp = (char *) malloc(strlen(mesg->get_descr())+10);
    strcpy(temp, mesg->get_descr());
    printf("got the request point: %s \n", temp);
    string str_point(temp);
    int return_val = 1;

    // divide the co-ordinate values to independent groups
    vec_of_intvec vec_of_vec = get_vectors(str_point);
    for(int i=0; i < num_groups; i++) {
        printf("gets here \n");
        int temp_dim=group_info[i];
        vector<int> temp_vector = vec_of_vec.at(i);
        cout << "the point:::::: " << vector_to_string(temp_vector);
        if(!is_valid(trees.at(i), temp_vector, temp_dim))
        {
            return_val = 0;
            break;
        }
    }
    HRegistMessage *m = new HRegistMessage(HMESG_CONFIRM,return_val);
    send_message(m,client_socket);
    delete m;
    delete mesg;
}


void do_projection_entire_simplex(HDescrMessage *mesg, int client_socket) {

    // first get the query string:: for projection of the entire simplex
    // the request string is assumed be in the following format
    //     point_1:point_2: ... : point_n
    //     where point_k is in the following format:
    //       value_dim_1 value_dim_2 ... value_dim_n, assuming we have n
    //       dimensionsal search space
    char* temp = (char *) malloc(strlen(mesg->get_descr())+10);
    strcpy(temp, mesg->get_descr());
    printf("got the request point: %s \n", temp);
    string str_points(temp);
    //tokenize the string
    StringTokenizer strtok(str_points, ":");
    // we need to return a list of lists
    string return_string = "[list ";

    while(strtok.hasMoreTokens())
    {
        string str_point = strtok.nextToken();
        return_string += " [list ";
        vec_of_intvec vec_of_vec = get_vectors(str_point);
        for(int i=0; i < num_groups; i++) {
            int temp_dim=group_info[i];
            vector<int> temp_vector = vec_of_vec.at(i);
            ANNpoint point = make_point_from_vector(temp_vector,temp_dim);
            if(!is_valid(trees.at(i),temp_vector, temp_dim)) 
            {
                return_string += get_neighbor(trees.at(i),point,nn_num[i]);
            }
            else
            {
                return_string += vector_to_string(temp_vector);
            }
        }
        return_string += "]";
    }
    return_string += "]";
    char *char_star;
    char_star = new char[return_string.length() + 1];
    strcpy(char_star, return_string.c_str());

    HDescrMessage *m = new HDescrMessage(HMESG_PROJ_RESULT,char_star,strlen(char_star));
    send_message(m,client_socket);
    delete m;
    delete mesg;
    delete [] char_star;
}




// project a single point
void do_projection_one_point(HDescrMessage *mesg, int client_socket) {

    // first get the query string
    char* temp = (char *) malloc(strlen(mesg->get_descr())+10);
    strcpy(temp, mesg->get_descr());
    printf("got the request point: %s \n", temp);
    string str_point(temp);

    string return_string = "[list ";

    vec_of_intvec vec_of_vec = get_vectors(str_point);
    for(int i=0; i < num_groups; i++) {
        int temp_dim=group_info[i];
        vector<int> temp_vector = vec_of_vec.at(i);
        ANNpoint point = make_point_from_vector(temp_vector,temp_dim);
        if(!is_valid(trees.at(i),temp_vector, temp_dim)) 
        {
            return_string += get_neighbor(trees.at(i),point,nn_num[i]);
        }
        else
        {
            return_string += vector_to_string(temp_vector);
        }
    }
    return_string += "]";
    char *char_star;
    char_star = new char[return_string.length() + 1];
    strcpy(char_star, return_string.c_str());

    HDescrMessage *m = new HDescrMessage(HMESG_PROJ_RESULT,char_star,strlen(char_star));
    send_message(m,client_socket);
    delete m;
    delete mesg;
    delete [] char_star;
}

// We can use either the L1 distance or the L2 distance to gather nearest
// neighbors for a given center point. If we use L1, we will get points on
// concentric hyper-spheres.

// When we want to combine nearest neighbors from independent
// groups:

void simplex_construction(HDescrMessage *mesg, int client_socket)
{
    // Assumption: The string message is of the form :
    //    radius point
    //    [if different radii]: radius1:radius2:...

    // first get the query string
    char* temp = (char *) malloc(strlen(mesg->get_descr())+10);
    strcpy(temp, mesg->get_descr());
    printf("simplex construction :::: got the request point: %s \n", temp);
    string str_point(temp);
    StringTokenizer strtok(str_point, " ");
    //string r_str = strtok.nextToken();
    int radii[num_groups];
    int radius=1;

    // use different scaling?
    if(use_diff_radius == 1)
    {
        string r_str = strtok.nextToken();
        StringTokenizer radtok (r_str, ":");
        if(radtok.countTokens() != num_groups) {
            fprintf(stderr, "not enough radius values \n");
        } else {
            for(int i = 0; i < num_groups; i++)
            {
                radii[i]=radtok.nextIntToken();
            }
        }
    } else {
        radius = strtok.nextIntToken();
    }
    printf("radius: %d \n", radius);
    string remaining    = strtok.remainingString();
    cout << "remaining: ";
    cout << remaining;
    cout << "\n";

    // first make sure the given init_point is a valid point : make a call
    // to projection function : could move this to a new routine later on
    vec_of_intvec vec_of_vec = get_vectors(remaining);
    string projected_init_point="";
    for(int i=0; i < num_groups; i++) {
        int temp_dim=group_info[i];
        vector<int> temp_vector = vec_of_vec.at(i);
        ANNpoint point = make_point_from_vector(temp_vector,temp_dim);
        if(!is_valid(trees.at(i),temp_vector, temp_dim))
        {
            projected_init_point += get_neighbor(trees.at(i),point,nn_num[i]);
        }
        else
        {
            projected_init_point += vector_to_string(temp_vector);
        }
    }
    cout << "Init Point projected from " << remaining << " to " << projected_init_point << "\n";

    // now constructed the simplex around the projected point
    vec_of_vec = get_vectors(projected_init_point);
    string return_string = "\n global init__ \n set init__ [list ";
    for(int i=0; i < num_groups; i++)
    {
        int temp_dim=group_info[i];
        vector<int> temp_vector = vec_of_vec.at(i);
        ANNpoint init_point = make_point_from_vector(temp_vector, temp_dim);
        if(use_diff_radius)
        {
            return_string += get_neighbor_at_dist(trees.at(i), radii[i], init_point, nn_num[i], i);
        } else
        {
            return_string += get_neighbor_at_dist(trees.at(i), radius, init_point, nn_num[i], i);
        }
    }
    return_string+="]";

    cout << "Initial Simplex Possible points:: " << return_string << "\n";
    char *char_star;
    char_star = new char[return_string.length() + 1];
    strcpy(char_star, return_string.c_str());
    //FILE * pFile;
    FILE *pFile;
    pFile = fopen ("/hivehomes/tiwari/tutorial-2010/harmony/bin/constructed_simplex.tcl","w");
    if (pFile!=NULL)
    {
        fputs (char_star,pFile);
        fclose (pFile);
    }
    HDescrMessage *mesg_desr=new HDescrMessage(HMESG_SIM_CONS_RESULT,char_star,strlen(char_star));
    send_message(mesg_desr, client_socket);
    delete mesg_desr;
    delete [] char_star;
}

/****
     Name:         revoke_resource
     Description:  calls scheduler to revoke allocated resources,
     remove tcl names related to the disrupted application, and
     start a new scheduling round
     Called from:  cilent_unregister
****/
void revoke_resources(char *appName) {
    fprintf(stderr, "revoke_resources: %s\n", appName);
    free(clientName[clientInfo[appName]]);
}

/***
 *
 * The client unregisters! Send back a confirmation message
 *
 ***/
void client_unregister(HRegistMessage *m, int client_socket) {

    /* clear all the information related to the client */

    printf("ENTERED CLIENT UNREGISTER!!!!!\n");

    FD_CLR(client_socket, &listen_set);
    if (client_socket==*socketIterator)
        socketIterator++;
    socket_set.remove(client_socket);
    if (highest_socket==client_socket)
        highest_socket--;
    clientSocket.erase(clientId[client_socket]);
    clientId.erase(client_socket);

    if (m == NULL){
        /* revoke resources */
        revoke_resources(clientName[client_socket]);
    } else {
        printf("Send confimation !\n");
        HRegistMessage *mesg = new HRegistMessage(HMESG_CONFIRM,0);

        /* send back the received message */
        send_message(mesg,client_socket);
        delete mesg;
    }

    fprintf(stderr, "Exit UNREGISTER!\n");
}

int in_process_message = FALSE;
void process_message_from(int temp_socket){

    HMessage *m;
    assert(!in_process_message);
    in_process_message = TRUE;
    printf("Waiting...%d...%d...\n",getpid(),temp_socket);

    /* get the message type */
    m=receive_message(temp_socket);

    if (m == NULL){
        client_unregister((HRegistMessage*)m, temp_socket);
        in_process_message = FALSE;
        return;
    }

    printf("Message has type: %d\n", m->get_type());

    switch (m->get_type()) {
        case HMESG_CLIENT_REG:
            client_registration((HRegistMessage *)m,temp_socket);
            break;
        case HMESG_CLIENT_UNREG:
            client_unregister((HRegistMessage *)m, temp_socket);
            break;
        case HMESG_IS_VALID:
            projection_is_valid((HDescrMessage*)m, temp_socket);
            break;
        case HMESG_SIM_CONS_REQ:
            simplex_construction((HDescrMessage*)m, temp_socket);
            break;
        case HMESG_PROJ_ONE_REQ:
            do_projection_one_point((HDescrMessage*)m, temp_socket);
            break;
        case HMESG_PROJ_ENT_REQ:
            do_projection_entire_simplex((HDescrMessage*)m, temp_socket);
            break;
        default:
            printf("Wrong message type!\n");
    }
    in_process_message = FALSE;
}


/***
 *
 * The main loop of the server
 *
 * We only exit on Ctrl-C
 *
 ***/
void main_loop() {

    struct sockaddr_in temp_addr;
    int temp_addrlen;
    int temp_socket;
    int pid;
    int active_sockets;
    fd_set listen_set_copy;

    /* do listening to the socket */
    if (listen(listen_socket, 10)) {
        h_exit("Listen to socket failed!");
    }
    while (1) {
        listen_set_copy=listen_set;
        //    printf("Select\n");
        active_sockets=select(highest_socket+1,&listen_set_copy, NULL, NULL, NULL);

        //printf("We have %d requests\n",active_sockets);

        /* we have a communication request */
        for (socketIterator=socket_set.begin();socketIterator!=socket_set.end();socketIterator++) {
            if (FD_ISSET(*socketIterator, &listen_set_copy) && (*socketIterator!=listen_socket) && (*socketIterator!=xfd)) {
                /* we have data from this connection */
                process_message_from(*socketIterator);
            }
        }
        if (FD_ISSET(listen_socket, &listen_set_copy)) {
            /* we have a connection request */

            if ((temp_socket = accept(listen_socket,(struct sockaddr *)&temp_addr, (unsigned int *)&temp_addrlen))<0) {
                //	h_exit("Accepting connections failed!");
            }
            fprintf(stderr, "accepted a client\n");
            process_message_from(temp_socket);
        }
        if (FD_ISSET(xfd, &listen_set_copy)) {
            char line[1024];
            int ret1 = read(0, line, sizeof(line));
            assert (ret1 >= 0);
            line[ret1] = '\0';
            fflush(stdout);
        }
    }
}

/***
 *
 * This is the main program.
 * It first checks the arguments.
 * It then initializes and goes for the main loop.
 *
 ***/
int main(int argc, char **argv) {

    check_parameters(argc, argv);

    /* do all the connection related stuff */
    server_startup();

    /* once all connection related stuff are done, make a call to ANN
     * intialization - to construct the trees ...
     */
    ANN_initialize();

    main_loop();
}

/******************************************************************
 * ANN WRAPPERS
 ******************************************************************/

// read point (false on EOF) utility
bool read_point(istream &in, ANNpoint p, int dim) {
  for (int i = 0; i < dim; i++) {
    if(!(in >> p[i]))
      return false;
  }
  maxpts++;
  return true;
}


// print point utility
void print_point(ostream &out, ANNpoint p, int dim) {
  out << " " << p[0];
  for (int i = 1; i < dim; i++) {
    out << "  " << p[i];
  }
  out << " ";
}

string vector_to_string(vector<int> v)
{
    stringstream ss;
    ss << " ";
    for (int i = 0; i < v.size(); i++)
    {
        ss << v.at(i) << " ";
    }
    return ss.str();
}

// given an int array, this returns a ANNpoint object
ANNpoint make_point(int coords[], int dim) {
  ANNpoint query_point;
  query_point = annAllocPt(dim);
  for(int index = 0; index < dim; index++) {
    query_point[index]=coords[index];
    printf("coords: %d ", coords[index]);
  }
  printf("\n");
  return query_point;
}

// given a vector of co-ordinates, this returns an ANNpoint object
ANNpoint make_point_from_vector(vector<int> vals, int dim)
{
    ANNpoint query_point;
    query_point = annAllocPt(dim);
    for(int index = 0; index < dim; index++) {
        query_point[index]=vals.at(index);
    }
    return query_point;
}

// given a file name, this gives the total number of lines in the file.
int get_line_count(string filename) {
    int num_lines=0;
    string line;
    ifstream myfile;

    myfile.open(filename.c_str());
    if (myfile.is_open()) {
        while (! myfile.eof()) {
            getline (myfile,line);
            num_lines++;
        }
        myfile.close();
    }
    else cout << "Unable to open file";
    return num_lines;
}

// this returns a vector of vector<int>s. each vector is a group of parameters
// that are related to one another by one or more constraints
//  uses global variable num_groups and group_info to divide the point
//  co-ordinate values to independent groups
vec_of_intvec get_vectors (string point)
{
    vec_of_intvec matrix;
    StringTokenizer strtok (point, " ");
    // num_groups :: how many independent groups?
    for(int i=0; i < num_groups; i++)
    {
        vector<int> temp_vector;
        // group_info :: how to divide up the values?
        for(int j=0; j < group_info[i];j++)
        {
            int temp = strtok.nextIntToken();
            temp_vector.push_back(temp);
        }
        matrix.push_back(temp_vector);
    }
    return matrix;
}

/* This is called at the beginning to initialize all the relevant globals.
 *   --> Read in the data point file and construct the k-d tree. This is a
 *       one time cost at the beginning of the search.

 *    Parameters: dimension of the search space, # of neighbors, space
 *    points file
 */
tree_struct construct_tree(int dim_, int k_, string filename)
{
    struct tree_struct return_struct;

    ANNkd_tree* kdTree=NULL;
    int nPts = 0;
    static ifstream dataStream;
    istream* dataIn=NULL;
    // just set this to some large value for now
    int maxpts=2155120;

    return_struct.dim = dim_;
    return_struct.k = k_;

    int dim = dim_;
    int k = k_;

    // get the total number of points in the search space
    maxpts = get_line_count(filename) - 1;

    return_struct.maxpts = maxpts;

    //cout << "maxPts: " << maxPts;
    ANNpointArray dataPts = annAllocPts(maxpts, dim);
    return_struct.dataPts = dataPts;

    //open the points file
    dataStream.open(filename.c_str(), ios::in);// open data file
    if (!dataStream) {
        cerr << "Cannot open data file\n";
        exit(1);
    }

    dataIn = &dataStream;
    //tree_struct.dataIn = dataIn;

    while (nPts < maxpts && read_point(*dataIn, dataPts[nPts], dim)) {
        nPts++;
    }

    kdTree=new ANNkd_tree(dataPts,nPts,dim);
    return_struct.kdTree=kdTree;

    dataStream.close();

    return return_struct;;
}

void ANN_initialize() {

    /* if only one co-ordinates file is defined, that is if there are no independent groups of variables,
       then we use one ANN tree to find the nearest neighbors. */
    if(use_multiple_trees == 0) {
        tree_struct tree = construct_tree(group_info[0],dummy_k,file_names[0]);
        cout << "Done constructing k-d-tree from " << file_names[0] << " file ... \n";
        trees.push_back(tree);
    }
    else if (use_multiple_trees == 1) {
        /* construct trees for each independent dimensions */
        for(int i = 0; i < num_groups; i++) {
            tree_struct tree = construct_tree(group_info[i],dummy_k,file_names[i]);
            cout << "Done constructing k-d-tree from " << file_names[i] << " file ... \n";
            trees.push_back(tree);
        }
    }
    cout << "ANN Initialization Done ... \n";
}

bool is_valid(tree_struct stree, vector<int> vals, int temp_dim) {

    ANNkd_tree *kdTree = stree.kdTree;
    int dim = stree.dim;
    ANNpointArray dataPts = stree.dataPts;

    // over-write the value of k to be 1
    int k_temp = 1;
    bool return_val=0;
    ANNidxArray nnIdx;            // near neighbor indices
    ANNdistArray dists;           // near neighbor distances
    nnIdx = new ANNidx[k_temp];   // allocate near neigh indices
    dists = new ANNdist[k_temp];
    cout << " is valid is testing this point:::: " << vector_to_string(vals) << endl;
    ANNpoint queryPt = make_point_from_vector(vals, temp_dim);
    kdTree->annkSearch(		// search
        queryPt,	// query point
        k_temp,	// number of near neighbors
        nnIdx,	// nearest neighbors (returned)
        dists,	// distance (returned)
        eps);	// error bound

    cout << "The distance is::::: " << dists[0] << endl;
    
    // just check if the distance is 0.
    if(dists[0] == 0)
        return_val=1;

    return return_val;
}

int max_radius(tree_struct stree, int dist,
               ANNpoint queryPt,
               int k_, int group)
{
    return 0;

}

// remember that dist is a squared radius
vector<int> get_neighbor_vector(tree_struct stree, int dist, ANNpoint queryPt)
{
    vector<int> return_vec;
    ANNkd_tree *kdTree = stree.kdTree;
    int dim = stree.dim;
    ANNpointArray dataPts = stree.dataPts;

    return return_vec;
}

// get_neighbors_at_dist
string get_neighbor_at_dist(tree_struct stree, int dist,
                              ANNpoint queryPt,
                              int k_, int group)
{
    ANNkd_tree *kdTree = stree.kdTree;
    int dim = stree.dim;
    ANNpointArray dataPts = stree.dataPts;
    int k=k_;
    int done = 1;
    int nPts;	           // actual number of data points
    ANNidxArray nnIdx;       // near neighbor indices
    ANNdistArray dists;      // near neighbor distances
    nnIdx = new ANNidx[k];   // allocate near neigh indices
    dists = new ANNdist[k];

    string return_string ="";

    print_point(cout, queryPt, dim);
    printf("looking for %d neighbors within the radius of %d \n", k, dist);

    // first make sure it is a valid point

    kdTree->annkSearch(		// search
        queryPt,	// query point
        k,	        // number of near neighbors
        nnIdx,	// nearest neighbors (returned)
        dists,	// distance (returned)
        eps);	// error bound


    char buf [8];
    sprintf(buf, "%d", group);
    string group_str(buf);
//    return_string = "global init__" + group_str +"\n set init__" +
//    group_str + " [list ";
    return_string += " [list ";
    for (int i=0; i < k; i++) {
        if (dists[i] == dist) {
            return_string += " [list ";
            for(int j=0; j < dim; j++) {
                stringstream ss;
                double d = (dataPts[nnIdx[i]])[j];
                ss << d;
                string buf =ss.str();
                return_string += buf;
                if(j!=dim-1)
                    return_string += " ";
            }
            return_string += "]";
            if(i!=k-1)
                return_string += " ";
        } 
    }
    return_string += "]\0";
    /*
      FILE * pFile;
      pFile = fopen (filename.c_str(),"a");
      if (pFile!=NULL)
      {
      fputs (return_string.c_str(),pFile);
      fclose (pFile);
      }
      else {
      done = 0;
      }
      return done;
    */
    return return_string;
}

// given a pointer to a kdTree and a query point, this returns k
//  nearest neighbors
string get_neighbor(tree_struct stree, ANNpoint queryPt, int k_) {
    ANNkd_tree *kdTree = stree.kdTree;
    int dim = stree.dim;
    ANNpointArray dataPts = stree.dataPts;
    int k=2;
    int nPts;	           // actual number of data points
    ANNidxArray nnIdx;       // near neighbor indices
    ANNdistArray dists;      // near neighbor distances
    nnIdx = new ANNidx[k];   // allocate near neigh indices
    dists = new ANNdist[k];

    string return_string ="";

    kdTree->annkSearch(		// search
        queryPt,	// query point
        k,	        // number of near neighbors
        nnIdx,	// nearest neighbors (returned)
        dists,	// distance (returned)
        eps);	// error bound
    int i = 1;
    return_string += " ";
    for(int j=0; j < dim; j++) {
        stringstream ss;
        double d = (dataPts[nnIdx[i]])[j];
        ss << d;
        string buf =ss.str();
        return_string += buf;
        if(j!=dim-1)
            return_string += " ";
    }
    return_string += "\0";
    cout << "from get neighbor ::: " << return_string << "\n";
    return return_string;

}
