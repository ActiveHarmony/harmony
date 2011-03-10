
proc random_method {appName} {

    puts "appname is $appName"
    set aName [string range $appName 0 [expr [string last "_" $appName]-1]]
    puts "global name is $aName"
    # record the best performance, the rest are in the log files
    upvar #0 ${appName}_obsGoodness(value) this_perf
    upvar #0 ${appName}_best_perf_so_far best_perf_so_far
    upvar #0 ${appName}_best_coordinate_so_far best_point
    
    # book-keeping
    if { $this_perf < $best_perf_so_far } {
	set best_perf_so_far $this_perf
	# collect the values
	upvar #0 ${appName}_bundles bundles
	upvar #0 ${appName}_best_coordinate_so_far best_point
	set temp_ls {}
	foreach bun $bundles {
	    upvar #0 ${appName}_bundle_${bun}(value) val
	    lappend temp_ls $val
	}
        set best_point $temp_ls
    }

    puts "Trying to print the best values"

    puts -nonewline "best coord so far :: "
    puts [list_to_string  $best_point]
    puts -nonewline "best perf so far :: "
    puts  $best_perf_so_far

    upvar #0 ${appName}_bundles bundles
    foreach bun $bundles {
        upvar #0 ${appName}_bundle_${bun}(value) bunv
        upvar #0 ${appName}_bundle_${bun}(minv) minv
        upvar #0 ${appName}_bundle_${bun}(maxv) maxv
        upvar #0 ${appName}_bundle_${bun}(domain) domain
        parr $domain
        set idx_upper [expr [llength $domain]-1]
        puts "upper bound on index: $idx_upper"
        set idx  0
        if { $idx_upper > 0 } {
            set idx  [random_uniform 0 $idx_upper 1]
        }
        #puts "random index:: $idx"
        set bunv [lindex $domain $idx]
        redraw_dependencies $bun $appName 0 0
    }
    global ${appName}_simplex_time
    incr ${appName}_simplex_time 
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

#### This is taken from tcllib-1.10
## module:: math
## file::  stat.tcl


# Inverse-cdf-uniform --
#    Return the argument belonging to the cumulative probability
#    for a uniform distribution (parameters as minimum/maximum)
#
# Arguments:
#    pmin      Minimum of the distribution
#    pmax      Maximum of the distribution
#    prob      Cumulative probability for which the "x" value must be
#              determined
#
# Result:
#    X value that gives the cumulative probability under the
#    given distribution
#
proc inverse-cdf-uniform { pmin pmax prob } {

    if {0} {
	if { $pmin >= $pmax } {
	    return -code error -errorcode ARG \
		    -errorinfo "Wrong order or zero range" \
		    "Wrong order or zero range"
	}
    }

    set x [expr {$pmin+$prob*($pmax-$pmin)}]

    if { $x < $pmin } { return $pmin }
    if { $x > $pmax } { return $pmax }

    return [expr {round($x)}]
}

proc random_uniform { pmin pmax number } {

    if { $pmin >= $pmax } {
	return -code error -errorcode ARG \
		-errorinfo "Wrong order or zero range" \
		"Wrong order or zero range"
    }

    set result {}
    for { set i 0 }  {$i < $number } { incr i } {
	lappend result [inverse-cdf-uniform $pmin $pmax [expr {rand()}]]
    }

    return $result
}
proc tack_on { ls_1  ls_2 } {
    set index 0
    set out_list {}
    foreach elem $ls_1 {
        lappend out_list [join [list $elem [lindex $ls_2 $index]]]
        incr index
    }
    return $out_list
}
