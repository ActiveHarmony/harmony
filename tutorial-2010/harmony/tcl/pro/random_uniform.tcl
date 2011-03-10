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

#set temp [random_uniform 100 900 5]
#set simplex {{1 2} {2 3} {4 5} {6 7} {8 9}}
#set tacked [tack_on $temp $simplex]
#puts $tacked
#puts [lsort -index 0 $tacked]
#puts $temp

