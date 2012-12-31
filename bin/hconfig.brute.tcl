#
# Copyright 2003-2012 Jeffrey K. Hollingsworth
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

global search_algorithm
set search_algorithm 4 

## load commons

# comment nearest_neighbor if you are not using ANN based projection
#load ${toBase}/../tcl/common/nearest_neighbor.so
# comment code_server.so if you are not using dynamic code generation
#load ${toBase}/../tcl/common/code_server.so

global init_simplex_method

# change this variable to either "max" (for default simplex construction
#   method or "user_defined" for user defined mode of simplex
#   construction.
set init_simplex_method "max"
#set init_simplex_method "user_defined"

source ${toBase}/../tcl/common/parseApp.tcl
source ${toBase}/../tcl/common/newparseApp.tcl
source ${toBase}/../tcl/common/utilities.tcl
source ${toBase}/../tcl/brute_force/brute_force_init.tcl
source ${toBase}/../tcl/brute_force/brute_force.tcl
