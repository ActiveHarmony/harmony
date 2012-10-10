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

#
## the next two procs were moved here for debugging purposes. delete/comment
## them for final run
#
proc K { x y } { set x }
#
proc rand_shuffle { list } {
    set n [llength $list]
    while {$n>0} {
        set j [expr {int(rand()*$n)}]
        lappend slist [lindex $list $j]
        incr n -1
        set temp [lindex $list $n]
        set list [lreplace [K $list [set list {}]] $j $j $temp]
    }
    return $slist
}

# keeps the distance between the generated points and init_point constant
proc combine_groups_method_1 { inits__ init_point group_info } {
    set simplex {}
    set current 0
    set spl_list [split_list $init_point $group_info]
    foreach init__ $inits__ {
        foreach elem $init__ {
            set temp {}
            for {set i 0} {$i < [llength $group_info]} {incr i} {
                if { $i == $current } {
                    lappend temp $elem
                } else {
                    lappend temp [lindex $spl_list $i]
                }
            }
            lappend simplex [join $temp]
        }
        incr current
    }
    return $simplex
}

# maintains a constant distance in per-group granularity -> the distance between
#  points in their entirety might be different. However, this method still
#  guarantees the search directions to be linearly independent.
proc combine_groups_method_2 { inits__ init_point group_info } {
    
    # indices list
    set indices [list]
    set simplex [list]
    set shuffled_inits__ [list]
    set size 0
    
    puts -nonewline "group_info from combine.tcl :: "
    puts $group_info

    # error checks: check to make sure that groups are non-empty:
    #set non_empty_inits__ [list]
    #set spl_list [split_list $init_point $group_info]
    #for {set i 0} { $i < [llength $group_info]} {incr i} {
    #    set curr_group [lindex $inits__ $i]
    #    if {[llength $curr_group] == 0 } {
    #        lappend non_empty_inits__ [lindex $spl_list $i]
    #   } else {
    #        lappend non_empty_inits__ $curr_group
    #    }
    #}
    #unset inits__
    
    # first shuffle the incoming confs
    ####foreach init__ $inits__ {
   ####     lappend shuffled_inits__ [rand_shuffle $init__]
   #### }
    ####unset inits__
    set shuffled_inits__ $inits__
    unset inits__
    
    # use the group_info variable to construct the indices list and to determine
    #  the number of points generated - which is bounded by the number of
    #  elements in the largest group
    for {set i 0} { $i < [llength $group_info]} {incr i} {
        lappend indices 0
        if { $size < [llength [lindex $shuffled_inits__ $i]]} {
            set size [llength [lindex $shuffled_inits__ $i]]
        }
    }
    
    # logic: take one element from each group to generate one candidate point
    #  in the search space. Note that the shuffled_inits__ has the following
    #  structure : { {list of group_1 elements} {list of group_2 elements} ... }
    
    for {set i 0} { $i < $size } { incr i } {
        
        set temp [list]
        for {set j 0} { $j < [llength $group_info] } {incr j} {
            set curr_group [lindex $shuffled_inits__ $j]
            set curr_index [lindex $indices $j]
            lappend temp [lindex $curr_group $curr_index]
        }
        set temp [join $temp]
        lappend simplex $temp
        
        # wrap-around if at the end
        set temp_indices [list]
        for {set j 0} { $j < [llength $group_info] } {incr j} {
            set curr_group_max [llength [lindex $shuffled_inits__ $j]]
            set curr_index [lindex $indices $j]
            if { $curr_index == [expr $curr_group_max -1] } {
                lappend temp_indices 0
            } else {
                lappend temp_indices [incr curr_index]
            }
        }
        set indices $temp_indices
    }
    
    return $simplex
}

# splits a given list into list-of-lists as per the "mapping" given in
# group_info variable
proc split_list { orig group_info } {
    set return_list {}
    set parts_list {}
    set index 0
    foreach group $group_info {
        puts $group
        for {set i 0} {$i < $group} {incr i} {
            lappend parts_list [lindex $orig $index]
            incr index 1
        }
        lappend return_list $parts_list
        set parts_list {}
    }
    return $return_list
}


#set sp_ls [split_list {100 10 20 1 4 32 64} {3 2 2}]
#puts $sp_ls
