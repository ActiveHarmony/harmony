# this has nothing to do with the Simplex method used to solve linear 
# programs. This is a method for the minimization of a function of n 
# variables, which depende on the comnparions of function values at the
# (n+1) vertices of a general simplex, followed by the replacement of the 
# vertex with the highest value by another point.
#
# for reference consult: 
# "Simplex method for function minimzation" by J.A Nelder and R. Mead
# 
# Note: this method was "adapted" to work for a discontinuous function
# we are not sure yet of the performances of this mutant.

# we define for each application the following variables:
#
# AppName_simplex_step - tells us where in the simplex algorithm we 
#                         have to resume with the new computed value
#
# AppName_simplex_points - an array of lists, each list containing the 
#                           "coordinates" of the point (the actual values)
#                           of the bundles
#
# AppName_simplex_npoints - the number of points
#
# AppName_simplex_performance - array containing the values of the performance
#                         function for all the points of the simplex
#
# AppName_simplex_low - an array with two cells: index and value of the best performance
# 
# AppName_simplex_high - the index and value of the worst performance 
#
# AppName_simplex_nhigh - the index and value of the next-worst performance
# 
# AppName_simplex_alpha - the value of the reflection coefficient
#
# AppName_simplex_beta - the value of the contraction coefficient (0<beta<1)
#
# AppName_simplex_gamma - the value of the expansion coefficient (>1)
#
# AppName_simplex_centroid - the centroid of the simplex
#
# AppName_simplex_pstar - the P* point
#
# AppName_simplex_ystar - the value of the performance in P*
#
# AppName_simplex_pdstar - the P** point
#
# AppName_simplex_ydstar - the value of the performance in P**
#
# AppName_simplex_time - the minimum time-stamp we need to get from the client
#                         in order to be able to continue with the simplex
#                         algorithm. This way we are assured that the 
#                         performance value that we get reflects the changes
#                         made to the parameters
#

