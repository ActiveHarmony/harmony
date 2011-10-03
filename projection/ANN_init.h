
/* for mm */ 
// use multiple trees?
int use_multiple_trees = 1;
// if multiple trees, how many independent projection components?
// two trees: one for tiling points and the other for unrolling points
// they are projected independently
int num_groups = 2;

// keeps track of all the ANN trees constructed
vector <tree_struct> trees;

// space dimension
int space_dimension = 5;

// how are the co-ordinates grouped into independent dimensions?
// TI,TJ,TK for the first group, thus 3
// UI, UJ for the second group
int group_info[2] = {3,2};

// points file name
string file_names[2] = {"TI_TJ_TK_gemm", "UI_UJ_gemm"};

// use different scaling?
int use_diff_radius=1;

// how many neighbors to consider?
int nn_num[2] = {2500,5};
int dummy_k = 5000;



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
