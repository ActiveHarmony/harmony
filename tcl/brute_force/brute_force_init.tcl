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
proc brute_force_init {appName} {
    
    global ${appName}_simplex_time
    set ${appName}_simplex_time 0

    global ${appName}_search_done
    set ${appName}_search_done 0

    global int_max_value
    set int_max_value 2147483647

    global best_perf_so_far
    set best_perf_so_far $int_max_value

    global best_coordinate_so_far
    set best_coordinate_so_far {}

    global ${appName}_code_timestep
    set ${appName}_code_timestep 1

    global ${appName}_bundles
    upvar #0 ${appName}_bundles bundles
    foreach bun $bundles {
        upvar #0 ${appName}_bundle_${bun}(value) bunv
        upvar #0 ${appName}_bundle_${bun}(minv) minv
        upvar #0 ${appName}_bundle_${bun} $bun
        puts $bunv
        puts $minv
        set bunv $minv
    }

    global ${appName}_time
}
