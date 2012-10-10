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

#source ./matrix.tcl
#source ./utilities.tcl
############################################################################
#     INITIAL SIMPLEX CONSTRUCTION METHODS                                 #
#  Methods:                                                                #
#   1 : use ANN to generate simplex points in a hypersphere (centered at   #
#       given init_point) of radius r.                                     #
#   2 : use scaling vector to generate simplex points around a given init  #
#       point [can generate only N+1 and 2N simplex, where N is the number #
#       of tunable parameters]                                             #
#   3 : use scaling vector to generate simplex points at the center of     #
#       the search space [--same as 2]                                     #
#   4 : random simplex : use a random uniform sampling on the search space #
#       to create an initial simplex                                       #
#   5 : read simplex from a file                                           #
#   6 : construct the simplex around a given initial point                 #
############################################################################

proc construct_initial_simplex { appName } {
    upvar #0 simplex_npoints size
    upvar #0 initial_simplex_method initial_simplex_method
    upvar #0 debug_mode debug_
    upvar #0 do_projection do_projection

    set temp {}
    set return_simplex {}

    #1 : Use Approximate Nearest Neighbor to generate the initial simplex
    #    around a given initial point.
    if { $initial_simplex_method == 1 } {
        upvar #0 icsm_${initial_simplex_method}_params(init_point) init_point
        upvar #0 icsm_${initial_simplex_method}_params(init_distance) init_distance
        upvar #0 icsm_1_params(use_exploration_point) use_e_point

        # using the best point from space exploration?
        if { $use_e_point == 1 } {
            upvar #0 space_explore_params(best_point) b_point
            set init_point $b_point
        }

        set temp [initial_simplex_with_ann $size $init_point $init_distance]

        if { [llength $temp] != $size } {
            while { [llength $temp] < $size } {
                # first increase the radius and request for more points
                incr init_distance
                set size_temp [expr $size - [llength $temp]]
                set extra_points [initial_simplex_with_ann $size_temp $init_point $init_distance]
                foreach point $extra_points {
                    lappend temp $point
                }
            }
        }
    }

    # 2: 2N simplex: with scaling. N is the number of tunable parameters
    if { $initial_simplex_method == 2 } {
        upvar #0 icsm_${initial_simplex_method}_params(scaling_vector) scaling_vector
        upvar #0 icsm_${initial_simplex_method}_params(init_point) init_point
        upvar #0 simplex_npoints  size
        upvar #0 icsm_${initial_simplex_method}_params(use_exploration_point) use_e_point

        # using the best point from space exploration?
        if { $use_e_point == 1 } {
            upvar #0 space_explore_params(best_point) b_point
            set init_point $b_point
        }
        set temp [initial_2N_simplex_with_scaling $init_point $scaling_vector $size]
    }

    # 3: same as 2, but start at the center of the search space
    if { $initial_simplex_method == 3 } {
        upvar #0 icsm_${initial_simplex_method}_params(scaling_vector) scaling_vector
        upvar #0 simplex_npoints  size
        upvar #0 icsm_${initial_simplex_method}_params(use_exploration_point) use_e_point

        set center [get_center_of_search_space $appName]
        set init_point $center
        set temp [initial_2N_simplex_with_scaling $center $scaling_vector $size]
    }

   # 4: Random simplex
    if { $initial_simplex_method == 4 } {
        upvar #0 icsm_${initial_simplex_method}_params(filename) random_simplex_file
        #puts $random_simplex_file
        upvar #0 icsm_${initial_simplex_method}_params(init_point) init_point
        #set temp [initial_simplex_from_file $random_simplex_file]
        set temp [construct_random_simplex_domain $appName]
    }


    #5 : Read the simplex from a file
    #  the file format should be the following:
    #  global init__
    #  set init__ [list [list [list param_1 param_2 ... ] [list param_1 param_2 ...]]]
    if { $initial_simplex_method == 5 } {
        upvar #0 icsm_${initial_simplex_method}_params(init_point) init_point
        upvar #0 icsm_${initial_simplex_method}_params(filename) init_simplex_filename
        set temp [initial_simplex_from_file $init_simplex_filename]
    }

    # 6: MOST USED
    #   Construct the initial simplex around a given initial point, without using
    #   the ANN
    if { $initial_simplex_method == 6 } {
        upvar #0 icsm_${initial_simplex_method}_params(init_point) init_point
        upvar #0 icsm_${initial_simplex_method}_params(init_distance) init_distance
        upvar #0 simplex_npoints size
        upvar #0 icsm_${initial_simplex_method}_params(use_exploration_point) use_e_point

        # using the best point from space exploration?
        if { $use_e_point == 1 } {
            upvar #0 space_explore_params(best_point) b_point
            set init_point $b_point
        }

        set temp [initial_simplex_no_ann $init_point $init_distance $size]
        if { [llength $temp] != $size } {
            while { [llength $temp] < $size } {
                # first increase the radius
                incr init_distance
                set size_temp [expr $size - [llength $temp]]
                set extra_points [initial_simplex_no_ann $init_point $init_distance $size_temp]
                foreach point $extra_points {
                    lappend temp $point
                }
            }
        }
    }

    #special case? N+1 simplex
    if { $initial_simplex_method == 7 } {
        #puts "inside initial simplex method 7"
        upvar #0 icsm_7_params(scaling_vector) scaling_vector
        upvar #0 icsm_7_params(init_point) init_point
        set temp [initial_N_plus_1_simplex_with_scaling $init_point $scaling_vector]
    }

    #puts $temp

    # do the projection
    if { $do_projection == 1} {
        set return_simplex [project_points $temp $appName]
    } else {
        set return_simplex $temp
    }

    if { $debug_ == 1 } {
        puts -nonewline "Creating an initial simplex with method "
        puts $initial_simplex_method
        puts -nonewline "Simplex:: with init_point:: "

        puts $init_point
        puts [format_initial_simplex $return_simplex]
    }

    return $return_simplex
}


