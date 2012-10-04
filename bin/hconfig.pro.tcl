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

# Store the path to the base directory
global toBase
set toBase [file dirname [info script]]

puts "server is reading the tcl files"
global search_algorithm
set search_algorithm 1 

## load commons

# comment nearest_neighbor if you are not using ANN based projection
#load ${toBase}/../tcl/common/nearest_neighbor.so

# comment code_server.so if you are not using dynamic code generation
#load ${toBase}/../tcl/common/code_server.so

puts "the backend files."
source ${toBase}/../tcl/common/utilities.tcl
source ${toBase}/../tcl/pro/utilities.tcl

source ${toBase}/../tcl/pro/parseApp_version_8.tcl
source ${toBase}/../tcl/pro/proparseApp_version_8.tcl
source ${toBase}/../tcl/pro/pro.tcl
source ${toBase}/../tcl/pro/initial_simplex_construction.tcl
source ${toBase}/../tcl/pro/random_uniform.tcl
source ${toBase}/../tcl/pro/matrix.tcl
source ${toBase}/../tcl/pro/pro_transformations.tcl
source ${toBase}/../tcl/pro/projection.tcl
source ${toBase}/../tcl/pro/combine.tcl

## pro_init_<appname>.tcl contains several parameters that users can use
##  configure the search. The most important ones are: # of tunable parameters, 
##   simplex_size, initial simplex construction method, local search method, 
##   reflection/expansion/shrink coefficients, whether to use space exploration
##   before starting the search algorithm etc. 
## As a starting point, take a look at the generic init file provided with 
##  this distribution.
#source ${toBase}/../tcl/pro/pro_init_generic.tcl
#source ${toBase}/../tcl/pro/pro_init_smg.tcl
#source ${toBase}/../tcl/pro/pro_init_irs.tcl
source ${toBase}/../tcl/pro/pro_init_gemm.tcl

puts "done loading the tcl related files"