# this function computes the new simplex until no visible improvement in the 
# performance function. For now we will run the simplex until the performance
# does not change in terms of units (3 units)
#
proc simplex_method {appName yvalue} {

    global ${appName}_bundles
    global ${appName}_simplex_points
    global ${appName}_simplex_centroid
    global ${appName}_simplex_step
    global ${appName}_simplex_pstar
    global ${appName}_simplex_pdstar
    global ${appName}_simplex_performance
    global ${appName}_simplex_pstar
    global ${appName}_simplex_ystar
    global ${appName}_simplex_pdstar
    global ${appName}_simplex_ydstar
    
    upvar #0 ${appName}_simplex_points points
    upvar #0 ${appName}_simplex_npoints npoints
    upvar #0 ${appName}_simplex_high high
    upvar #0 ${appName}_simplex_nhigh nhigh
    upvar #0 ${appName}_simplex_low low
    upvar #0 ${appName}_simplex_alpha alpha
    upvar #0 ${appName}_simplex_beta beta
    upvar #0 ${appName}_simplex_gamma gamma
    upvar #0 ${appName}_simplex_centroid centroid
    upvar #0 ${appName}_simplex_step step
    upvar #0 ${appName}_simplex_pstar pstar
    upvar #0 ${appName}_simplex_ystar ystar
    upvar #0 ${appName}_simplex_pdstar pdstar
    upvar #0 ${appName}_simplex_ydstar ydstar
    upvar #0 ${appName}_simplex_performance performance
    upvar #0 ${appName}_bundles bundles
    
    global temporary
    
    puts "Entered simplex with step: $step ; yvalue=$yvalue"
    puts $performance
    parr ${appName}_simplex_points
    
    if {$step == 0} {
        
        # Provide initial simplex construction methods here.
        
        puts "********** Step 0 : $npoints ; $bundles"
        collect_bundles $appName $npoints
        lappend performance $yvalue
        incr npoints
        if {$npoints > [llength $bundles]} {
            incr step
            simplex_method $appName $yvalue
        } else {
            upvar #0 init_simplex_method init_method
            set_ith_bundle_to_given_value $appName [expr $npoints-1] $init_method
        }  
    } elseif {$step == 1} {
        puts "=============== Step 1 ; $yvalue"
        
	    low_high_index $appName 
	    
	    compute_centroid $appName
	    
        puts "Centroid: $centroid"
        
	    #compute P*
	    set temp {}
	    for {set j 0} {$j < [llength $centroid]} {incr j} {
            lappend temp [expr [lindex $centroid $j] * (1.0 + $alpha) - $alpha * [lindex $points($high(index)) $j]]
	    }
	    
	    puts "New values for params: $temp"
	    
	    set_bundles_to_new_values $appName $temp
        
	    recollect_bundles $appName ${appName}_simplex_pstar
        
	    puts "Pstar: $pstar"
        
	    incr step
        
	    puts "Step: $step"
    } elseif {$step == 2} {
        puts "********** Step 2: $performance ; $low(value) ; $yvalue ; $nhigh(value)"
        set ystar $yvalue
        if {$low(value) < $ystar && $ystar <= $nhigh(value)} {
            #replace Ph by P*
            puts "Replace Ph by P* : $ystar"
            puts "Performance initially: $performance"
            set ${appName}_simplex_points($high(index)) $pstar
            set ${appName}_simplex_performance [lreplace $performance $high(index) $high(index) $ystar]
            puts "New performance: $performance"
            set ${appName}_simplex_step 1
            simplex_method $appName $yvalue
        } elseif {$ystar < $low(value)} {
            #compute P**
            set temp {}
            for {set j 0} {$j < [llength $centroid]} {incr j} {
                lappend temp [expr [lindex $centroid $j] * (1.0 - $gamma) + $gamma * [lindex $pstar $j]]
            }
            
            puts "Computed P**: $temp"
            
            set_bundles_to_new_values $appName $temp
            
            recollect_bundles $appName ${appName}_simplex_pdstar
            
            puts "Pdstar: $pdstar"
            
            incr step
        } else { #means $nhigh(value) < $ystar
            if {$ystar < [lindex $performance $high(index)]} {
                set ${appName}_simplex_performance [lreplace $performance $high(index) $high(index) $ystar]
            }
            puts "Compute P**"
            #compute P**
            set temp {}
            for {set j 0} {$j < [llength $centroid]} {incr j} {
                lappend temp [expr [lindex $centroid $j] * (1.0 - $beta) + $beta * [lindex $points($high(index)) $j]]
            }
            
	    puts "Setting bundles to :$temp!"
    
	    set_bundles_to_new_values $appName $temp

	    puts "Bundles set!"

	    recollect_bundles $appName ${appName}_simplex_pdstar 
	    
	    puts "Pdstar: $pdstar"

	    incr step
	    incr step
	}
    } elseif {$step==3} {
	set ydstar $yvalue
	puts "============ Step 3: $performance ; $low(value) ; $low(index) ; $high(value) ; $high(index) ; $ydstar ; $ystar"
	if {$ydstar < $low(value)} {
	    puts "Replace $high(value) with $ydstar"
	    set ${appName}_simplex_performance [lreplace $performance $high(index) $high(index) $ydstar]
	    set ${appName}_simplex_points($high(index)) $pdstar
	} else {
	    puts "Replace $high(value) with $ystar"
	    set ${appName}_simplex_performance [lreplace $performance $high(index) $high(index) $ystar]
	    set ${appName}_simplex_points($high(index)) $pstar
	}
	puts " New performance: $performance"
	set ${appName}_simplex_step 1
	simplex_method $appName $yvalue
    } elseif {$step==4} {
	puts "Step 4!!!!! $ydstar ; $npoints"
	set ${appName}_simplex_ydstar $yvalue
	if {$ydstar <= [min $high(value) $ystar]} {
	    set ${appName}_simplex_performance [lreplace $performance $high(index) $high(index) $ydstar]
	    set ${appName}_simplex_points($high(index)) $pdstar

	    set ${appName}_simplex_step 1
	    simplex_method $appName $yvalue
	} else {
	    puts "Shrinking towards min! $performance"
	    
	    parr ${appName}_simplex_low
	    parr ${appName}_simplex_points

	    # this keeps the value of the 
	    global ${appName}_simplex_refp
	    set ${appName}_simplex_refp $points($low(index))

	    #for {set i 0} {$i < $npoints} {incr i} {set ${appName}_simplex_points($i) {} }

	    set npoints 0

	    upvar \#0 ${appName}_simplex_refp refp
	    set temp {}
	    for {set j 0} {$j < [llength $centroid]} {incr j} {
		puts "Shrink from [lindex $points($npoints) $j] to [lindex $refp $j] + [lindex $points($npoints) $j])/2]"
		lappend temp [expr ([lindex $refp $j] + [lindex $points($npoints) $j])/2]
		if {$npoints==$low(index)} {
		    set_ith_bundle_to_given_value $appName $j  [expr ([lindex $refp $j] + [lindex $points($npoints) $j])/2]
		}
	    }

	    puts $temp

	    if {$npoints!=$low(index)} {
		set_bundles_to_new_values $appName $temp
	    }
	    collect_bundles $appName $npoints

	    puts "Point 0: $points(0)"

	    incr step
	}
    } elseif {$step==5} {
        puts "Step 5............ $npoints ; [llength $performance] ; $performance"
        set ${appName}_simplex_performance [lreplace $performance $npoints $npoints $yvalue]
        
        puts $performance
        parr ${appName}_simplex_low
        
        incr npoints
        
        if {$npoints<[llength $performance]} {
            upvar \#0 ${appName}_simplex_refp refp
            set temp {}
            for {set j 0} {$j < [llength $centroid]} {incr j} {
                puts "Shrink from [lindex $points($npoints) $j] to [lindex $refp $j] + [lindex $points($npoints) $j])/2]"
                lappend temp [expr ([lindex $refp $j] + [lindex $points($npoints) $j])/2]
                if {$npoints==$low(index)} {
                    set_ith_bundle_to_given_value $appName $j  [expr ([lindex $refp $j] + [lindex $points($npoints) $j])/2]
                }
            }
            
            
            puts $temp
            
            if {$npoints!=$low(index)} {
                set_bundles_to_new_values $appName $temp
            }
            collect_bundles $appName $npoints
            puts "Point $npoints: $points([expr $npoints-1])"
        } else {
            set ${appName}_simplex_step 1
        }
    }
}