proc get_center_of_search_space { appName } {
    upvar #0 ${appName}_bundles bundles
    set return_mat {}
    foreach bun $bundles {
        upvar #0 ${appName}_bundle_${bun}(domain) domain
        set domain_size [llength $domain]
        set mid_point [expr $domain_size/2]
        puts "$domain_size : $mid_point"
        #set rounded [round mid_point]
        lappend return_mat [lindex $domain $mid_point]
    }
    return $return_mat
}

# construct a random simplex
proc construct_random_simplex { appName } {
    upvar #0 simplex_npoints size
    upvar #0 ${appName}_bundles bundles
    upvar #0 ${appName}_procs procs

    set temp {}
    set points {}
    for {set i 0} {$i < $size} {incr i} {
        foreach bun $bundles {
            upvar #0 ${appName}_bundle_${bun}(min) min
            upvar #0 ${appName}_bundle_${bun}(max) max
            lappend temp [random_uniform $min $max 1]
        }
        lappend points $temp
        set temp {}
    }

    return $points
}

# construct a random simplex - domain dependent
proc construct_random_simplex_domain { appName } {
    upvar #0 simplex_npoints size
    upvar #0 ${appName}_bundles bundles
    upvar #0 ${appName}_procs procs

    set temp {}
    set points {}
    for {set i 0} {$i < $size} {incr i} {
        foreach bun $bundles {
            upvar #0 ${appName}_bundle_${bun}(min) min
            upvar #0 ${appName}_bundle_${bun}(max) max
            upvar #0 ${appName}_bundle_${bun}(domain) domain

            set idx_upper [expr [llength $domain]-1]
            set idx  0
            if { $idx_upper > 0 } {
                set idx  [random_uniform 0 $idx_upper 1]
            }
            lappend temp [lindex $domain $idx]
        }
        lappend points $temp
        set temp {}
    }

    return $points
}

