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

# Written by: Ananta Tiwari, University of Maryland at College Park

# for N variables, we will maintain a KN simplex. For the rationale behind using
#  at least 2N simplex, refer to SC05 paper by Vahid Tabatabaee et. al.

##### Note we heavily rely on the variables defined in the pro_init_<appname>.tcl file.

# Overview of the Algorithm:
    #  Compute the reflection points in parallel
    #  If reflection is successful
    #   compute the expansion point for the point with the best reflection
    #   If the computed expansion point is better than the reflection point
    #       Accept expansion
    #   Else
    #       Accept Reflection
    #  Else
    #   Accept Shrink

    # The decision to reflect, expand or shrink is based on pro_step
    #  variable

    # pro_step : -2, construct a random simplex and record the best so far
    # pro_step : -1, calculate initial simplex
    # pro_step : 0, reflect
    # pro_step : 1, test if the reflection was successful
    #               if reflection was succesful, expand the best point.
    # pro_step : 2, test if the expansion in the preceding step was successful
    #               if expansion was successful, expand the rest of the
    #               points
    #               if not, accept reflection
    # pro_step : 3, reflection wasn't successful, shrink

    # At the end of each pro call, check for convergence.
    # If there is a convergence, do a specified number of local search iterations


proc pro { appName } {

    # Do we need dynamic code generation? If yes, make the connection
    #  to the code server here.
    global code_generation_params
    upvar #0 code_generation_params(enabled) cs_enabled
    upvar #0 code_generation_params(type) cs_type
    upvar #0 code_generation_params(host) cs_host
    upvar #0 code_generation_params(port) cs_port
    upvar #0 code_generation_params(connected) cs_connected

    if { $cs_enabled == 1 } {
        if { $cs_type == 1 } {
            if { $cs_connected == 0 } {
                set c_status [codeserver_startup $cs_host $cs_port]
                if { $c_status == 0 } {
                    puts -nonewline "Connection to the code server at "
                    puts -nonewline $cs_host
                    puts -nonewline " and port "
                    puts -nonewline $cs_port
                    puts "failed"
                } else {
                    set cs_connected 1
                    puts "Requesting code generation"
                }
            }
        }
    }

    # ANN tree construction variables
    #  Are we using ANN for projection?
    #  If yes, make the connection to the projection server here.
    upvar #0 ann_params(use_ann) u_ann
    upvar #0 ann_params(pserver_host) pserver_host
    upvar #0 ann_params(pserver_port) pserver_port
    upvar #0 ann_params(connected) connected

    # if we are using ANN, make a connection to the points server :: currently
    #  there are no fallbacks provided if the connection to the server fails
    if { $u_ann == 1 } {
        if { $connected == 0 } {
            set status [projection_startup $pserver_host $pserver_port]
            if { $status == 0 } {
                puts -nonewline "Connection to the point server at "
                puts -nonewline $pserver_host
                puts -nonewline " and port "
                puts -nonewline $pserver_port
                puts "failed"
            } else {
                set connected 1
            }
        }
    }

    #  Are we using the space exploration or constructing the simplex from a
    #  given point in the search space?
    #  If we are exploring the space then:
    upvar #0 space_explore_params(num_iterations) rand_iterations
    if { $rand_iterations > 0 } {
        explore_space $appName
    } else {
        # This is the search part.
        pro_algorithm $appName
    }
}

