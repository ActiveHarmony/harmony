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

# procs to generate candidate simplices [reflection, expansion, shrink] using
#   preliminary matrix operations provided in matrix.tcl

source "matrix.tcl"
load ./nearest_neighbor.so
load ./round.so

proc reflect { wrt coeff points } {
    set ref_matrix {}
    foreach point $points {
        set point_negated [scale_vect -1 $point]
        set axpy__ [axpy_vect 2 $wrt $point_negated]
        lappend ref_matrix $axpy__
    }
    return $ref_matrix
}

proc expand { wrt coeff points } {
    set exp_matrix {}
    foreach point $points {
        set point_negated [scale_vect -2 $point]
        set axpy__ [axpy_vect 3 $wrt $point_negated]
        lappend exp_matrix $axpy__
    }
    return $exp_matrix
}

proc shrink { wrt coeff points } {
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

proc round_towards_min { wrt point } {
    # here the assumption is that the step-size is one
    #    i.e. the co-ordinate system is normalized
    # Change this should the step size change
    
    set step 1
    set temp_ind {}
    for {set j 0} {$j < [llength $wrt]} {incr j} {
        if {[expr (abs ( [lindex {$wrt} $j] - [lindex $point  $j]))] < $step} {
            lappend temp_ind [lindex $wrt  $j]
        } else {
            lappend temp_ind [lindex $point $j]
        }
    }
    return $temp_ind
}

# apply fun to all the elements in the list
proc map {fun ls} {
    set res {}
    foreach element $ls {lappend res [$fun $element]}
    return $res
}

proc tack_on { ls_1  ls_2 } {
    set index 0
    set out_list {}
    foreach elem $ls_1 {
        lappend out_list [list $elem [lindex ls_2 $index]]
        incr index
    }
    return $out_list
}

proc list_to_string { ls } {
    set return_string ""
    foreach elem $ls {
        append return_string $elem " "
    }
    return $return_string
}

puts [list_to_string {1 2 3}]

proc projection { tree point } {    
    set return_point $point
    # preliminaries : convert list to string
    #                 make a string object
    set temp_point [list_to_string $point]
    set temp_obj [make_string $temp_point]
    set query_point [make_point $temp_obj]
    # check if the point is valid
    set validity [is_valid $tree $query_point]
    if { $validity != 1} {
        set return_point [expr [get_neighbor $tree $query_point]]
    }
    return $return_point
}

proc print_matrix { points repeat_first tag } {
    foreach point $points {
        puts -nonewline $tag
        puts $point
    }
    puts -nonewline $tag
    if { $repeat_first == 1 } {
        puts [lindex $points 0]
    }
}

#set filename [make_string "test_2_vars"]
#set tree [construct_tree 2 1 $filename]

#puts -nonewline "Projection for 51 63:: "
#puts [projection $tree {51 63}]

# 4N simplex
# (+-xd,0), (0,+-yd), (+-2,2), (2,+-2)
# 2d
#set transform_matrix {{-7 0} {0 7} {7 0} {0 -7}}
set transform_matrix {{-5 0} {-3 3} {0 5} {3 3} {5 0} {3 -3} {0 -5} {-3 -3}}
#3d
#set transform_matrix {{3 0 0} {0 3 0} {0 0 3} {-3 0 0} {0 -3 0} {0 0 -3} {1 1 1} {1 1 -1} {1 -1 1} {1 -1 -1} {-1 1 1} {-1 1 -1} {-1 -1 1} {-1 -1 -1}}
set init_point {30 35}
set replicated [replicate_rows 8 $init_point]
set points [add_mat $replicated $transform_matrix]

##print_matrix $points 1 "init: "
#
#set init_matrix $points
#
## calculate reflection
#puts "----"
#set wrt {30 40}
#set ref_1 [reflect $wrt 2 $init_matrix]
#print_matrix $ref_1 1 "ref_1: "
#
##
#puts "----"
#set wrt {35 49}
#set ex_1 [expand $wrt 3 $ref_1]
#print_matrix $ex_1 1 "ex_1: "
#
#puts "----"
#set wrt {51 39}
#set ref_2 [reflect $wrt 2 $ex_1]
#print_matrix $ref_2 1 "ref_2: "
#
#
#puts "----"
#set wrt {51 39}
#set shr_1 [shrink $wrt 2 $ex_1]
#print_matrix $shr_1 1 "shr_1: "
#
#puts "----"
#set wrt {51 39}
#set shr_2 [shrink $wrt 2 $shr_1]
#print_matrix $shr_2 1 "shr_2: "
#
#puts "----"
#set wrt {51 39}
#set shr_3 [shrink $wrt 2 $shr_2]
#print_matrix $shr_3 1 "shr_3: "
#
#puts "----"
#set wrt {51 39}
#set shr_4 [shrink $wrt 2 $shr_3]
#print_matrix $shr_4 1 "shr_4: "
#puts "----"
#
#set wrt {51 39}
#set shr_5 [shrink $wrt 2 $shr_4]
#print_matrix $shr_5 1 "shr_5: "
#puts "----"