# Method 1: using ANN
proc initial_simplex_with_ann { size init_point distance } {

    # do a nearest neighbor calculation and extract all
    #   neighbors within a given distance
    # If number of neighbors returned by ANN is greater than
    #   the size of the simplex, use uniform random numbers to
    #   select the required number of points

    # parameters:
    #  size: size of the simplex
    #  init_point: center of concentric hyper-spehers
    #  distance is a list of radius for each independent dimension

    upvar #0 ann_params(num_groups) num_groups
    upvar #0 ann_params(group_info) group_info
    upvar #0 ann_params(use_diff_radius) diff_radius
    set c_method 2

    # construct the distance string
    set dist_str ""
    set index 0
    if {$diff_radius == 1} {
        foreach dist $distance {
            if { $index == [expr [llength $distance]-1] } {
               # no need for trailing ':', but need a trailing ' '
              append dist_str $dist " "
            } else {
                append dist_str $dist ":"
            }
            incr index 1
        }
    } else {
        append dist_str [car $distance] " "
    }

    set init_simplex {}

    # make request string
    set request_str ""
    append request_str $dist_str [list_to_string $init_point]

    puts -nonewline "request string from initial simplex constructor :: "
    puts $request_str

    upvar #0 pro_step curr_step
  
    # special case for the new directions
    if {$curr_step == 10} {
        simplex_construction $request_str "constructed_simplex.tcl"
    }
    upvar #0 ann_params(init_simplex_name) init_simplex_file
     simplex_construction $request_str $init_simplex_file
    source ./constructed_simplex.tcl
   
    set init_simplex_temp [combine_groups_method_${c_method} $init__ $init_point $group_info]

    set index [expr [llength $init_simplex_temp] -1]

    set init_simplex {}

    # corner case : what if we do not have enough neighbors?
    set neighbors $init_simplex_temp
    if { [llength $neighbors] > $size } {
        set init_simplex [select_k_random $init_simplex_temp $size]
    } else {
        set init_simplex $neighbors
    }
    return $init_simplex
}


proc select_k_random { ls k } {
    set return_list [list]
    set random_numbers [random_uniform 100 999 [llength $ls]]
    set tacked [tack_on $random_numbers $ls]
    set sorted [lsort -index 0 $tacked]
    for {set j 0} {$j < $k} {incr j} {
        lappend return_list [car [cdr [lindex $sorted $j]]]
    }

    return $return_list
}


# scaling simplex -- 2N size -- This does not use ANN
proc initial_2N_simplex_with_scaling { init_point scaling_vector size } {

    puts $init_point
    puts $scaling_vector
    set negated [scale_vect -1 $scaling_vector]
    puts $negated
    set upper_N [make_diagonal $scaling_vector]
    set lower_N [make_diagonal $negated]

    set joined_temp [list $upper_N $lower_N]

    set joined [join $joined_temp]
    set replicated [replicate_rows $size $init_point]
    puts $replicated
    puts $joined
    set init_simplex [add_mat $replicated $joined]

    return $init_simplex
}

proc initial_N_plus_1_simplex_with_scaling { init_point scaling_vector } {
    upvar #0 simplex_npoints size
    set size_2n [expr [llength $init_point] * 2]
    set 2N_simplex [initial_2N_simplex_with_scaling $init_point $scaling_vector $size_2n]
    return [select_k_random $2N_simplex $size]
}

# method 5: read simplex from a file: Assumption is that the file is constains
#  a definition for a list of lists named as init__

proc initial_simplex_from_file { filename } {
    source ./$filename
    return $init__
}

# given a list of integers, returns a list of all possible permutations
#  can use this if the the search dimension is less than 7. Slow for
#  higher number of dimensions.
proc permutations {list} {
    set res [list [lrange $list 0 0]]
    set posL {0 1}
    foreach item [lreplace $list 0 0] {
       set nres {}
       foreach pos $posL {
	  foreach perm $res {
	     lappend nres [linsert $perm $pos $item]
	  }
       }
       set res $nres
       lappend posL [llength $posL]
    }
    return $res
}

