# only assign_new_values to bundles will make a call to projection
# keep sorted list of the last 25 best points along with thier performances
# start new direction search from the best point in the list and move downwards

# for N variables, we will maintain a KN simplex. For the rationale behind using
#  at least 2N simplex, refer to SC05 paper by Vahid Tabatabaee et. al.

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


proc pro { appName } {
    
    # space exploration part
    upvar #0 space_explore_params(num_iterations) rand_iterations
    if { $rand_iterations > 0 } {
        explore_space $appName
    } else {
        # the search part
        pro_algorithm $appName
    }
}

proc explore_space { appName } {
    
    upvar #0 space_explore_params(num_iterations) rand_iterations
    upvar #0 space_explore_params(random_step) r_step
    upvar #0 space_explore_params(best_perf) best_perf
    upvar #0 space_explore_params(best_point) best_point
    upvar #0 candidate_low can_low
    upvar #0 candidate_simplex c_points
    upvar #0 ${appName}_procs procs
    upvar #0 simplex_iteration iteration
     set suffix ""
    if [info exists env(HARMONY_S_PORT)] {
        set suffix $env(HARMONY_S_PORT)
    } else {
        set suffix "1977"
    }
    incr iteration
    if { $r_step > 0 } {
        set counter 0
        foreach proc $procs {
            upvar #0 ${proc}_performance perf
            if { $perf < $best_perf } {
                set best_perf $perf
                set best_point [lindex $c_points $counter]
            }
            incr counter
        }

        upvar #0 best_perf_so_far best_perf_so_far
        upvar #0 best_coordinate_so_far best_coord_so_far
        set best_perf_so_far $best_perf
        set best_coord_so_far $best_point 
    } else {
        incr r_step
    }

    set c_points [construct_random_simplex $appName]
    set points $c_points
    puts [format_initial_simplex $points]
    assign_values_to_bundles $appName $c_points
    incr rand_iterations -1

    ##############################################
    write_candidate_simplex $appName
    set fname__ [open "/hivehomes/tiwari/pmlb/fusion_ex/best_file.$iteration.dat" "w"]
    #set fname__ [open "/hivehomes/tiwari/pmlb/sensitivity/best_file.$iteration.dat" "w"]
    set out_str ""
    #if { $r_step == 0 } {
        append out_str "streaming_all_default"
    #} else {
    #    set out_str "code/streaming_all"
    #    foreach cooord $best_point {
    #        append out_str "_" [expr int($cooord)]
    #    }
    #}
    append out_str ".so"
    puts $fname__ $out_str
    close $fname__
    ###################################
   
    set fname [open "best_points.log_${suffix}" {WRONLY APPEND CREAT} 0600]
    puts -nonewline $fname [list_to_string  $best_coord_so_far]
    puts $fname $best_perf_so_far
    close $fname 
    signal_go $appName 0
    puts "exiting the explore space function"
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

    # ANN tree construction variables
    upvar #0 ann_params(use_ann) u_ann
    upvar #0 ann_params(pserver_host) pserver_host
    upvar #0 ann_params(pserver_port) pserver_port
    upvar #0 ann_params(group_info) group_info
    upvar #0 ann_params(connected) connected

    set temp_all_done 0

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

    set suffix ""
    if [info exists env(HARMONY_S_PORT)] {
        set suffix $env(HARMONY_S_PORT)
    } else {
        set suffix "1977"
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
        set fname [open "best_points.log_${suffix}" {WRONLY APPEND CREAT} 0600]
        puts $fname "------------------"
        close $fname
        #set fname [open "all_points.log" {WRONLY APPEND CREAT} 0600]
        #puts $fname "------------------"
        #close $fname
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

            puts "calling expand_one_point proc"
            upvar #0 sorted_perfs sorted
            set wrt [lindex $points $sim_low(index)]
            #puts "got wrt point"
            set c_points [transform_simplex 2 $wrt $points $appName]
            puts "simplex transformed"
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
        #signal_all_done $appName
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

    upvar #0 best_perf_so_far best_perf_so_far
    upvar #0 best_coordinate_so_far best_coord_so_far
    upvar #0 last_k_best last_k_best
    
    #   We are done with this pro iteration, now signal all
    #   the applications to get new performance values.
    puts "signalling go"
    signal_go $appName $expand_best_local
    
    if { $temp_all_done == 1 } {
        signal_all_done $appName $best_coord_so_far
    }

    #  Call a post-processing proc to set all the label-texts
    set_label_texts $appName $iteration

    set fname [open "best_points.log_${suffix}" {WRONLY APPEND CREAT} 0600]

    puts -nonewline "current pro step :: "
    puts $pro_step
    upvar #0 save_to_best_file save_best
    if { $save_best == 1 } {

        puts "saving the best so far to all_points file"
        # compare with both sim_low and can_low
        puts -nonewline "can_low(value):: "
        puts $can_low(value)
        puts -nonewline "sim_low(value):: "
        puts $sim_low(value)
        puts -nonewline "best coord so far :: "
        puts [list_to_string  $best_coord_so_far]
        puts -nonewline "best perf so far :: "
        puts  $best_perf_so_far

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
        puts -nonewline "best coord so far :: "
        puts [list_to_string  $best_coord_so_far]
        puts -nonewline "best perf so far :: "
        puts  $best_perf_so_far

    }
    close $fname
    #set fname [open "all_points.log" {WRONLY APPEND CREAT} 0600]
    #puts $fname "---"
    #puts $fname [format_initial_simplex $c_points]
    #close $fname

    logging "------------------------" $appName 0

    write_candidate_simplex $appName
    #  ************* HACK ALERT. save the best_conf.
    set fname__ [open "/hivehomes/tiwari/pmlb/fusion_ex/best_file.$iteration.dat" "w"]
    #set fname__ [open "/hivehomes/tiwari/pmlb/sensitivity/best_file.$iteration.dat" "w"]
    set out_str "code/streaming_all"
    set wrt_temp [lindex $points $sim_low(index)]
    foreach cooord $best_coord_so_far {
        append out_str "_" [expr int($cooord)]
    }
    append out_str ".so"
    puts $fname__ $out_str
    close $fname__
    #  ************* HACK ALERT. save the best_conf.

}

proc best_point { procs } {

    upvar #0 pro_step_cost step_cost
    upvar #0 simplex_all_perfs all_perfs
    upvar #0 candidate_low c_low
    upvar #0 candidate_simplex points
    upvar #0 sorted_perfs sorted__
    set current_perfs {}

    # gather individual performance numbers
    set index 0
    foreach proc $procs {
        upvar #0 ${proc}_performance perf
        lappend current_perfs [list $perf $index]
        incr index
    }

    # for record keeping -
    lappend all_perfs $current_perfs

    # sort wrt to the first element in the list
    #  for now, all the perf numbers are integers
    # sorted is a global variable
    set sorted__ [lsort -index 0 -real $current_perfs]

    # set the best
    set c_low(index) [car [cdr [lindex $sorted__ 0]]]
    set c_low(value) [car [lindex $sorted__ 0]]
    upvar #0 cand_best_coord temp_coord
    set temp_coord [lindex $points [car [cdr [lindex $sorted__ 0]]]]

    puts -nonewline "temp_coord from best_point proc:: "
    puts $temp_coord

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

proc test_success_e_check { perf_1 perf_2 } {
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
    #  take same value
    # Other checks such as how far the performances reported are for each of the
    #  simplex points can also be used.
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
        #new_search_directions $appName $first_point
    } else {
        set simplex_converged 0
    }
}


# to do: this is ugly.. re do this in the future
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