# space exploration
proc explore_space { appName } {

    upvar #0 space_explore_params(num_iterations) rand_iterations
    upvar #0 space_explore_params(random_step) r_step
    upvar #0 space_explore_params(best_perf) best_perf
    upvar #0 space_explore_params(best_point) best_point
    upvar #0 candidate_low can_low
    upvar #0 candidate_simplex c_points
    upvar #0 ${appName}_procs procs
    upvar #0 simplex_iteration iteration
    upvar #0 debug_mode debug_
    upvar #0 save_to_best_file save_best
    upvar #0 best_perf_so_far best_perf_so_far
    upvar #0 best_coordinate_so_far best_coord_so_far
    incr iteration
    if { $r_step > 0 } {
        set counter 0
        foreach proc $procs {
            upvar #0 ${proc}_performance perf
            puts -nonewline $perf
            puts -nonewline ", "
            if { $perf < $best_perf } {
                set best_perf $perf
                set best_point [lindex $c_points $counter]
            }
            incr counter
        }
        puts ""
        set best_perf_so_far $best_perf
        set best_coord_so_far $best_point
    } else {
        set save_best 1
        incr r_step
    }


    puts "Space Exploration segment?"

    upvar #0 save_to_best_file save_best
    if { $save_best == 1 } {
        set fname [open "best_points.log" {WRONLY APPEND CREAT} 0600]

        puts -nonewline $fname [list_to_string  $best_coord_so_far]
        puts $fname $best_perf_so_far

        if { $debug_ == 1 } {
            puts -nonewline "best coord so far :: "
            puts [list_to_string  $best_coord_so_far]
            puts -nonewline "best perf so far :: "
            puts  $best_perf_so_far
        }
        close $fname
    }
    upvar #0 do_projection do_projection
    set temp [construct_random_simplex_domain $appName]
    if { $do_projection == 1} {
        set c_points [project_points $temp $appName]
    } else {
        set c_points $temp
    }

    set points $c_points
    if { $debug_ == 1 } {
        puts [format_initial_simplex $points]
    }
    assign_values_to_bundles $appName $c_points
    incr rand_iterations -1
 
    global code_generation_params
    upvar #0 code_generation_params(enabled) cs_enabled
    upvar #0 code_generation_params(type) cs_type
    if { $cs_enabled == 1 } {
        if { $cs_type == 1 } {
            send_candidate_simplex $appName
        } else {
            write_candidate_simplex $appName
        }
    }
    #puts "exiting the explore space function"
    upvar #0 include_last_random_simplex include_last
    set include_last 1
    #  Call a post-processing proc to set all the label-texts
    set_label_texts $appName $iteration
     # signal the application
    signal_go $appName 0
}

