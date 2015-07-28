/*
 * Copyright 2003-2015 Jeffrey K. Hollingsworth
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
#ifndef __PSERVER_H__
#define __PSERVER_H__

/***
 *
 * include system headers
 *
 ***/
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
/***
 *
 * include user defined headers
 *
 ***/
#include "putil.h"
#include "pmesgs.h"
#include "psockutil.h"
#include <ANN/ANN.h>

using namespace std;


/***
 *
 * define macros
 *
 ***/

#define SERVER_PORT 2077
#define BUFF_SIZE 1024

/***
 *
 * define prototypes
 *
 ***/
struct tree_struct {
  ANNkd_tree* kdTree;
  int dim;
  ANNpointArray dataPts;
  int k;
  int maxpts;
  istream* dataIn;
};

typedef vector<vector<int> > vec_of_intvec;

vec_of_intvec get_vectors (string point);
string vector_to_string(vector<int> vec);
bool read_point(istream &in, ANNpoint p, int dim);
void print_point(ostream &out, ANNpoint p, int dim);
ANNpoint make_point(int coords[], int dim);
ANNpoint make_point_from_vector(vector<int> v, int dim);
int get_line_count(string filename);
tree_struct construct_tree(int dim_, int k_, string filename);
void ANN_initialize();
bool is_valid(tree_struct stree, vector<int>, int temp_dim);
string get_neighbor_at_dist(tree_struct stree, int dist,
                            ANNpoint queryPt,
                            int k_, int group);
string get_neighbor(tree_struct stree, ANNpoint queryPt, int k_);

#endif /* __PSERVER_H__ */
