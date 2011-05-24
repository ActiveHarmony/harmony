#
# Copyright 2003-2011 Jeffrey K. Hollingsworth
#
# This file is part of Active Harmony.
#
# Active Harmony is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Active Harmony is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with Active Harmony.  If not, see <http://www.gnu.org/licenses/>.
#
global search_algorithm
set search_algorithm 1 

#### to disable client windows, set draw_har_windows variable to 0
global draw_har_windows
set draw_har_windows 1 

## load commons
load ../tcl/common/round.so
# comment nearest_neighbor if you are not using ANN based projection
load ../tcl/common/nearest_neighbor.so
# comment code_server.so if you are not using dynamic code generation
#load ../tcl/common/code_server.so
source ../tcl/common/logo.tk

source ../tcl/pro/parseApp_version_8.tcl
source ../tcl/pro/drawApp.tk
source ../tcl/pro/proparseApp_version_8.tcl
source ../tcl/pro/newdrawApp.tk
source ../tcl/pro/pro.tcl
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
source ../tcl/pro/pro_init_generic.tcl
#source ../tcl/pro/pro_init_smg.tcl
#source ../tcl/pro/pro_init_irs.tcl
#source ../tcl/pro/pro_init_gemm.tcl