# the problem of finding points *at* a given L1 distance can be reduced to the
#  the problem of dividing n balls into k bins.
#  Essentially, this amounts to calculating how many ways the "change" (in terms
#   of values) from the given init-point can be distributed.
proc balls_and_bins { balls bins } {
    set return_list [list]
    set remaining $balls
    set num_start 0
    while { $num_start != [expr $balls + 1] } {
        set temp [list]
        set current_bin 1
        lappend temp $num_start
        set remaining [expr $remaining - $num_start]
        # if there are more balls left
        for {set i $current_bin} { $i < [expr $bins -1] } {incr i} {
            if { $remaining != 0 } {
                lappend temp 1
                set remaining [expr $remaining -1]
            } else {
	      lappend temp 0
	    }
        }
        if { $remaining != 0 } {
            lappend temp $remaining
        } else {
	    lappend temp 0
	}
        lappend return_list $temp
        incr num_start
	set remaining $balls
    }
    return $return_list
}

# a utility method to construct a diagonal matrix (diagonal elements are
#  passed as a list
proc make_diagonal { diag_elems } {
    set return_matrix [list]
    set curr_index 0
    for {set j 0} {$j < [llength $diag_elems]} {incr j} {
        for {set i 0} {$i < [llength $diag_elems]} {incr i} {
            if {$curr_index == $i} {
                # set the respective diagonal element
                lappend temp [lindex $diag_elems $i]
            } else {
                lappend temp 0
            }
        }
        lappend return_matrix $temp
        set temp [list]
        incr curr_index
    }
    return $return_matrix
}

# another utility method to convert list to a string; each element in the
#  list will be separated by the given delimiter
proc list_to_string_with_delimiter { ls delim } {
    set return_str ""
    set index 0
    foreach elem $ls {
	if { $index != [expr [llength $ls] - 1] } {
	    append return_str $elem " " $delim " "
	} else {
	    append return_str $elem
	}
	incr index
    }
    return $return_str
}

# given a list of configurations, this removes duplicate occurrences.
proc discard_same_elems { ls } {
    set return_list [list]
    set str ""
    # join the elements in the list to a string with some delim
    foreach elem $ls {
	lappend str [list_to_string_with_delimiter $elem " : "]
    }
    # use tcl provided lsort with -unique flag to remove the duplicates
    set unique [lsort -unique $str]

    # change the string back to list - using split
    foreach elem $unique {
	set one_line [join [split $elem ":"]]
	set temp [list]
	foreach mem $one_line {
	    lappend temp [string trim $mem]
	}
	lappend return_list $temp
    }

    return $return_list
}

# given a list of configurations, this removes confs that give the same
#  permutations. Uses the same logic as discard same elems - but there are
#  subtle differences.
#  For example, [1 0 2], [2 1 0] and [1 2 0] all give the same permutations
proc remove_duplicate_perms { ls } {
    set return_list [list]
    set sorted_ls [list]
    foreach elem $ls {
	lappend sorted_ls [lsort -integer $elem]
    }
    set str ""
    foreach elem $sorted_ls {
	lappend str [list_to_string_with_delimiter $elem " : "]
    }
    set unique [lsort -unique $str]
    foreach elem $unique {
	set one_line [join [split $elem ":"]]
	set temp [list]
	foreach mem $one_line {
	    lappend temp [string trim $mem]
	}
	lappend return_list $temp
    }
    return $return_list
}


# get a random permutation of the elements in the given list
#  an unbiased version - each permutation is equally likely.
proc K { x y } { set x }

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

proc rand_shuffle__ { list } {
     set len [llength $list]
     set len2 $len
     for {set i 0} {$i < $len-1} {incr i} {
         set n [expr {int($i + $len2 * rand())}]
	 incr len2 -1

         # Swap elements at i & n
         set temp [lindex $list $i]
         lset list $i [lindex $list $n]
         lset list $n $temp
     }
     return $list
}

# wrapper - get n random permutations :: NOT USED RIGHT NOW :: Keeping it
#  around for error checking and debugging purposes

proc num_rand_shuffle { ls n } {
    set return_list [list]

    for {set i 0} {$i < $n} {incr i} {
	lappend return_list [rand_shuffle $ls]
    }

    # make sure all the elments are unique
    set unique_elems [discard_same_elems $return_list]

    # probably need some more error-checking here
    while { [llength $unique_elems] != $n } {
	lappend return_list [rand_shuffle $ls]
	set unique_elems [discard_same_elems $return_list]
    }

    return $unique_elems
}

