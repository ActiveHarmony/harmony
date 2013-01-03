#
# Copyright 2003-2013 Jeffrey K. Hollingsworth
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

proc list_to_string { ls } {
    set return_string ""
    foreach elem $ls {
        append return_string $elem " "
    }
    return $return_string
}

proc brute_force_done { appName } {
    upvar #0 ${appName}_search_done search_done
    set search_done 1
    puts "Brute Force Algorithm Done! "
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

# unique configuration
proc get_next_configuration { appName } {

     set min 0

     set out_str ""

     upvar #0 ${appName}_bundles buns

     foreach bun $buns {

	 upvar #0 ${appName}_bundle_${bun}(domain) domain

	 set idx_upper [expr [llength $domain]]

	 set range [expr {$idx_upper-$min}]

	 #unique random number generator
	 set idx [expr {int($min+$range*rand())}]

	 append out_str $idx "_"

     }

     set out_str [string range $out_str 0 [expr [string length $out_str] -2]]
     append $out_str "\0"
     return $out_str
    
}

proc get_candidate_configuration { appName } {
    set out_str ""
    upvar #0 ${appName}_bundles buns
    foreach bun $buns {
        upvar #0 ${appName}_bundle_${bun}(value) v
        append out_str $v " "
    }
    set out_str [string range $out_str 0 [expr [string length $out_str] -2]]
    append $out_str "\0"
    return $out_str
}

proc get_best_configuration { appName } {
    upvar #0 best_coordinate_so_far best_coord_so_far
    return $best_coord_so_far
}

proc write_candidate_simplex { appName } {
    set tmp_filename ""
    append tmp_filename "/tmp/harmony-tmp." [pid] "." [clock clicks]
    set fname [open $tmp_filename "w"]

    set out_str ""
    set c_points [get_candidate_configuration $appName]
    foreach elem $c_points {
        append out_str " " $elem 
    }
    puts $fname $out_str
    close $fname

    global code_generation_params
    upvar #0 code_generation_params(port) cs_port
    upvar #0 code_generation_params(user) cs_user
    upvar #0 code_generation_params(host) cs_host
    upvar #0 code_generation_params(path) cs_path

    upvar #0 ${appName}_code_timestep iter
    set dst_filename "$cs_path/candidate_simplex.$appName.$iter.dat"
    eval "exec scp $cs_port $tmp_filename ${cs_user}${cs_host}:$dst_filename"
    file delete $tmp_filename
    incr iter 1
}

proc send_candidate_simplex { appName } {
    global code_generation_params
    upvar #0 code_generation_params(enabled) cs_enabled
    upvar #0 code_generation_params(type) cs_type

    if { $cs_enabled == 1 } {
        if { $cs_type == 1 } {
            send_candidate_simplex_to_server $appName
        } else {
            write_candidate_simplex $appName
        }
    }
}

proc send_candidate_simplex_to_server { appName } {

    upvar #0 ${appName}_candidate_simplex [get_candidate_configuration $appName]
    global code_generation_params
    upvar #0 code_generation_params(connected) cs_connected
    upvar #0 code_generation_params(port) cs_port
    upvar #0 code_generation_params(host) cs_host

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
            # debug
            #generate_code "2 2 2 1 3 : 1 2 1 2 1 | 3 3 3 3 3"
        }
    }

    set out_str ""
    foreach elem $point {
        append out_str " " $elem
    }
    append out_str " | "

    # generate_code is a part of code-server API: callable from tcl via swig
    generate_code $out_str
}