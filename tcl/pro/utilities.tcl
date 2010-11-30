
proc signal_go { appName expand_best_local } {
    upvar #0 ${appName}_procs deplocals1
    upvar #0 best_coordinate_so_far best_coord
    foreach process $deplocals1 {
        upvar #0 ${process}_expand_best expand_best
        if {$expand_best_local == 1} {
            set expand_best 1
        } else {
            set expand_best 0
        }
        upvar #0 ${process}_next_iteration next
        set next 1
        upvar #0 ${process}_all_done a_done
        set a_done 0
        upvar #0 ${process}_best_coord_so_far bc
        set bc $best_coord
    }
}


proc signal_all_done { appName best_so_far } {
    upvar #0 ${appName}_procs deplocals1
    foreach process $deplocals1 {
        upvar #0 ${process}_all_done a_done
        set a_done 1
    }

    upvar #0 simplex_npoints size
    upvar #0 simplex_points points
    
    set temp [replicate_rows $size $best_so_far]
    set points $temp
    assign_values_to_bundles $appName $points

}

proc user_puts { message } {
    upvar #0 debug_mode debug_
    if { $debug_ == 1 } {
        puts $message
    }
}

proc set_label_texts { appName iteration } {
    #global ${appName}_procs
    upvar #0 ${appName}_procs deplocals1
    foreach process $deplocals1 {
        upvar #0 ${process}_label_text label_text
        upvar #0 ${process}_bundles bundles
        set label_text " "
        foreach bun $bundles {
            upvar #0 ${process}_bundle_${bun}(value) val
            set justify_text ""
            if { $val <= 9 } {
                append justify_text "$val       "
            } elseif { ($val >= 10 ) && ($val < 100) } {
                append justify_text "$val      "
            } elseif { ($val >= 100) && ($val < 1000) } {
                append justify_text "$val     "
            } else {
                append justify_text "$val    "
            } 
            append label_text " ${bun}=$justify_text"
        }
    }
    
    # for display purposes set the current best simplex point as the label text
    #  for main window

    if { $iteration > 2 } {
        upvar #0 simplex_low disp_lowest
        upvar #0 simplex_low(index) disp_index
        upvar #0 label_text label_text_main
        upvar #0 simplex_points disp_points
        upvar #0 best_perf_so_far best_perf_so_far
        upvar #0 best_coordinate_so_far disp_low_point
        set label_text_main ""
        upvar #0 ${appName}_label_text label_main
        set label_main ""
        #puts "\#\#\#\#\#\#\#\#\#\#\#\#\#\#${appName}_label_text"
        # our current best point
        ##set disp_low_point [lindex $disp_points $disp_index]
        set counter 0
        upvar #0 ${appName}_bundles bun_names
        foreach bun $disp_low_point {
            set current_bun_name [lindex $bun_names $counter]
            incr counter
            #append label_text_main " $current_bun_name: $bun "
            append label_main " $current_bun_name=$bun "
        }
    }
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
        lappend out_list [list $elem [lindex $ls_2 $index]]
        incr index
    }
    return $out_list
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

proc cdr {l} { return [lrange $l 1 end]}

proc list_to_string { ls } {
    set return_string ""
    foreach elem $ls {
        append return_string $elem " "
    }
    return $return_string
}



proc get_candidate_configuration { appName } {
    set out_str ""
    upvar #0 candidate_simplex c_points
    foreach point $c_points {
        set temp ""
        foreach dim $point {
            append temp $dim " "
        }
        append temp "\n"
        append out_str $temp
        set temp ""
    }
    append $out_str "\0"
    return $out_str 
}

proc get_test_configuration { appName } {
    set out_str ""
    upvar #0 ${appName}_bundles buns
    foreach bun $buns {
        upvar #0 ${appName}_bundle_${bun}(value) v
        append out_str $v "_"
    }
    set out_str [string range $out_str 0 [expr [string length $out_str] -2]]
    append $out_str "\0"
    return $out_str
}

