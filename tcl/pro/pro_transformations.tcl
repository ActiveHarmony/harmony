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

##################################################################
# PRO TRANSFORMATION
#  type: 1 Reflection
#        2 Expand one point
#        3 Expansion
#        4 Shrink

proc transform_simplex { type wrt points appName {debug 1} {how_many 30} } {
    set before_projection {}
    set transformed {}
    upvar #0 do_projection project
    upvar #0 debug_mode debug_
    if { $type == 1 } {
        set before_projection [reflect $wrt $points]
        if { $debug_ == 1 } {
            puts -nonewline "Reflection wrt :: "
            puts $wrt
            puts "Candidate Simplex is:: "
        }
    }

    if { $type == 2 } {
        upvar #0 sorted_perfs sorted
        #set how_many [expr [llength $points] / 2]
        set how_many 1
        #puts $how_many
        set before_projection [expand_one_point $wrt $points $sorted $how_many]
        #set before_projection [expand_one_point $wrt $points $sorted 1]
        if { $debug_ == 1 } {
            puts -nonewline "Expand the best "
            puts -nonewline $how_many
            puts -nonewline " wrt:: "
            puts $wrt
        }
    }

    if { $type == 3 } {
        set before_projection [expand $wrt $points]
        if { $debug_ == 1 } {
            puts -nonewline "Expanding the simplex wrt "
            puts $wrt
            puts "Candidate Simplex is:: "
        }
    }

    if { $type == 4 } {
        set before_projection [shrink $wrt $points]
        if { $debug_ == 1 } {
           puts -nonewline "Shrinking the simplex wrt "
           puts $wrt
           puts "Candidate Simplex is:: "
        }
    }

    if { $project == 1 } {
        set transformed [project_points $before_projection $appName]
    }

    if { $debug_ == 1 } {
        puts [format_initial_simplex $transformed]
    }

    return $transformed
}


proc reflect { wrt points } {
    set ref_matrix {}
    foreach point $points {
        set point_negated [scale_vect -1 $point]
        set axpy__ [axpy_vect 2 $wrt $point_negated]
        lappend ref_matrix $axpy__
    }

    return $ref_matrix
}

proc expand { wrt points } {
    set exp_matrix {}
    foreach point $points {
        set point_negated [scale_vect -2 $point]
        set axpy__ [axpy_vect 3 $wrt $point_negated]
        lappend exp_matrix $axpy__
    }

    return $exp_matrix
}

proc shrink { wrt points } {
    set shr_matrix {}
    foreach point $points {
        set points_added [add_vect $point $wrt]
        #set raw_points [map round [scale_vect 0.5 $points_added]]
        set raw [round_towards_min $wrt [scale_vect 0.5 $points_added]]
        set raw_points [map round $raw]
        lappend shr_matrix $raw_points
    }

    return $shr_matrix
}

proc expand_one_point { wrt points sorted how_many } {
    # sorted is in the form [perf_1 index_1] ... [perf_n index_n]
    set return_matrix {}

    upvar #0 debug_mode debug
    upvar #0 cand_best_coord last_best_cand
    #puts -nonewline "last best:: "
    #parr $last_best_cand
    
    set possible_directions {}
    set best_direction {}
    for { set i 0 } { $i < $how_many } { incr i } {
        set temp [lindex $sorted $i]
        set index [car [cdr $temp]]
        lappend possible_directions [lindex $points $index]
        #parr $possible_directions

    }

    foreach point $possible_directions {
        set point_negated [scale_vect -2 $point]
        #parr $point_negated
        set axpy__ [axpy_vect 3 $wrt $point_negated]
        #parr $axpy__
        lappend return_matrix $axpy__
    }

    # now fill in the (size-how_many) spots in the simplex : use the most
    #  "promising" direction OR USE THE BEST POINT IN THE SORTED LIST

    set fill_in [lindex $possible_directions 0]
    #set fill_in $wrt
    #upvar #0 best_coordinate_so_far fill_in
    for { set j [expr $how_many] } { $j < [llength $points] } { incr j } {
        #lappend return_matrix $fill_in
        lappend return_matrix $last_best_cand
    }

    if { $debug == 1 } {
        puts -nonewline "Most promising candidate for expansion check in the simplex is :: "
        puts $fill_in
    }

    return $return_matrix
}
