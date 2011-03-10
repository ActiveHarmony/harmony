global search_algorithm
set search_algorithm 2 

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

## Nelder Mead Simplex Algorithm
source ../tcl/common/parseApp.tcl
source ../tcl/common/drawApp.tk
source ../tcl/common/newparseApp.tcl
source ../tcl/common/newdrawApp.tk
source ../tcl/common/utilities.tcl
source ../tcl/simplex/simplex.tcl
source ../tcl/simplex/simplex_init.tcl

global init_simplex_method
# change this variable to either "max" (for default simplex construction
#   method or "user_defined" for user defined mode of simplex
#   construction.
set init_simplex_method "min" 