# returns one scaling vector of given size

proc get_one_scale_vector {size} {
    set return_list {}
    for {set i 0} {$i < $size} {incr i} {
        # call rand() method to decide the sign
        set plus_or_minus [expr rand()]
        if { $plus_or_minus > 0.5 } {
            lappend return_list +1
        } else {
            lappend return_list -1
        }
    }
    return $return_list
}

proc initial_simplex_no_ann { init_point dist size } {
    # number of ways to generate points within the given L1 distance
    set num_ways_to_add [balls_and_bins $dist [llength $init_point]]

    # discard the configurations that give the same permutations : and shuffle
    set num_ways_unique_temp [remove_duplicate_perms $num_ways_to_add]
    set num_ways_unique [rand_shuffle $num_ways_unique_temp]
    unset num_ways_unique_temp
    unset num_ways_to_add
    # take each solution from the balls-and-bins problem and take a random
    #  permutation
    #  Corner cases: For smaller L1 distances there might not be enough
    #                solutions.

    # stop looking if we can't produce enough unique permutations after
    #  MAX_ITER iterations. can move this to pro_init
    set MAX_ITER 500
    set current_size 0
    set index 0
    set total_iterations 0
    set rand_permutations [list]
    set rand_permutations_temp [list]
    while { $total_iterations < $MAX_ITER && $current_size < $size } {
        set temp [rand_shuffle [lindex $num_ways_unique $index]]
        lappend rand_permutations_temp $temp
        # if at the end of the number of solutions, wrap around
        if { $index == [expr [llength $num_ways_unique] - 1] } {
            set index 0
        } else {
            incr index
        }
        # if we already generated, $size number of random permutations check for
        #  duplicates
        if { [llength $rand_permutations_temp] == $size } {
            set rand_permutations [discard_same_elems $rand_permutations_temp]
            set current_size [llength $rand_permutations]
        } else {
            set current_size [llength $rand_permutations_temp]
        }
        incr total_iterations
    }
    unset rand_permutations_temp
    unset num_ways_unique
    # generate scaling matrix :: the maximum number of scaling vectors that we
    #  will generate is bounded by a global variable.
    #  For cases where the number of possible scaling vectors is less than the
    #  upper bound, we consider all of them.

    # can move this to the global definitions file
    set MAX_SCALING_SIZE 60
    set temp_dim [llength $init_point]
    # how many unique scaling vectors are available to us?
    set max_avail [expr pow(2,$temp_dim)]
    set scaling_matrix [list]

    if { $MAX_SCALING_SIZE > $max_avail } {
        set scaling_matrix [scaling_matrix_${temp_dim}_d]
    } else {
        set current_size 0
        set index 0
        set scaling_temp [list]
        while { $current_size < $MAX_SCALING_SIZE } {
            set temp [get_one_scale_vector [llength $init_point]]
            lappend scaling_temp $temp
            set current_size [llength $scaling_temp]
        }
        # once the scaling matrix is generated, discard the same elements
        set scaling_matrix [discard_same_elems $scaling_temp]
    }

    set index 0
    set scaled [list]
    set temp_scaled [list]
    set temp_2 [list]
    foreach elem $rand_permutations {
        foreach scale_elem $scaling_matrix {
            for {set i 0} {$i < [llength $init_point]} {incr i} {
                lappend temp_2 [expr [lindex $elem $i] * [lindex $scale_elem $i]]
            }
            lappend temp_scaled $temp_2
            set temp_2 [list]
        }
        lappend scaled [discard_same_elems $temp_scaled]
        set temp_scaled [list]
    }

    unset rand_permutations
    unset scaling_matrix

    set scaled [join $scaled]
    set temp_size [llength $scaled]
    set replicated [replicate_rows $temp_size $init_point]
    set result [add_mat $replicated $scaled]
    unset replicated
    unset scaled



    set init_simplex {}
    if { [llength $result] > $size } {
        set random_shuffle [rand_shuffle $result]
        set init_simplex [lrange $random_shuffle 0 [expr $size -1]]
    } else {
        set init_simplex $result
    }

    return $init_simplex
}