# this function computes the low, high, and next-high indexes
proc low_high_index {appName} {

    global ${appName}_simplex_low
    global ${appName}_simplex_high
    global ${appName}_simplex_nhigh

    upvar #0 ${appName}_simplex_npoints npoints
    upvar #0 ${appName}_simplex_points points
    upvar #0 ${appName}_simplex_low low
    upvar #0 ${appName}_simplex_high high
    upvar #0 ${appName}_simplex_nhigh nhigh
    upvar #0 ${appName}_simplex_performance perf
    upvar #0 ${appName}_simplex_high(index) hindex
    upvar #0 ${appName}_simplex_high(value) hvalue

    set ${appName}_simplex_low(index) 0
    set ${appName}_simplex_low(value) [lindex $perf 0]

    if {[lindex $perf 0]>[lindex $perf 1]} {
	set ${appName}_simplex_high(index) 0
	set ${appName}_simplex_high(value) [lindex $perf 0]
	set ${appName}_simplex_nhigh(index) 1
	set ${appName}_simplex_nhigh(value) [lindex $perf 1]
    } else {
	set ${appName}_simplex_high(index) 1
	set ${appName}_simplex_high(value) [lindex $perf 1]
	set ${appName}_simplex_nhigh(index) 0
	set ${appName}_simplex_nhigh(value) [lindex $perf 0]
    }


    for {set i 0} {$i < $npoints} {incr i} {
	if {[lindex $perf $i] < $low(value)} {
	    set ${appName}_simplex_low(index) $i
	    set ${appName}_simplex_low(value) [lindex $perf $i]
	}
	if {[lindex $perf $i] > $high(value)} {
	    set ${appName}_simplex_nhigh(index) $hindex
	    set ${appName}_simplex_nhigh(value) $hvalue
	    set ${appName}_simplex_high(index) $i
	    set ${appName}_simplex_high(value) [lindex $perf $i]
	} elseif {([lindex $perf $i] > $nhigh(value)) && ([lindex $perf $i]<$high(value))} {
	    set ${appName}_simplex_nhigh(index) $i
	    set ${appName}_simplex_nhigh(value) [lindex $perf $i]	    
	}
    }
}


