#include "ANN_init.h"
#include "StringTokenizer.h"


int maxpts;
double eps = 0;

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

// given a string delimited with spaces, this returns a ANNpoint object
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


// given a pointer to a kdTree and a query point, this returns k
//  nearest neighbors
const char* get_neighbor(tree_struct stree, ANNpoint queryPt, int k_) {
    ANNkd_tree *kdTree = stree.kdTree;
    int dim = stree.dim;
    ANNpointArray dataPts = stree.dataPts;
    int k=k_;
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

    return_string += "";
    return return_string.c_str();

}

// get_neighbors_with_dist
bool get_neighbor_with_dist__(tree_struct stree, int dist,
                              ANNpoint queryPt,string filename,
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

    kdTree->annkSearch(		// search
        queryPt,	// query point
        k,	        // number of near neighbors
        nnIdx,	// nearest neighbors (returned)
        dists,	// distance (returned)
        eps);	// error bound


    char buf [8];
    sprintf(buf, "%d", group);
    string group_str(buf);
    return_string = "global init__" + group_str +"\n set init__" + group_str + " [list ";
    for (int i=0; i < k; i++) {
        if (dists[i] == dist) {
            return_string += "[list ";
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
}



// given a pointer to kdTree and a query point, this retuns a boolean
//  true -- if the point is in the constrained search space
//  false -- otherwise
bool is_valid(ANNkd_tree* kdTree, ANNpoint queryPt) {

    // over-write the value of k to be 1
    int k_temp =1;
    bool return_val=0;
    int nPts;	                // actual number of data points
    ANNidxArray nnIdx;            // near neighbor indices
    ANNdistArray dists;           // near neighbor distances
    nnIdx = new ANNidx[k_temp];   // allocate near neigh indices
    dists = new ANNdist[k_temp];

    string return_string ="";

    kdTree->annkSearch(		// search
        queryPt,	// query point
        k_temp,	// number of near neighbors
        nnIdx,	// nearest neighbors (returned)
        dists,	// distance (returned)
        eps);	// error bound

    // just check if the distance is 0.
    if(dists[0] == 0)
        return_val=1;

    return return_val;
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

/* This is called at the beginning to initialize all the relevant globals.
 *   --> Read in the data point file and construct the k-d tree. This is a
 *       one time cost at the beginning of the search.

 * Parameters: dimension of the search space, # of neighbors, space points file
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
        trees.push_back(tree);
    }

    else if (use_multiple_trees == 1) {
        /* construct trees for each independent dimensions */
        for(int i = 0; i < num_groups; i++) {
            tree_struct tree = construct_tree(group_info[i],dummy_k,file_names[i]);
            trees.push_back(tree);
        }
    }
    cout << "ANN Initialization Done ... \n";
}


/**************************************************************************
Handling the Requests:

Types of requests: 1,2
1: construct initial simplex / new directions simplex
   params: radius
           dimension
           init_point

2: projection
   params: dimension
           point

Assumptions:
   request_string is of the following form:
    1 [space] <radius> [space] <dimension> [space] <init_point> [space]
    2 [space] <dimension> [space] <init_point>

    <init_point> : values along different dimensions are also separated by
                   spaces

      Lets say we have 7 tunable parameters, giving a 7-dimension search
      space. A point in this search space will be represented by
      [a,b,c,d,e,f,g]. Consider that (a,b,c) are constrained by a one/more
      constriants on a,b and c. Lets also assume that (d,e) and (f,g) are
      constrained in a similar fashion. So, essentially we have three
      independent co-ordinate systems and projection on one system will be
      unaffected by the projection on other system. This observation leads
      to a smaller points_file for ANN to do its calculation.

       For this scenario, group_info will be defined as {3,2,2}.

     Looking at a search space where all the parameters are linked to one
     another by one/more constraints, thereby leading to a large points
     file is left for future investigation. [This might not be that bad
     because the actual number of admissible paramsters might not be too
     big.]
**************************************************************************/

const char* handle_request(string request_string) {

    string return_string = "";
    // tokenize the request_string
    string delimiter = " ";
    StringTokenizer strtok(request_string,delimiter);

    // get the request_type
    int request_type= strtok.nextIntToken();

    cout << " request_type:: ";
    cout << request_type;
    cout << "\n";

    if(request_type == 1) {
        // initial/new_directions simplex construction
        // get the squared radius
        int radius = strtok.nextIntToken();
        cout << "radius:: ";
        cout << radius;
        cout << "\n";

        //get the dimension
        int dimension = strtok.nextIntToken();
        cout << "dimension::";
        cout << dimension;
        cout << "\n";

        int coords[dimension];

        for(int i = 0; i < dimension; i++) {
            coords[i] = strtok.nextIntToken();
        }

        int groups[num_groups];
        int offset = 0;
        printf("num_groups:: %d \n", num_groups);
        for(int i=0; i < num_groups; i++) {
            int temp_dim=group_info[i];
            printf("temp_dim:: %d \n", temp_dim);
            int temp_coords[temp_dim];
            for (int j=0; j < temp_dim; j++) {
                temp_coords[j] = coords[offset+j];
            }
            ANNpoint point = make_point__(temp_coords,temp_dim);
            get_neighbor_with_dist__(trees.at(i), radius, point, "temp.tcl", nn_num[i], i);
            offset+=group_info[i];
        }
    }
    if(request_type == 2) {
        return_string += "[list ";
        int dimension = strtok.nextIntToken();
        int coords[dimension];

        for(int i = 0; i < dimension; i++) {
            coords[i] = strtok.nextIntToken();
        }
        int groups[num_groups];
        int offset = 0;
        printf("num_groups:: %d \n", num_groups);
        for(int i=0; i < num_groups; i++) {
            int temp_dim=group_info[i];
            printf("temp_dim:: %d \n", temp_dim);
            int temp_coords[temp_dim];
            for (int j=0; j < temp_dim; j++) {
                temp_coords[j] = coords[offset+j];
            }
            ANNpoint point = make_point__(temp_coords,temp_dim);
            return_string+= get_neighbor(trees.at(i), point,nn_num[i]);
            offset+=group_info[i];
        }

        return_string += "]";
    }

    cout << return_string;
    cout << "\n";
    return return_string.c_str();
}


main(int argc, char* argv[])
{
    /* server's address  */
    struct  sockaddr_in server_address;

    /* port number */
    int     port;

    /* client's address */
    struct  sockaddr_in client_address;
    /* length */
    int     alen;

    /* socket descriptors */
    int     server_socket, connectionSocket;

    char    request_string[128];

    char    capitalizedSentence[128];
    char    buff[128];
    int     i, n;

    /* command line arguments */
    if (argc > 1) {
        port = atoi(argv[1]);
    } else {
        fprintf(stderr,"Usage: %s port-number\n",argv[0]);
        exit(1);
    }

    /* Create a socket */
    server_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        fprintf(stderr, "No socket created... \n");
        exit(1);
    }

    /* Server Bind */
    memset((char *)&server_address,0,sizeof(server_address));
    /* set family to Internet */
    server_address.sin_family = AF_INET;
    /* set the local IP address */
    server_address.sin_addr.s_addr = INADDR_ANY;
    /* set the port number */
    server_address.sin_port = htons((u_short)port);

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        fprintf(stderr,"Bind failed ... \n");
        exit(1);
    }

    /* Specify the size of request queue */
    if (listen(server_socket, 10) < 0) {
        fprintf(stderr,"listen failed... \n");
        exit(1);
    }

    /******************************************
     * ANN INITIALIZATIONS                    *
     ******************************************/

    ANN_initialize();

    /* Main server loop - accept and handle requests */

    while (1) {
        alen = sizeof(client_address);
        if ( (connectionSocket=accept(server_socket, (struct sockaddr *)&client_address,(socklen_t*) &alen)) < 0) {
            fprintf(stderr, "accept failed\n");
            exit(1);
        }
        else {
            printf("Received request \n");
        }

        //printf("checkpoint 1");

        request_string[0]='\0';
        //printf("adfadsf");
        n=read(connectionSocket,buff,sizeof(buff));
        while(n > 0){
            //printf("here?");
            strncat(request_string,buff,n);
            if (buff[n-1]=='\0') break;
            n=read(connectionSocket,buff,sizeof(buff));
        }

        //printf("checkpoint 2");

        /* handle the request */
        string temp(request_string);
        printf("Request String :: ");
        cout << request_string ;
        cout << "\n";

        printf(" Calling the handle request:");
        string temp__ = handle_request(request_string);

        cout << "temp__ :: ";
        cout << temp__;
        cout << "\n";

        //ANNpoint query_point = make_point(temp);

        int dim=3;

        //int neighbor = get_neighbor_with_dist(trees.at(0), 20, query_point, "temp_out.tcl",100000);

        /* Capitalize the sentence */

        /*
          for(i=0; i <= strlen(request_string); i++){
          if((request_string[i] >= 'a') && (request_string[i] <= 'z'))
          capitalizedSentence[i] = request_string[i] + ('A' - 'a');
          else
          capitalizedSentence[i] = request_string[i];
          }
        */
        /* send it to the client */

        //cout << neighbor.c_str();

        //write(connectionSocket, neighbor.c_str(), strlen(neighbor.c_str())+1);

        close(connectionSocket);
    }
}