proc initial_simplex_no_ann_deprecated { init_point dist size } {
    set num_ways_to_add [balls_and_bins $dist [llength $init_point]]
    set num_ways_unique [remove_duplicate_perms $num_ways_to_add]

    set all_possible_perms [list]
    foreach elem $num_ways_unique {
        set perm [permutations $elem]
        set uni [discard_same_elems $perm]
        lappend all_possible_ways $uni
    }

    set all_possible_ways [join $all_possible_ways]

    # generate scaling matrix
    set scaling_matrix [list]

    # hack alert
    set hack_dim [llength $init_point]
    set scaling_matrix [scaling_matrix_${hack_dim}_d]

    puts "done constructing the scaling matrix"

    set index 0
    set scaled [list]
    set temp_scaled [list]
    set temp_2 [list]
    foreach elem $all_possible_ways {
        foreach scale_elem $scaling_matrix {
            for {set i 0} {$i < [llength $init_point]} {incr i} {
                lappend temp_2 [expr [lindex $elem $i] * [lindex $scale_elem $i]]
            }
            lappend temp_scaled $temp_2
            set temp_2 [list]
        }
        lappend scaled [discard_same_elems $temp_scaled]
        set temp_scaled [list]
    }

    puts "done selecting points"
    set scaled [join $scaled]
    set temp_size [llength $scaled]
    set replicated [replicate_rows $temp_size $init_point]
    set result [add_mat $replicated $scaled]

    puts $result

    set init_simplex {}
    #####################################
    upvar #0 curr_new_dir_trial curr_trial
    upvar #0 pro_step pro_step
    set size_temp $size
    if { $pro_step == 10 } {
        if {$curr_trial == 2} {
            puts "now constructing the 2N simplex"
            upvar #0 icsm_2_params(scaling_vector) scaling_vector
            set  init_simplex [initial_2N_simplex_with_scaling  $init_point $scaling_vector [expr [llength $init_point] *2]]
            puts "done constructing the 2N simplex"
            puts $init_simplex
            set size_temp [expr $size - [expr [llength $init_point]*2]]
        }
    }

    #set size_temp [expr $size - [expr [llength $init_point]*2]]
    # change the size_temp to size if 2N simplex not needed
    set random_numbers [random_uniform 100 999 [llength $result]]
    if { [llength $result] > $size_temp } {
        set tacked [tack_on $random_numbers $result]
        set sorted [lsort -index 0 $tacked]
        for {set j 0} {$j < $size_temp} {incr j} {
            lappend init_simplex [car [cdr [lindex $sorted $j]]]
        }
    } else {
        set init_simplex $result
    }
    return $init_simplex
}

#############################################################################
#     NEW DIRECTIONS SIMPLEX CONSTRUCTION METHODS                           #
#  Methods:                                                                 #
#   1 : use ANN to generate simplex points in a hypersphere (centered at    #
#       current best point) of radius r.                                    #
#       (We start with provided initial radius - if the initial radius      #
#       does not provide required number of neighbors, we increment the     #
#       radius and extract more neighbors.                                  #
#   2 : use scaling vector to generate simplex points around the current    #
#       best point -- the starting radius is porvided as a global variable. #
#       The increment/decrement are also provided.                          #
#   3 : use scaling vector to generate simplex points around top k best     #
#       vertices  (not implemented yet)                                     #
#   4 : No ANN (analogous to initial simplex construction method 6)         #
#############################################################################

