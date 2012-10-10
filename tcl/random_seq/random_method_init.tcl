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
proc random_init {appName} {

    global ${appName}_code_timestep
    set ${appName}_code_timestep 0
    global ${appName}_search_done
    set ${appName}_search_done 0
    global int_max_value
    set int_max_value 2147483647
    global ${appName}_best_perf_so_far
    global ${appName}_best_coordinate_so_far
    set ${appName}_best_perf_so_far $int_max_value
    set ${appName}_best_coordinate_so_far {}

    global ${appName}_max_search_iterations
    set ${appName}_max_search_iterations 100

    global ${appName}_simplex_time
    set ${appName}_simplex_time -1

    upvar #0 ${appName}_bundles bundles
    foreach bun $bundles {
        upvar #0 ${appName}_bundle_${bun}(value) bunv
        upvar #0 ${appName}_bundle_${bun}(minv) minv
        upvar #0 ${appName}_bundle_${bun}(maxv) maxv
        upvar #0 ${appName}_bundle_${bun}(domain) domain
        upvar #0 ${appName}_bundle_${bun} $bun
        set bunv [random_uniform $minv $maxv 1]
        puts $bunv
    }
}