proc collect_bundles {appName indx} {
    global ${appName}_simplex_points 
    
    global ${appName}_simplex_points($indx)

    upvar #0 ${appName}_bundles bundles

    set bundle_list {}
    puts "AppName: $appName ; indx: $indx"
    puts "Collected bundles: $bundle_list"

    foreach bun $bundles {
        upvar #0 ${appName}_bundle_${bun}(value) bunv
        puts "Collecting $bun ; $bunv"
        lappend bundle_list $bunv
    }

    puts "Collected bundles: $bundle_list"
    set ${appName}_simplex_points($indx) $bundle_list
}


proc recollect_bundles {appName where} {
    global ${appName}_simplex_points 
    global $where
    upvar #0 ${appName}_bundles bundles
    set bundle_list {}
    foreach bun $bundles {
        upvar #0 ${appName}_bundle_${bun}(value) $bun
        lappend bundle_list [expr $$bun]
    }
    set $where $bundle_list
}


proc compute_centroid {appName} {

    global ${appName}_simplex_points
    global ${appName}_simplex_centroid

    upvar #0 ${appName}_simplex_points points
    upvar #0 ${appName}_simplex_npoints npoints

    upvar #0 ${appName}_simplex_centroid centroid

    set centroid $points(0)

    for {set i 1} {$i < $npoints} {incr i} {
	set temp {}
	for {set j 0} {$j < [llength $centroid]} {incr j} {
	    lappend temp [expr [lindex $centroid $j] + [lindex $points($i) $j]]
	}
	set ${appName}_simplex_centroid $temp
    }

    set temp {}
    for {set j 0} {$j < [llength $centroid]} {incr j} {
	lappend temp [expr [lindex $centroid $j] / ($npoints + 0.0)]
    }
    set ${appName}_simplex_centroid $temp
}


proc set_bundles_to_new_values {appName values} {
    
    upvar #0 ${appName}_bundles bundles
    
    
    global temporary

    set ok 0
    for {set sign 1} {($ok==0) && ($sign>-2)} {incr sign -2} {
	for {set retry -1} {($ok==0) && ($retry < [llength $values])} {incr retry} {
	    puts "Retry : $retry ; [llength $values] ; $ok ; sign = $sign"
#	    puts "------"
	    set i 0
	    foreach bun $bundles {
		upvar #0 ${appName}_bundle_${bun}(value) bunv
		upvar \#0 ${appName}_bundle_${bun}(minv) minv
		upvar \#0 ${appName}_bundle_${bun}(maxv) maxv
		upvar #0 ${appName}_bundle_${bun}(stepv) stepv
		upvar #0 ${appName}_bundle_${bun} $bun
		if {$i < [llength $values]} {
		    set closestv [closest_value ${appName}_bundle_${bun} [lindex $values $i]]
		    set proposedv [expr $closestv + $stepv*($i==$retry)*$sign]
 
#		    puts "Values: $closestv ; $proposedv ; $minv ; $maxv"
		    if {($proposedv >= $minv) && ($proposedv <= $maxv)} {
#			puts "Proposed passed!"
			set ${bun}(value) $proposedv
		    } else {
			set ${bun}(value) $closestv
		    }
		    redraw_dependencies $bun $appName 0 0
		    incr i
		}
	    }
	    recollect_bundles $appName temporary   
	    if {[exists_point $temporary ${appName}]} {
		set ok 0
	    } else {
#		            puts "point does not exist"
		set ok 1
	    }
	}
#	puts "OK: $ok ;($ok==0) && ($retry < [llength $values]) ; sign = $sign "
    }

    global ${appName}_simplex_time
    global ${appName}_time
    upvar #0 ${appName}_time time
#    puts $time
#    puts ${appName}_simplex_time
    set ${appName}_simplex_time $time
}


proc exists_point {temp appName} {
    upvar #0 ${appName}_simplex_npoints npoints
    upvar #0 ${appName}_simplex_points points
    for {set i 0} {$i < $npoints} {incr i} {
        #puts "Checking $temp against $points($i) ; $npoints ; $i"
        if {$temp == $points($i)} {
            puts "Found equality: $temp = points($i)=$points($i)"
            return 1}
    }
    return 0
}


