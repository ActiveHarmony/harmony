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
proc simplex_init {appName} {
    
    global ${appName}_simplex_step
    global ${appName}_simplex_points
    global ${appName}_simplex_npoints
    global ${appName}_simplex_performance
    global ${appName}_simplex_low
    global ${appName}_simplex_high
    global ${appName}_simplex_nhigh
    global ${appName}_simplex_alpha
    global ${appName}_simplex_beta
    global ${appName}_simplex_gamma
    global ${appName}_simplex_centroid
    global ${appName}_simplex_pstar
    global ${appName}_simplex_ystar
    global ${appName}_simplex_pdstar
    global ${appName}_simplex_ydstar
    global ${appName}_simplex_time
    global ${appName}_code_timestep
    global ${appName}_no_underscore_name
    global ${appName}_search_done
    global ${appName}_next_iteration

    # simplex_history maintains recent simplices and is used in the termination test.
    global ${appName}_simplex_history
    set ${appName}_simplex_history {}

    set g_name [string range $appName 0 [expr [string last "_" $appName]-1]]

    puts "global name $g_name"

    set int_max_value 2147483647
    global ${appName}_best_perf_so_far
    set ${appName}_best_perf_so_far $int_max_value

    global ${appName}_best_coordinate_so_far
    set ${appName}_best_coordinate_so_far {}

    global ${g_name}_next_iteration
    set ${g_name}_next_iteration 1

    set ${appName}_code_timestep 0
    set ${appName}_simplex_step 0

    set ${appName}_simplex_npoints 0

    set ${appName}_simplex_low(index) -1
    set ${appName}_simplex_low(value) 0

    set ${appName}_simplex_high(index) -1
    set ${appName}_simplex_high(value) 0
  
    set ${appName}_simplex_nhigh(index) -1
    set ${appName}_simplex_nhigh(value) -1

    set ${appName}_simplex_alpha 1
    set ${appName}_simplex_beta 0.5
    set ${appName}_simplex_gamma 2

    set ${appName}_simplex_pstar {}
    set ${appName}_simplex_ystar -1
    set ${appName}_simplex_pdstar {}
    set ${appName}_simplex_ydstar -1

    set ${appName}_simplex_time -1

    set ${appName}_simplex_performance {}
    set ${appName}_simplex_first_call 1
    set ${appName}_search_done 0
    set ${appName}_next_iteration 1

    global ${appName}_time
    if {![info exists ${appName}_time]} {
        set ${appName}_time 0
    }
}