proc pro_algorithm {appName} {

    # bring relevant variables to current scope
    upvar #0 simplex_low sim_low
    upvar #0 candidate_low can_low
    upvar #0 simplex_points points
    upvar #0 candidate_simplex c_points
    upvar #0 simplex_reflection_success ref_success
    upvar #0 simplex_save_perfs save_perfs
    upvar #0 simplex_exapansion_success exp_success
    upvar #0 simplex_converged converged
    upvar #0 simplex_new_directions_dist new_dir_dist
    upvar #0 ${appName}_procs procs
    upvar #0 debug_mode debug_
    upvar #0 tolerance tolerance
    upvar #0 simplex_npoints size
    upvar #0 space_dimension dim
    upvar #0 include_last_random_simplex include_last
    upvar #0 best_perf_so_far best_perf_so_far
    upvar #0 best_coordinate_so_far best_coord_so_far
    upvar #0 last_k_best last_k_best
    set temp_all_done 0
    set expand_best_local 0
    upvar #0 pro_step pro_step
    set check_convergence 1

    # increment the iteration variable
    upvar #0 simplex_iteration iteration

    set log ""
    logging [append log "PRO iteration: " $iteration] $appName 0
    incr iteration

    # if the step is not equal to -1, calculate the best
    #  point in the experimental simplex
    if { $pro_step > -1 } {
        # best_point with second argument as 1 returns the best_point
        #  in the candidate simplex
        best_point $procs
        logging "" $appName 1
    }

    if { $include_last == 1 } {
        best_point $procs
        upvar #0 last_best_cand_perf last_best_cand
        if { $last_best_cand < $best_perf_so_far } {
            upvar #0 cand_best_coord temp_coord
            upvar #0 space_explore_params(best_perf) best_perf
            upvar #0 space_explore_params(best_point) best_point
            set best_coord_so_far $temp_coord
            set best_perf_so_far $last_best_cand
            set best_point $temp_coord
            set best_perf $last_best_cand
        }
    }
    # if debugging mode
    if { $debug_ == 1 } {
        puts "---------------------------------------"
        if { $pro_step != -1 } {
            puts -nonewline " Received the following performance for "
            puts "candidate simplex configurations "
            foreach proc $procs {
                upvar #0 ${proc}_performance perf
                puts -nonewline $perf
                puts -nonewline ", "
            }
            puts ""
            puts -nonewline "Best performance:: "
            puts -nonewline $can_low(value)
            puts -nonewline " Associated Point:: "
            puts [lindex $c_points $can_low(index)]
        }
    }

    if { $pro_step == -1 } {
        set fname [open "best_points.log" {WRONLY APPEND CREAT} 0600]
        puts $fname "------------------"
        close $fname
        logging [append log "PRO step: " $pro_step ". Creating Initial Simplex: \n"] $appName 0
        set init_simplex_points [construct_initial_simplex $appName]
        set points $init_simplex_points
        set c_points $init_simplex_points
        incr pro_step
        set check_convergence 0
        assign_values_to_bundles $appName $c_points

    } elseif { $pro_step == 0 } {

        upvar #0 save_to_best_file save_best
        set save_best 1
        logging [append log "PRO step: " $pro_step ". Reflection\n"] $appName 0
        # ::: save the performance if we changed
        #     our real simplex in the last iteration
        if { $save_perfs == 1 } {
            if { $debug_ == 1 } {
                puts "  Saving the performance for the simplex "
                puts "  Reflection step: "
            }
            save_simplex_perfs $appName
            set save_perfs 0
        }

        set wrt [lindex $points $sim_low(index)]
        set c_points [transform_simplex 1 $wrt $points $appName]
        assign_values_to_bundles $appName $c_points
        incr pro_step

    } elseif { $pro_step == 1 } {

        append log "PRO step: " $pro_step ". "
        if { [test_success $sim_low(value)  $can_low(value)] == 1 } {
            set ref_success 1
            append log "Reflection Successful: Expand the best point. \n"

            if { $debug_ == 1 } {
                puts "Reflection Successful!"
            }

            # before expanding the best point, save the current
            #  candidate (reflected) simplex.
            upvar #0 last_reflected_simplex r_simplex
            upvar #0 last_reflected_perf r_perf

            set r_perf {}
            set r_simplex $c_points
            
            # save the best performance of the reflected simplex. We need this
            #  value saved for expansion check step.
            
            upvar #0 reflected_low_perf r_low_perf
            set r_low_perf $can_low(value)

            upvar #0 ${appName}_procs procs
            foreach proc $procs {
                upvar #0 ${proc}_performance perf
                lappend r_perf $perf
            }

            if { $debug_ == 1 } {
                puts "calling expand_one_point proc"
            }
            upvar #0 sorted_perfs sorted
            set wrt [lindex $points $sim_low(index)]
            set c_points [transform_simplex 2 $wrt $c_points $appName]
            if { $debug_ == 1 } {
                puts "simplex transformed"
            }
            assign_values_to_bundles $appName $c_points
            incr pro_step

        } else {
            if { $debug_ == 1 } {
                puts "Reflection Unsuccessful! Shrinking the simplex"
            }
            append log "Reflection Unsuccessful: Shrink \n"
            set ref_success 0
            set wrt [lindex $points $sim_low(index)]
            set c_points [transform_simplex 4 $wrt $points $appName]
            set points $c_points
            assign_values_to_bundles $appName $c_points
            set save_perfs 1
            set pro_step 0

        }
        logging $log $appName 0

    } elseif { $pro_step == 2 } {

        set log "PRO step "
        append log $pro_step ". \n"
        
        upvar #0 reflected_low_perf r_low_perf
        # remember we consider only the perf of the first configuration 
        # in the candidate simplex... other points in the candidate simplex
        # are just "filled-in" with the best in the last candidate simplex
        upvar #0 ${appName}_1_performance comp_cand
        if { [test_success $r_low_perf  $comp_cand] == 1 } {

            if { $debug_ == 1 } {
                puts "Expansion check step successful. Now on to Expansion"
            }
            append log "Expansion of the best reflection point was successful. "
            set exp_success 1
            append log "Expanding the entire simplex. \n"
            set wrt [lindex $points $sim_low(index)]
            set c_points [transform_simplex 3 $wrt $points $appName]
            set points $c_points
            assign_values_to_bundles $appName $c_points
            # Accept the candidate simplex and
            #  save the performance values in the next iteration.
            set save_perfs 1
            accept_candidate $appName
            set pro_step 0

        } else {

            if { $debug_ == 1 } {
                puts "Expansion Check step failed. Reverting back to the last reflected simplex"
            }
            append log "Expansion of the best reflection point was not successful. "
            set exp_success 0
            # expansion of the best point wasn't successful - if the reflection
            #  was successful, go ahead and accept the simplex and save
            #  the performance.
            append log "Reverting to last reflected simplex. \n"
            if { $ref_success == 1 } {
                set save_perfs 1
                accept_last_reflected $appName
                set wrt [lindex $points $sim_low(index)]
                set c_points [transform_simplex 1 $wrt $points $appName]
                assign_values_to_bundles $appName $c_points
                set pro_step 1
            } else {
                incr pro_step 1
            }
        }
        logging $log $appName 0

    } elseif { $pro_step == 10 } {

        # new directions search
        set log "PRO step:"
        append log $pro_step ". "
        if { [test_success $sim_low(value)  $can_low(value)] == 1 } {
            if { $debug_ == 1 } {
                puts "  New directions search successful... Accepting the candidate simplex."
            }
            # accept new directions and reset all new_dir params
            reset_new_dir_params
            append log "Accepting new directions and reflecting it. "
            set new_dir 0
            accept_candidate $appName
            save_simplex_perfs $appName
            set wrt [lindex $points $sim_low(index)]
            set c_points [transform_simplex 1 $wrt $points $appName]
            assign_values_to_bundles $appName $c_points
            set pro_step 1
            set converged 0
        } else {
            append log "New Search directions are no good. Converged..."
        }
        logging $log $appName 0

    } elseif { $pro_step == 11 } {
        puts "we are done"
        set temp_all_done 1
    }

    # check for convergence
    # if converged, search in new directions
    if { $check_convergence == 1 } {
        check_for_convergence $appName
    }

    if { ($converged == 1) && ($pro_step != 11) } {
        logging "Convergence: Looking for new directions." $appName 0

        if { $debug_ == 1 } {
            puts "Simplex converged to a point"
        }
        set temp [construct_new_directions $appName]
        if { [llength $temp] != 0 } {
            set c_points $temp
            set pro_step 10
            assign_values_to_bundles $appName $c_points
        } else {
            set the_best [lindex $points $sim_low(index)]
            set replicated [replicate_rows $size $the_best]
            set points $replicated
            set c_points $replicated
            assign_values_to_bundles $appName $points
            set pro_step 11
            #signal_all_done $appName
            set temp_all_done 1
        }
    }

    
    #   We are done with this pro iteration, now signal all
    #   the applications to get new performance values.
    if { $debug_ == 1 } {
        puts "signalling go"
    }
    signal_go $appName $expand_best_local
    
    if { $temp_all_done == 1 } {
        signal_all_done $appName $best_coord_so_far
    }

    #  Call a post-processing proc to set all the label-texts
    set_label_texts $appName $iteration

    
    if { $debug_ == 1 } {
        puts -nonewline "current pro step :: "
        puts $pro_step
    }
    upvar #0 save_to_best_file save_best
    if { $save_best == 1 } {
        set fname [open "best_points.log" {WRONLY APPEND CREAT} 0600]
        if { $debug_ == 1 } {
            puts "saving the best so far to all_points file"
            # compare with both sim_low and can_low
            puts -nonewline "can_low(value):: "
            puts $can_low(value)
            puts -nonewline "sim_low(value):: "
            puts $sim_low(value)
        }

        if { $best_perf_so_far >= $can_low(value) } {
            set best_perf_so_far $can_low(value)
            upvar #0 cand_best_coord temp_coord
            set best_coord_so_far $temp_coord
            lappend last_k_best $temp_coord
        }
        if { $best_perf_so_far >= $sim_low(value) } {
            set best_perf_so_far $sim_low(value)
            set best_coord_so_far [lindex $points $sim_low(index)]
            lappend last_k_best $best_coord_so_far
        }

        puts -nonewline $fname [list_to_string  $best_coord_so_far]
        puts $fname $best_perf_so_far

        if { $pro_step == 11 } {
            puts $fname "-->"
        }
        if { $debug_ == 1 } {
            puts -nonewline "best coord so far :: "
            puts [list_to_string  $best_coord_so_far]
            puts -nonewline "best perf so far :: "
            puts  $best_perf_so_far
        }
        close $fname
    }
    
    logging "------------------------" $appName 0

    global code_generation_params
    upvar #0 code_generation_params(enabled) cs_enabled
    upvar #0 code_generation_params(type) cs_type
    if { $cs_enabled == 1 } {
        if { $cs_type == 1 } {
            send_candidate_simplex $appName
        } else {
            write_candidate_simplex $appName
        }
    }
}

