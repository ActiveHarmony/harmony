/* for rosenbrock
int use_multiple_trees = 0;
// if multiple trees, how many independent projection components?
int num_groups = 1;
// keeps track of all the ANN trees constructed
vector <tree_struct> trees;
// space dimension
int space_dimension = 2;
// how are the co-ordinates grouped into independent dimensions?
int group_info[1] = {2};
// points file name
string file_names[2] = {"rosen_data.pts"};
// use different scaling?
int use_diff_radius=0;
// how many neighbors to consider?
int nn_num[1] = {50000};
int dummy_k = 4;
*/

/* for mm
// use multiple trees?
int use_multiple_trees = 1;
// if multiple trees, how many independent projection components?
int num_groups = 3;

// keeps track of all the ANN trees constructed
vector <tree_struct> trees;

// space dimension
int space_dimension = 5;

// how are the co-ordinates grouped into independent dimensions?
int group_info[3] = {2,2,1};

// points file name
string file_names[3] = {"tiling_redblack_1.dat", "tiling_redblack_2.dat", "unrolling_redblack.dat"};

// use different scaling?
int use_diff_radius=1;

// how many neighbors to consider?
int nn_num[3] = {5000,5000,2};
int dummy_k = 4;
*/


/* for mm 
// use multiple trees?
int use_multiple_trees = 1;
// if multiple trees, how many independent projection components?
int num_groups = 2;

// keeps track of all the ANN trees constructed
vector <tree_struct> trees;

// space dimension
int space_dimension = 5;

// how are the co-ordinates grouped into independent dimensions?
int group_info[2] = {3,2};

// points file name
string file_names[2] = {"tiling_points_mm_reduced_8_8.dat", "unrolling_points_mm.dat"};

// use different scaling?
int use_diff_radius=1;

// how many neighbors to consider?
int nn_num[2] = {25000,50};
int dummy_k = 4;
*/


/* for trsm 
// use multiple trees?
int use_multiple_trees = 1;
// if multiple trees, how many independent projection components?
int num_groups = 3;

// keeps track of all the ANN trees constructed
vector <tree_struct> trees;

// space dimension
int space_dimension = 7;

// how are the co-ordinates grouped into independent dimensions?
int group_info[3] = {3,2,2};

// points file name
string file_names[3] = {"tiling_points_trsm.dat", "unrolling_points_mm.dat","unrolling_points_trsm.dat"};

// use different scaling?
int use_diff_radius=1;

// how many neighbors to consider?
int nn_num[3] = {5000,50,50};
int dummy_k = 50;

*/

/* for LU  
// use multiple trees?
int use_multiple_trees = 1;
// if multiple trees, how many independent projection components?
int num_groups = 5;

// keeps track of all the ANN trees constructed
vector <tree_struct> trees;

// space dimension
int space_dimension = 11;

// how are the co-ordinates grouped into independent dimensions?
int group_info[5] = {1,3,3,2,2};

// points file name
string file_names[5] = {"lu_tj.dat", "tiling_points_mm.dat","tiling_points_trsm.dat", "unrolling_points_mm.dat","unrolling_points_trsm.dat"};

// use different scaling?
int use_diff_radius=1;

// how many neighbors to consider?
int nn_num[5] = {20,50000,50000,50,50};
int dummy_k = 4;

*/


/* for smg2000 */
/*
// use multiple trees?
int use_multiple_trees = 1;
// if multiple trees, how many independent projection components?
int num_groups = 2;

// keeps track of all the ANN trees constructed
vector <tree_struct> trees;

// space dimension
int space_dimension = 5;

// how are the co-ordinates grouped into independent dimensions?
int group_info[2] = {2,3};

// points file name
string file_names[2] = {"ti_ui_smg", "us_tj_tk_smg"};

// use different scaling?
int use_diff_radius=1;

// how many neighbors to consider?
int nn_num[2] = {400,1000};
int dummy_k = 200;

*/

/* for IRS */
// use multiple trees?
int use_multiple_trees = 0;
// if multiple trees, how many independent projection components?
int num_groups = 1;

// keeps track of all the ANN trees constructed
vector <tree_struct> trees;

// space dimension
int space_dimension = 6;

// how are the co-ordinates grouped into independent dimensions?
int group_info[1] = {6};

// points file name
string file_names[1] = {"final_irs_data"};

// use different scaling?
int use_diff_radius=0;

// how many neighbors to consider?
int nn_num[1] = {50000};
int dummy_k = 5000;
 
