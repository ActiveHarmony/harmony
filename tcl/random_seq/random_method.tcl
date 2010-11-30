proc random_init {appName} {

    
    upvar #0 ${appName}_bundles bundles
    global ${appName}_simplex_time
    set ${appName}_simplex_time -1

    foreach bun $bundles {
        upvar #0 ${appName}_bundle_${bun}(value) bunv
        upvar #0 ${appName}_bundle_${bun}(minv) minv
        upvar #0 ${appName}_bundle_${bun}(maxv) maxv

        upvar #0 ${appName}_bundle_${bun} $bun
        set bunv [random_uniform $minv $maxv 1]
        puts $bunv
    }
}


proc random_method {appName} {

    upvar #0 ${appName}_bundles bundles
    foreach bun $bundles {
        upvar #0 ${appName}_bundle_${bun}(value) bunv
        upvar #0 ${appName}_bundle_${bun}(minv) minv
        upvar #0 ${appName}_bundle_${bun}(maxv) maxv
        set bunv [random_uniform $minv $maxv 1]
    }
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
