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
proc http_status { } {
    set retval ""

    global current_appName
    if { [info exists current_appName] == 1 } {
        append retval "app:" $current_appName "|"

        set varlist ""
        set ignore_list {}
        upvar #0 ${current_appName}_bundles bundles
        if { [info exists bundles] } {
            foreach var [split $bundles] {
                append varlist "g:" $var ","

                lappend ignore_list $var
                upvar #0 ${current_appName}_bundle_${var}(deplocals) locals
                if { [info exists locals] } {
                    lappend ignore_list [split $locals]
                }
            }
        }

        foreach bundle [info globals *_bundles] {
            upvar #0 $bundle bun
            foreach var [split $bun] {
                if { [lsearch -exact $ignore_list $var] == -1 } {
                    append varlist "l:" $var ","
                }
            }
        }
        append retval "var:" [string trimright $varlist ","] "|"

        global best_coordinate_so_far
        global best_perf_so_far
        if { [info exists best_coordinate_so_far] &&
             [string length $best_coordinate_so_far] > 0 &&
             [info exists best_perf_so_far] &&
             [string length $best_perf_so_far] > 0 } {

            set best_coord ""
            foreach val [split $best_coordinate_so_far " "] {
                append best_coord "," $val
            }
            append retval "coord:best" $best_coord "," $best_perf_so_far "|"
        }

    } else {
        append retval "app:\[No Application Connected\]";
    }

    return $retval
}

proc get_http_test_coord { appName } {
    set out_str $appName
    upvar #0 ${appName}_bundles buns
    foreach bun $buns {
        upvar #0 ${appName}_bundle_${bun}(value) v
        append out_str "," $bun ":" $v
    }
    return $out_str
}

# Small helper function used completely for debugging.
# It should be removed at some point in the future.
proc var_to_string { args } {
    set return_string ""
    
    foreach arg $args {
        foreach varName [split $arg " "] {
            append return_string $varName
            upvar $varName var

            if { [array exists var] == 1 } {
                append return_string " {\n"
                foreach index [array names var] {
                    append return_string "\t" $index " => " $var($index) "\n"
                }
                append return_string "}\n"

            } elseif { [info exists var] == 1 } {
                append return_string " => " $var "\n"

            } else {
                append return_string " does not exist.\n"
            }
        }
    }

    return $return_string
}

proc signal_go { appName expand_best_local } {
    puts "inside signal_go"
    upvar #0 ${appName}_procs deplocals1

    puts "tcl check 1"
    upvar #0 best_coordinate_so_far best_coord
    puts "tcl check 1"
    foreach process $deplocals1 {
	puts "process $process"
        upvar #0 ${process}_expand_best expand_best
        upvar #0 ${process}_simplex_time s_time
        incr s_time
        if {$expand_best_local == 1} {
            set expand_best 1
        } else {
            set expand_best 0
        }
        upvar #0 ${process}_next_iteration next
        set next 1
        upvar #0 ${process}_search_done s_done
        set s_done 0
        upvar #0 ${process}_best_coord_so_far bc
        set bc $best_coord
    }
    puts "done signalling"
}


proc signal_all_done { appName best_so_far } {
    upvar #0 ${appName}_procs deplocals1
    foreach process $deplocals1 {
        upvar #0 ${process}_search_done s_done
        set s_done 1
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
        set counter 0
        upvar #0 ${appName}_bundles bun_names
        foreach bun $disp_low_point {
            set current_bun_name [lindex $bun_names $counter]
            incr counter
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

proc get_next_configuration_dep { appName } {
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

# unique configuration
proc get_next_configuration { appName } {

     set min 0

     set out_str ""

     #creating the list of the bundles
     #set bundle_list[list]

     upvar #0 ${appName}_bundles buns

     foreach bun $buns {

	 upvar #0 ${appName}_bundle_${bun}(domain) domain

	 set idx_upper [expr [llength $domain]]

	 set range [expr {$idx_upper-$min}]
	 
	 set idx [expr {int($min+$range*rand())}]

	 append out_str $idx "_"

     }

     set out_str [string range $out_str 0 [expr [string length $out_str] -2]]
     append $out_str "\0"
     return $out_str
    
}


proc get_next_configuration_by_name { appName varName } {
    set min 1
    set out_str ""

    #creating the list of the bundles
    set bundle_list [list]

    #put each varName in the list
    lappend bundle_list ${varName}

    #search for the unique varName
    if{ [lsearch -exact $bundle_list ${varName}] != -1 } 
    {
	
	upvar #0 ${appName}_bundle_${varName}(domain) domain
    
	set idx_upper [expr [llength $domain]-1]

	#unique random number generator
	set idx [expr {int($min+(($idx_upper-$min)*rand()))}]

	append out_str  [lindex $domain $idx]
   
	append $out_str "\0"
   
	return $out_str

    }
    else 
    {

	upvar #0 ${appName}_bundle_${varName}(domain) domain
	
        set idx_upper [expr [llength $domain]-1]

	#unique random number generator
	set idx [expr {int($min+(($idx_upper-$min)*rand()))}]

	append out_str  [lindex $domain $idx]
   
	append $out_str "\0"
   
	return $out_str
    }
     
}


proc round_towards_min { wrt point step } {
    # here the assumption is that the step-size is one
    #    i.e. the co-ordinate system is normalized
    # Change this should the step size change
    
    #set step 1
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
    upvar #0 simplex_iteration iter
    upvar #0 codegen_conf_host c_host
    upvar #0 codegen_conf_dir c_path

    set tmp_filename ""
    append tmp_filename "/tmp/harmony-tmp." [pid] "." [clock clicks]
    set fname [open $tmp_filename "w"]

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

    set dst_filename "$c_host:$c_path/candidate_simplex.$appName.$iter.dat"
    exec "scp" $tmp_filename $dst_filename
    file delete $tmp_filename
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