# ask the code-server if the code generation is complete
proc code_completion_query { appName } {
    set code_completion_respose [code_generation_complete]
    return $code_completion_respose
}

# iterate through all the performance numbers and find the best
proc best_point { procs } {

    upvar #0 pro_step_cost step_cost
    upvar #0 simplex_all_perfs all_perfs
    upvar #0 candidate_low c_low
    upvar #0 candidate_simplex points
    upvar #0 sorted_perfs sorted__
    global last_best_candidate
    upvar #0 last_best_cand_perf last_best_cand
    set current_perfs {}

    # gather individual performance numbers
    set index 0
    foreach proc $procs {
        upvar #0 ${proc}_performance perf
        puts -nonewline $perf
        puts -nonewline ", "
        lappend current_perfs [list $perf $index]
        incr index
    }
    puts ""
    # for record keeping -
    lappend all_perfs $current_perfs

    # sort wrt to the first element in the list
    #  for now, all the perf numbers are integers
    # sorted is a global variable
    set sorted__ [lsort -index 0 -real $current_perfs]
 
    # set the best
    set c_low(index) [car [cdr [lindex $sorted__ 0]]]
    set c_low(value) [car [lindex $sorted__ 0]]
    set last_best_cand  [car [lindex $sorted__ 0]]
    puts -nonewline "last best candidate "
    puts $last_best_cand
    upvar #0 cand_best_coord temp_coord
    set temp_coord [lindex $points [car [cdr [lindex $sorted__ 0]]]]

    lappend step_cost [car [cdr [lindex $sorted__ [expr [llength $procs] - 1]]]]

}

