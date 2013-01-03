#
# Copyright 2003-2013 Jeffrey K. Hollingsworth
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
global harmony_root
if { [string length harmony_root] == 0 } {
    error "$$harmony_root must be set prior to evaluating this file"
}

global search_algorithm
set search_algorithm 2 

## Nelder Mead Simplex Algorithm
source ${harmony_root}/libexec/tcl/nm/parseApp.tcl
source ${harmony_root}/libexec/tcl/nm/newparseApp.tcl
source ${harmony_root}/libexec/tcl/nm/utilities.tcl
source ${harmony_root}/libexec/tcl/nm/nm.tcl
source ${harmony_root}/libexec/tcl/nm/nm_init.tcl

global init_simplex_method
# change this variable to either "max" (for default simplex construction
#   method or "user_defined" for user defined mode of simplex
#   construction.
#set init_simplex_method "min" 
set init_simplex_method "max"