proc round_towards_min { wrt point } {
    # here the assumption is that the step-size is one
    #    i.e. the co-ordinate system is normalized
    # Change this should the step size change
    
    set step 1
    set temp_ind {}
    for {set j 0} {$j < [llength $wrt]} {incr j} {
        if {[expr (abs ( [lindex $wrt $j] - [lindex $point  $j]))] < $step} {
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

proc list_to_string { ls } {
    set return_string ""
    foreach elem $ls {
        append return_string $elem " "
    }
    return $return_string
}

# the name says it all
proc print_all_bundles {appName} {
    upvar #0 ${appName}_bundles bundles
    foreach bun $bundles {
	upvar #0 ${appName}_bundle_${bun}(value) value
	puts $value
    }
}

# This proc is for writing logs.
proc logging { str appName flag } {
    set fname [open "${appName}_pro.log" {WRONLY APPEND CREAT} 0600]
    puts $fname $str
    if { $flag == 1 } {
        puts $fname "Simplex:"
        upvar #0 simplex_points points
        upvar #0 candidate_simplex c_points
        upvar #0 simplex_performance s_perf
        upvar #0 candidate_performance c_perf
        puts $fname [format_print_simplex $points $s_perf]
        puts $fname " "
        puts $fname "Candidate simplex:"
        puts $fname [format_print_simplex $c_points $c_perf]
        puts $fname " "
    }
    close $fname
}

proc write_candidate_simplex { appName } {
    upvar #0 candidate_simplex c_points
    upvar #0 simplex_iteration iteration
    upvar #0 code_generation_params(code_generation_destination) code_generation_dest
    
    set fname [open "/tmp/candidate_simplex.$iteration.dat" "w"]

    set i 0
    set out_str ""
    foreach point $c_points {
        foreach elem $point {
           append out_str " " $elem
        }
        append out_str "\n"
        incr i
    }
    puts $fname $out_str
    close $fname
    set filename_ "/tmp/candidate_simplex.$iteration.dat"
    scp_candidate $filename_ $code_generation_dest

}
proc send_candidate_simplex { appName } {
    upvar #0 candidate_simplex c_points
    upvar #0 simplex_iteration iteration
    set i 0
    set out_str ""
    foreach point $c_points {
        foreach elem $point {
           append out_str " " $elem
        }
        append out_str " : "
        incr i
    }

    append out_str " | "
    
    generate_code $out_str

}
proc format_print_simplex { simplex perf } {
    set i 0
    set out_str ""
    foreach point $simplex {
        foreach elem $point {
           append out_str "\t" $elem
        }
        append out_str "\t" [lindex $perf $i] "\n"
        incr i
    }
    return $out_str
}

proc format_initial_simplex { simplex } {
    set out_str ""
    foreach point $simplex {
        foreach elem $point {
        append out_str " " $elem
        }
        append out_str "\n"
    }
    return $out_str
}

# UGLY UGLY nested loops to generate all possible scaling matrices for different
#  dimensions from here on out. upto 9 dimensions.
proc scaling_matrix_1_d {} {
    set signs [list +1 -1]
    set rand__ [random_uniform 0 1 1]
    lappend return_list [lindex $signs $rand__]
    return $return_list
}


proc scaling_matrix_2_d {} {
    set signs [list +1 -1]
    set return_matrix [list]
    foreach sign_1 $signs {
        foreach sign_2 $signs {
            lappend return_list [list $sign_1 $sign_2 ]
        }
    }
    return $return_list
}

proc scaling_matrix_3_d {} {
    set signs [list +1 -1]
    set return_matrix [list]
    foreach sign_1 $signs {
        foreach sign_2 $signs {
                foreach sign_3 $signs {
                    lappend return_list [list $sign_1 $sign_2 $sign_3 ]
                }
            }
        }
    return $return_list
}

proc scaling_matrix_4_d {} {
    set signs [list +1 -1]
    set return_matrix [list]
    foreach sign_1 $signs {
        foreach sign_2 $signs {
                foreach sign_3 $signs {
                    foreach sign_4 $signs {
                            lappend return_list [list $sign_1 $sign_2 $sign_3 $sign_4 ]
                    }
                }
            }
        }
    return $return_list
}

proc scaling_matrix_5_d {} {
    set signs [list +1 -1]
    set return_matrix [list]
    foreach sign_1 $signs {
        foreach sign_2 $signs {
                foreach sign_3 $signs {
                    foreach sign_4 $signs {
                        foreach sign_5 $signs {
                            lappend return_list [list $sign_1 $sign_2 $sign_3 $sign_4 $sign_5 ]
                        }
                    }
                }
            }
        }
    return $return_list
}

proc scaling_matrix_6_d {} {
    set signs [list +1 -1]
    set return_matrix [list]
    foreach sign_1 $signs {
        foreach sign_2 $signs {
                foreach sign_3 $signs {
                    foreach sign_4 $signs {
                        foreach sign_5 $signs {
                            foreach sign_6 $signs {
                                lappend return_list [list $sign_1 $sign_2 $sign_3 $sign_4 $sign_5 $sign_6 ]
                            }
                        }
                    }
                }
            }
        }
    return $return_list
}

proc scaling_matrix_7_d {} {
    set signs [list +1 -1]
    set return_matrix [list]
    foreach sign_1 $signs {
        foreach sign_2 $signs {
            foreach sign_3 $signs {
                foreach sign_4 $signs {
                    foreach sign_5 $signs {
                        foreach sign_6 $signs {
                            foreach sign_7 $signs {
                                lappend return_list [list $sign_1 $sign_2 $sign_3 $sign_4 $sign_5 $sign_6 $sign_7]
                            }
                        }
                    }
                }
            }
            }
        }
    return $return_list
}

proc scaling_matrix_8_d {} {
    set signs [list +1 -1]
    set return_matrix [list]
    foreach sign_1 $signs {
        foreach sign_2 $signs {
            foreach sign_3 $signs {
                foreach sign_4 $signs {
                    foreach sign_5 $signs {
                        foreach sign_6 $signs {
                            foreach sign_7 $signs {
                                foreach sign_8 $signs {
                                    lappend return_list [list $sign_1 $sign_2 $sign_3 $sign_4 $sign_5 $sign_6 $sign_7 $sign_8]
                                }
                            }
                        }
                    }
                }
            }
            }
        }
    return $return_list
}

proc scaling_matrix_9_d {} {
    set signs [list +1 -1]
    set return_matrix [list]
    foreach sign_1 $signs {
        foreach sign_2 $signs {
            foreach sign_3 $signs {
                foreach sign_4 $signs {
                    foreach sign_5 $signs {
                        foreach sign_6 $signs {
                            foreach sign_7 $signs {
                                foreach sign_8 $signs {
                                    foreach sign_9 $signs {
                                        lappend return_list [list $sign_1 $sign_2 $sign_3 $sign_4 $sign_5 $sign_6 $sign_7 $sign_8 $sign_9]
                                    }
                                }
                            }
                        }
                    }
                }
            }
            }
        }
    return $return_list
}