proc too_similar_values {temp appName} {

    upvar #0 ${appName}_simplex_npoints npoints
    upvar #0 ${appName}_simplex_points points
    upvar \#0 ${appName}_simplex_low low
    upvar \#0 ${appName}_simplex_high high
    upvar \#0 ${appName}_simplex_centroid centroid

    set diffs {}
    for {set i 0} {$i < [llength $centroid]} {incr i} {
	lappend diffs 0
    }

    for {set j 0} {$j < $npoints} {incr j} {
	for {set i 0} {$i < [llength $centroid]} {incr i} {
	    if {([lindex $temp $i] != [lindex $points($j) $i])} {
		set diffs [lreplace $diffs $i $i [expr [lindex $diffs $i]+1]]
	    }
	}
    }

    puts "Diffs: $diffs"

    for {set i 0} {$i < [llength $centroid]} {incr i} {
	if {[lindex $diffs $i]<2} {
	    puts "Found similarity in position $i"
	    return 1
	}
    }
    
    return 0
    
}

proc too_similar_values_2 {temp appName} {
    upvar #0 ${appName}_simplex_npoints npoints
    upvar #0 ${appName}_simplex_points points
    upvar \#0 ${appName}_simplex_low low
    upvar \#0 ${appName}_simplex_high high
    upvar \#0 ${appName}_simplex_centroid centroid

    for {set i 0} {$i < [llength $centroid]} {incr i} {
	if {([lindex $temp $i] == [lindex $points($low(index)) $i]) && \
		([lindex $temp $i] == [lindex $centroid $i])} {
	    puts "Similar: $i ; $temp ; $centroid"
	    return 1
	}
    }

    return 0
}

proc closest_value {bundleName value} {
 
    global $bundleName

    upvar #0 ${bundleName}(domain) domain

    for {set i 0} {($i < [llength $domain]) && ([lindex $domain $i] < $value)} {incr i} { }
    
    if {$i==0} {
	set ${bundleName}(value) [lindex $domain $i]
    } elseif {$i >= [llength $domain]} {
	set ${bundleName}(value) [lindex $domain end]
    } else {
	set dif1 [expr [lindex $domain $i]-$value]
	set dif2 [expr $value - [lindex $domain [expr $i-1]]]
	if {$dif1 > $dif2} {
	    set ${bundleName}(value) [lindex $domain [expr $i-1]]
	} else {
	    set ${bundleName}(value) [lindex $domain $i]
	}
    }
} 

proc set_ith_bundle_to_given_value {appName indx value} {
    
    upvar #0 ${appName}_bundles bundles

    set bun [lindex $bundles $indx]

    upvar #0 ${appName}_bundle_${bun}(value) bunv
    upvar #0 ${appName}_bundle_${bun}(maxv) maxv
    upvar #0 ${appName}_bundle_${bun}(minv) minv
   
    upvar #0 ${appName}_bundle_${bun} $bun

    switch -- $value {
	"max" {
	    set ${bun}(value) $maxv
	}
        "center" {
            set temp [expr ($minv+$maxv)/2]
            set ${bun}(value) [closest_value ${appName}_bundle_${bun} $temp]
        }
        "user_defined" {
            # first set all the other bundles to their initial values
            foreach ind_bun $bundles {
                upvar #0 ${appName}_bundle_${ind_bun}(value) curr_val
                upvar #0 ${appName}_bundle_${ind_bun}(ivalue) init_val
                set curr_val $init_val
            }
            upvar #0 ${appName}_bundle_${bun}(iscale) iscale
            set temp [expr $bunv+$iscale]
            set ${bun}(value) [closest_value ${appName}_bundle_${bun} $temp]
        }
	default {
	    puts $value
	    puts [closest_value ${appName}_bundle_${bun} $value]
	    set ${bun}(value) [closest_value ${appName}_bundle_${bun} $value]
	}
    }
    redraw_dependencies $bun $appName 0 0
}


proc print_all_bundles {appName} {

    puts "Printing bundles..."

    upvar #0 ${appName}_bundles bundles
    
    foreach bun $bundles {
	global ${appName}_bundle_${bun}
	parr ${appName}_bundle_${bun}
    }
}

proc min {v1 v2} {

    puts "Compute Min: $v1 ; $v2"
    if {$v1<=$v2} {
	return $v1
    } else {
	return $v2
    }
}

