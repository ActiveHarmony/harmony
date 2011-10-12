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
proc brute_force {appName} {
    # record the best performance, the rest are in the log files
    upvar #0 ${appName}_obsGoodness(value) this_perf
    upvar #0 best_perf_so_far best_perf_so_far
    upvar #0 best_coordinate_so_far best_point

    # book-keeping
    if { $this_perf < $best_perf_so_far } {
        set best_perf_so_far $this_perf
        # collect the values
        upvar #0 ${appName}_bundles bundles
        upvar #0 best_coordinate_so_far best_point
        set temp_ls {}
        foreach bun $bundles {
            upvar #0 ${appName}_bundle_${bun}(value) val
            lappend temp_ls $val
        }
        set best_point $temp_ls
    }
    puts -nonewline "best coord so far :: "
    puts [list_to_string  $best_point]
    puts -nonewline "best perf so far :: "
    puts  $best_perf_so_far
    
    upvar #0 ${appName}_search_done search_done
    # generate new conf
    if { $search_done == 0 } {
        brute_force_modify $appName 0
    }
    global ${appName}_simplex_time
    incr ${appName}_simplex_time
}

proc brute_force_modify {appName param} {
    upvar #0 ${appName}_bundles bundles
    set bun [lindex $bundles $param]
    puts "Brute force Modify: $bun"

    upvar #0 ${appName}_bundle_${bun}(value) bunv
    upvar #0 ${appName}_bundle_${bun}(minv) minv
    upvar #0 ${appName}_bundle_${bun}(maxv) maxv
    upvar #0 ${appName}_bundle_${bun}(stepv) stepv
    upvar #0 ${appName}_bundle_${bun} $bun

    set overflow 0 
    if {$bunv == $maxv} {
        incr overflow
    } else {
        set bunv [expr $bunv+$stepv]
        #redraw_dependencies $bun $appName 0 0
    }
    puts "Param value: $bunv"
    if {$overflow > 0} {
        puts "Reached max"
        if { [expr $param+1] > [expr [llength $bundles]-1] } {
            brute_force_done $appName
        } else {
            brute_force_modify $appName [expr $param+1]
            set bunv $minv
            #redraw_dependencies $bun $appName 0 0
        }
    }

    # Update any dependant local variables, if they exist.
    upvar #0 ${appName}_bundle_${bun}(deplocals) deplocals
    if { [info exists deplocals] } {
        foreach dep $deplocals {
            upvar #0 $dep dep_bun
            set dep_bun(value) $bunv
        }
    }
}

proc brute_force_done { appName } {
    upvar #0 ${appName}_search_done search_done
    set search_done 1
    puts "Brute Force Algorithm Done! "
}