proc construct_new_directions { appName } {
    upvar #0 curr_new_dir_trial curr_trial
    upvar #0 max_new_dir_trial max_new_trials
    upvar #0 new_directions_method method
    upvar #0 simplex_low sim_low
    upvar #0 simplex_points curr_simplex
    set best_point [lindex $curr_simplex $sim_low(index)]
    upvar #0 simplex_npoints size
    upvar #0 do_projection do_projection
    upvar #0 debug_mode debug_

    # if we have exceeded the maximum number of trials, we return an empty
    #   simplex - a signal that the search is done
    set temp {}

    if { $curr_trial <= $max_new_trials } {

        if { $method == 1 } {
            upvar #0 ndm_${method}_params params
            upvar #0 ndm_${method}_params(curr_distance) curr_dist
            upvar #0 ndm_${method}_params(distance_step) dist_step
            set temp [initial_simplex_with_ann $size $best_point $curr_dist]

            if { [llength $temp] == $size } {
                set curr_dist [add_vect $dist_step $curr_dist]
            } else {
                while { [llength $temp] < $size } {
                    # first increase the radius
                    set curr_dist [add_vect $dist_step $curr_dist]
                    set size_temp [expr $size - [llength $temp]]
                    set extra_points [initial_simplex_with_ann $size_temp $best_point $curr_dist]
                    foreach point $extra_points {
                        lappend temp $point
                    }
                }
            }
        }

        # method 2: use scaling vector to create the simplex
        if { $method == 2 } {
            upvar #0 ndm_${method}_params params
            upvar #0 ndm_${method}_params(scaling_vector) scaling_vector
            upvar #0 ndm_${method}_params(step) scaling_step
            upvar #0 simplex_npoints size
            set temp [initial_2N_simplex_with_scaling $best_point $scaling_vector $size]
            set scaling_vector [scale_vect $scaling_step $scaling_vector]
        }

        # method 3: initial point is a member of best k-points of this pro run
        if { $method == 3 } {
        }
        if { $method == 4 } {
            upvar #0 ndm_${method}_params params
            upvar #0 ndm_${method}_params(curr_distance) curr_dist
            upvar #0 ndm_${method}_params(distance_step) dist_step
            set temp [initial_simplex_no_ann $best_point [car $curr_dist] $size]
            if { $debug_ == 1 } {
                puts $curr_dist
            }


            if { [llength $temp] == $size } {
                set curr_dist [add_vect $dist_step $curr_dist]
            } else {
                while { [llength $temp] < $size } {
                    # first increase the radius
                    set curr_dist [add_vect $dist_step $curr_dist]
                    set size_temp [expr $size - [llength $temp]]
                    set extra_points [initial_simplex_no_ann $best_point [car $curr_dist] $size_temp]
                    foreach point $extra_points {
                        lappend temp $point
                    }
                }
            }
        }
    } else {

        # take a best point from the last k-list
        #  start from second element in the list
        upvar #0 ndm_3_params(next_index) next_index
        upvar #0 last_k_best last_k_best
        set last_k_best [discard_same_elems $last_k_best]
       
        #only look at half of the last-k points, starting from the most recent best
        # LOWERING THE LAST k SEARCH to last 1
        if { $next_index < 0 } {
            upvar #0 simplex_npoints size
            set best_point [lindex $last_k_best $next_index]
            set curr_temp_distance {20}
            set temp [initial_simplex_no_ann $best_point $curr_temp_distance $size]

            puts -nonewline "current best k :: "
            puts $best_point

            while { [llength $temp] < $size } {
                set curr_dist_temp [add_vect {1} $curr_temp_distance]
                set size_temp [expr $size - [llength $temp]]
                set extra_points [initial_simplex_no_ann $best_point [car $curr_dist_temp] $size_temp]
                foreach point $extra_points {
                    lappend temp $point
                }
            }
            incr next_index
        }
    }
    incr curr_trial

    if { $do_projection == 1} {
        set return_simplex [project_points $temp $appName]
    } else {
        set return_simplex $temp
    }

    if { $debug_ == 1 } {
        puts -nonewline "Creating a new directions simplex with method "
        puts $method
        puts -nonewline "Simplex:: with init_point:: "
        puts $best_point
        puts [format_initial_simplex $return_simplex]
    }

    return $return_simplex
}

proc reset_new_dir_params {} {
    upvar #0 curr_new_dir_trial curr_trial
    set curr_trial 1

    upvar #0 ndm_1_params(curr_distance) curr_dist
    set curr_dist {1}

    upvar #0 ndm_4_params(curr_distance) curr_dist
    set curr_dist {1}

}

