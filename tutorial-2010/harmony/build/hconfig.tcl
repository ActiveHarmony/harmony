global search_algorithm
# search algorithms
# 1. Parallel Rank Ordering
# 2. Nelder-Mead Simplex
# 3. Random
# 4. Brute Force

set search_algorithm 1 

#### to disable client windows, set draw_har_windows variable to 0
global draw_har_windows
set draw_har_windows 1 

## load commons
load ../tcl/common/round.so
# comment nearest_neighbor if you are not using ANN based projection
#load ../tcl/common/nearest_neighbor.so
# comment code_server.so if you are not using dynamic code generation
#load ../tcl/common/code_server.so
source ../tcl/common/logo.tk

if { $search_algorithm == 1 } {
	## parallel rank ordering
	puts "Using the PRO algorithm"
	source ../tcl/pro/parseApp_version_8.tcl
	source ../tcl/pro/drawApp.tk
	source ../tcl/pro/sched.tcl
	source ../tcl/pro/proparseApp_version_8.tcl
	source ../tcl/pro/newdrawApp.tk
	source ../tcl/pro/pro_version_10.tcl
	source ../tcl/pro/initial_simplex_construction.tcl
	source ../tcl/pro/random_uniform.tcl
	source ../tcl/pro/utilities.tcl
	source ../tcl/pro/matrix.tcl
	source ../tcl/pro/pro_transformations.tcl
	source ../tcl/pro/projection.tcl
	source ../tcl/pro/combine.tcl

	## pro_init_<appname>.tcl contains several parameters that users can use
	##  configure the search. The most important ones are: # of tunable parameters, 
	##   simplex_size, initial simplex construction method, local search method, 
	##   reflection/expansion/shrink coefficients, whether to use space exploration
	##   before starting the search algorithm etc. 
	## As a starting point, take a look at the generic init file provided with 
	##  this distribution.
	#source ../tcl/pro/pro_init_generic.tcl
	source ../tcl/pro/pro_init_gemm.tcl
	#source ../tcl/pro/pro_init_smg.tcl
	#source ../tcl/pro/pro_init_irs.tcl
} elseif { $search_algorithm == 2 } {
	## Nelder Mead Simplex Algorithm
	source ../tcl/common/parseApp.tcl
	source ../tcl/common/drawApp.tk
	source ../tcl/common/newparseApp.tcl
	source ../tcl/common/newdrawApp.tk
	source ../tcl/common/utilities.tcl
	source ../tcl/simplex/simplex.tcl

	global init_simplex_method

	# change this variable to either "max" (for default simplex construction
	#   method or "user_defined" for user defined mode of simplex
	#   construction.
	set init_simplex_method "max" 

} elseif { $search_algorithm == 3 } {
	## random

       global init_simplex_method

        # change this variable to either "max" (for default simplex construction
        #   method or "user_defined" for user defined mode of simplex
        #   construction.
        set init_simplex_method "max"

       source ../tcl/common/utilities.tcl
       source ../tcl/common/parseApp.tcl
       source ../tcl/common/drawApp.tk
       source ../tcl/common/newparseApp.tcl
       source ../tcl/common/newdrawApp.tk
       source ../tcl/random_seq/random_method.tcl

} elseif { $search_algorithm == 4 } {
	global init_simplex_method

        # change this variable to either "max" (for default simplex construction
        #   method or "user_defined" for user defined mode of simplex
        #   construction.
        set init_simplex_method "max"
        #set init_simplex_method "user_defined"

	source ../tcl/common/parseApp.tcl
	source ../tcl/common/drawApp.tk
	source ../tcl/common/newparseApp.tcl
	source ../tcl/common/newdrawApp.tk
	source ../tcl/common/utilities.tcl
	source ../tcl/brute_force/brute_force_init.tcl
	source ../tcl/brute_force/brute_force.tcl
	

}

