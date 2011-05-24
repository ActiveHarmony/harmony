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