proc test_success { perf_1 perf_2 } {
    set return_value 0
    upvar #0 tolerance tolerance
    upvar #0 debug_mode debug_
    if { $perf_1 > $perf_2 } {
        if { [expr {abs($perf_1-$perf_2)}] > $tolerance } {
            set return_value 1
        } else {
            if { $debug_ == 1 } {
                puts -nonewline "  Candidate configuration is within the tolerance level of "
                puts $tolerance
            }
        }
    }
    return $return_value
}

proc check_for_convergence {appName} {
    # the simplex is said to have converged when all the points in the simplex
    #  take the same value
    # Other checks such as how far the performances reported are for each of the
    #  simplex points can also be used. for now, we use the former.
    upvar #0 simplex_points points
    # since we deal with 2N points, number of points considered is always even
    set first_point [lindex $points 0]
    set converged__ 1
    for {set i 1} {($i < [llength $points]) && ($converged__ != 0)} {incr i} {
        set second_point [lindex $points $i]
        for {set j 0} {$j < [llength $first_point]} {incr j} {
            if {[lindex $first_point $j] != [lindex $second_point $j]} {
                set converged__ 0
            }
        }
        set first_point $second_point
    }
    # if converged, get new directions to search
    upvar #0 simplex_converged simplex_converged
    if {$converged__ == 1} {
        set simplex_converged 1
    } else {
        set simplex_converged 0
    }
}


proc accept_last_reflected {appName} {
    upvar #0 simplex_points s_points
    upvar #0 simplex_performance s_perf
    upvar #0 last_reflected_simplex r_points
    upvar #0 last_reflected_perf r_perf
    set s_points $r_points
    set s_perf $r_perf

    upvar #0 simplex_low low

    set low_index 0
    set low_value 0
    set first_pass 1
    set counter 0

    # calculate the new low index and value
    foreach perf $s_perf {
        if {$first_pass == 1} {
            set low_value $perf
            set first_pass 0
        } else {
            if {$low_value > $perf} {
                set low_value $perf
                set low_index $counter
            }
        }
        incr counter
    }

    set low(index) $low_index
    set low(value) $low_value
}

# this is a simple routine which basically sets the real
#  simplex to the values of candidate simplex
proc accept_candidate { appName } {
    upvar #0 simplex_points s_points
    upvar #0 candidate_simplex c_points
    set s_points $c_points
}

# retrieve and store performances of the simplex points.
proc save_simplex_perfs { appName } {
    global ${appName}_procs
    global #0 simplex_low
    upvar #0 ${appName}_procs procs
    upvar #0 simplex_npoints npoints
    upvar #0 simplex_low low
    upvar #0 simplex_points points

    upvar #0 simplex_performance simplex_perf

    set low_index 0
    set low_value 0
    set first_pass 1
    set counter 0
    set simplex_perf {}

    foreach proc $procs {
        upvar #0 ${proc}_performance perf
        lappend simplex_perf $perf
        if {$first_pass == 1} {
            set low_value $perf
            set first_pass 0
        } else {
            if {$low_value > $perf} {
                set low_value $perf
                set low_index $counter
            }
        }
        incr counter
    }
    set low(index) $low_index
    set low(value) $low_value
}

# update the values of the variables
proc assign_values_to_bundles { appName points } {
    upvar #0 ${appName}_bundles bundles
    upvar #0 ${appName}_procs procs
    #puts "Assignment::: "
    foreach proc $procs point $points {
        set projected_value $point
        foreach bun $bundles dim $projected_value {
            #puts -nonewline "current assignment:: "
            #puts -nonewline $bun
            #puts -nonewline "-->"
            #puts  $dim
            upvar #0 ${proc}_bundle_${bun} current_proc_param
            set current_proc_param(value) $dim
        }
    }
}


proc get_best_configuration { appName } {
    upvar #0 best_coordinate_so_far best_coord_so_far
    return $best_coord_so_far
}
