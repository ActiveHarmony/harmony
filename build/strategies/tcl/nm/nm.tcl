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
    global ${appName}_simplex_time
    
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
    
    # OLDPENALTY
    # set a fake infinity function value to an infeasible point
    # regardless of what is received from a client
    if {0} {
        foreach bun $bundles {
            upvar #0 ${appName}_bundle_${bun}(domain) domain
            upvar #0 ${appName}_bundle_${bun}(value) v
            puts "OLDPENALTY: domain=$domain"
            puts "OLDPENALTY: value=$v"
            if {$v < [lindex $domain 0] || [lindex $domain end] < $v} {
                puts "OLDPENALTY: change yvalue $yvalue to infinity"
                set yvalue 999999999
            }
        }
    }

    puts "Entered simplex with step: $step ; yvalue=$yvalue"
    puts $performance

    parr ${appName}_simplex_points
    
    incr ${appName}_simplex_time

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
        # process the simplex at the current iteration
        puts "=============== Step 1 ; $yvalue"

        # execute the termination test here at the beginning of one algorithm step
        check_for_convergence $appName
        upvar #0 ${appName}_search_done simplex_converged
        if {$simplex_converged == 1} {
            puts "check_for_convergence: converged"
            return
        }

	low_high_index $appName 
	compute_centroid $appName
        puts "Centroid: $centroid"
	#compute P*: Reflection
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
        if {1} {
            if {[is_infeasible_point $appName]} {
                puts "PENALTY: The reflection point Pstar is infeasible."
                puts "PENALTY: Give the penalty value 999999999 to Pstar"
                simplex_method $appName 999999999
            }
        }
    } elseif {$step == 2} {
        # process Reflected simplex
        puts "********** Step 2: $performance ; $low(value) ; $yvalue ; $nhigh(value)"
        set ystar $yvalue
        if {$low(value) <= $ystar && $ystar <= $nhigh(value)} {
            #replace Ph by P*
            puts "Replace Ph by P* : $ystar"
            puts "Performance initially: $performance"
            set ${appName}_simplex_points($high(index)) $pstar
            set ${appName}_simplex_performance [lreplace $performance $high(index) $high(index) $ystar]
            puts "New performance: $performance"
            set ${appName}_simplex_step 1
            simplex_method $appName $yvalue
        } elseif {$ystar < $low(value)} {
            #compute P**: Expansion
            set temp {}
            for {set j 0} {$j < [llength $centroid]} {incr j} {
                lappend temp [expr [lindex $centroid $j] * (1.0 - $gamma) + $gamma * [lindex $pstar $j]]
            }
            
            puts "Computed P**: $temp"
            
            set_bundles_to_new_values $appName $temp
            
            recollect_bundles $appName ${appName}_simplex_pdstar
            
            puts "Pdstar: $pdstar"
            
            incr step
            # goto step 3
            if {1} {
                if {[is_infeasible_point $appName]} {
                    puts "PENALTY: The expansion point Pdstar is infeasible."
                    puts "PENALTY: Give the penalty value 999999999 to Pdstar"
                    simplex_method $appName 999999999
                }
            }
        } else {
            # entering this block means $nhigh(value) < $ystar
            # so, the pstar is worst or second worst

            if {$ystar <= [lindex $performance $high(index)]} {
                # Reflection is better than the original worst. Then contract with respect to Reflection
                puts "Reflection point is bad but better than the original worst."
                set ${appName}_simplex_performance [lreplace $performance $high(index) $high(index) $ystar]
	        set ${appName}_simplex_points($high(index)) $pstar
            }
            puts "Compute P**"
            #compute P**: Contraction
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
            # goto step 4
            if {1} {
                if {[is_infeasible_point $appName]} {
                    puts "PENALTY: The contraction point Pdstar is infeasible."
                    puts "PENALTY: Give the penalty value 999999999 to Pdstar"
                    simplex_method $appName 999999999
                }
            }
	}
    } elseif {$step==3} {
        # process Expanded simplex
	set ydstar $yvalue
	puts "============ Step 3: $performance ; $low(value) ; $low(index) ; $high(value) ; $high(index) ; $ydstar ; $ystar"
	if {$ydstar < $low(value)} {
            # best E
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
        # process Contracted simplex
	puts "Step 4!!!!! $ydstar ; $npoints"
	set ${appName}_simplex_ydstar $yvalue
	if {$ydstar <= [min $high(value) $ystar]} {
	    set ${appName}_simplex_performance [lreplace $performance $high(index) $high(index) $ydstar]
	    set ${appName}_simplex_points($high(index)) $pdstar

	    set ${appName}_simplex_step 1
	    simplex_method $appName $yvalue
	} else {
            # worst Contraction
	    puts "Shrinking towards min! $performance"
	    
	    parr ${appName}_simplex_low
	    parr ${appName}_simplex_points

	    # this keeps the value of the 
	    global ${appName}_simplex_refp
	    set ${appName}_simplex_refp $points($low(index))

	    set npoints 0

            # shrink first point only and move on to the next step
	    upvar \#0 ${appName}_simplex_refp refp
	    set temp {}
	    for {set j 0} {$j < [llength $centroid]} {incr j} {
		puts "Shrink from [lindex $points($npoints) $j] to ([lindex $refp $j] + [lindex $points($npoints) $j])/2"
		lappend temp [expr ([lindex $refp $j] + [lindex $points($npoints) $j])/2]
                # @@@ do we really need this? it is redundant to just use temp...
                # also, low point doesn't need to be recalculated...
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
            # go to step 5
            if {1} {
                if {[is_infeasible_point $appName]} {
                    puts "PENALTY: The first shrinked point temp is infeasible."
                    puts "PENALTY: Give the penalty value 999999999 to temp"
                    simplex_method $appName 999999999
                }
            }
	}
    } elseif {$step==5} {
        # process Shrinked simplex
        puts "Step 5............ $npoints ; [llength $performance] ; $performance"
        set ${appName}_simplex_performance [lreplace $performance $npoints $npoints $yvalue]
        
        puts $performance
        parr ${appName}_simplex_low
        
        incr npoints
        
        # shrink the next point until all the points are shrinked
        if {$npoints<[llength $performance]} {
            upvar \#0 ${appName}_simplex_refp refp
            set temp {}
            for {set j 0} {$j < [llength $centroid]} {incr j} {
                puts "Shrink from [lindex $points($npoints) $j] to ([lindex $refp $j] + [lindex $points($npoints) $j])/2"
                lappend temp [expr ([lindex $refp $j] + [lindex $points($npoints) $j])/2]
                # @@@ do we really need this? it is redundant to just use temp...
                # also, low point doesn't need to be recalculated...
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
            if {1} {
                if {[is_infeasible_point $appName]} {
                    puts "PENALTY: The shrinked point temp is infeasible."
                    puts "PENALTY: Give the penalty value 999999999 to temp"
                    simplex_method $appName 999999999
                }
            }
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

# This function checks if the current point (or a set of bundles)
#   is in an infeasible area.
# It supports only a boxed feasible area. ex) 0 < x1 < 10 and 0 < x2 < 20
# More complicated constraints such as x1 + x2 < 30 are not supported.
# For those complicated constraints, the Omega library is recommended to be used.
proc is_infeasible_point {appName} {
    upvar #0 ${appName}_bundles bundles
    foreach bun $bundles {
        upvar #0 ${appName}_bundle_${bun}(domain) domain
        upvar #0 ${appName}_bundle_${bun}(value) v
        puts "is_infeasible_point: domain=$domain"
        puts "is_infeasible_point: value=$v"
        if {$v < [lindex $domain 0] || [lindex $domain end] < $v} {
            puts "is_infeasible_point: YES"
            return 1
        }
    }
    puts "is_infeasible_point: NO"
    return 0
}

# This function computes the centroid of points of the current simplex except the worst point.
proc compute_centroid {appName} {

    global ${appName}_simplex_points
    global ${appName}_simplex_centroid

    upvar #0 ${appName}_simplex_points points
    upvar #0 ${appName}_simplex_npoints npoints

    upvar #0 ${appName}_simplex_centroid centroid

    upvar #0 ${appName}_simplex_high high

    # initialize a centroid point
    set centroid {}
    for {set i 0} {$i < [llength $points(0)]} {incr i} {
        lappend centroid 0
    }

    # exclude worst(highest) point from centroid computation
    for {set i 0} {$i < $npoints} {incr i} {
        #puts "points($i): $points($i)"
        #puts "points(high): $points($high(index))"
        if {$points($i) == $points($high(index))} {
            puts "skip the worst point $points($i) for centroid computation"
            continue
        }
	set temp {}
	for {set j 0} {$j < [llength $centroid]} {incr j} {
	    lappend temp [expr [lindex $centroid $j] + [lindex $points($i) $j]]
	}
	set ${appName}_simplex_centroid $temp
    }

    set temp {}
    for {set j 0} {$j < [llength $centroid]} {incr j} {
        # divide by npoints-1
	lappend temp [expr [lindex $centroid $j] / ($npoints - 1.0)]
    }
    set ${appName}_simplex_centroid $temp
    #puts "centroid = $centroid"
}


proc set_bundles_to_new_values {appName values} {
    
    upvar #0 ${appName}_bundles bundles
    global temporary

    set ok 0
    #for {set sign_i 1} {($ok==0) && ($sign_i<=1)} {incr sign_i 1}
    #  for {set sign_j 1} {($ok==0) && ($sign_j>-2)} {incr sign_j -2}
    #    set sign [expr $sign_i*$sign_j]
    for {set sign 1} {($ok==0) && ($sign>-2)} {incr sign -2} {
	for {set retry -1} {($ok==0) && ($retry < [llength $values])} {incr retry} {
	    puts "Retry : $retry ; [llength $values] ; $ok ; sign = $sign"
	    set i 0
	    foreach bun $bundles {
		upvar #0 ${appName}_bundle_${bun}(value) bunv
		upvar #0 ${appName}_bundle_${bun}(minv) minv
		upvar #0 ${appName}_bundle_${bun}(maxv) maxv
		upvar #0 ${appName}_bundle_${bun}(stepv) stepv
		upvar #0 ${appName}_bundle_${bun}(deplocals) deps
		upvar #0 ${appName}_bundle_${bun} $bun
		if {$i < [llength $values]} {
		    set closestv [closest_value ${appName}_bundle_${bun} [lindex $values $i]]
		    set proposedv [expr $closestv + $stepv*($i==$retry)*$sign]

                    set val_from [lindex $values $i]
                    puts "FROM: $val_from TO:$closestv PROPOSED:$proposedv"
 
		    if {($proposedv >= $minv) && ($proposedv <= $maxv)} {
			set ${bun}(value) $proposedv
		    } else {
			set ${bun}(value) $closestv
		    }

                    # Update any dependant local variables, if they exist.
                    if { [info exists deps] } {
                        foreach dep $deps {
                            upvar #0 $dep dep_bun
                            set dep_bun(value) $bunv
                        }
                    }

                    global ${appName}_time
                    incr ${appName}_time
		    incr i
		}
	    }
	    recollect_bundles $appName temporary   
            set ok 1
	    if {[exists_point $temporary ${appName}]} {
                set ok 0
                continue
	    }
	}
    }
    if {$ok == 0} {
        puts "proposedv not satisfied"
    }
}

# check if a point temp already exists in the current simplex
proc exists_point {temp appName} {
    upvar #0 ${appName}_simplex_npoints npoints
    upvar #0 ${appName}_simplex_points points
    #upvar #0 ${appName}_simplex_high high
    for {set i 0} {$i < $npoints} {incr i} {
        # skip worst point
        #if {$i == $high(index)} {
        #    puts "exists_point i=$i skip worst"
        #    continue
        #}
        if {$temp == $points($i)} {
            puts "Found equality: $temp = points($i)=$points($i)"
            return 1}
    }
    return 0
}


proc too_similar_values {temp appName} {

    upvar #0 ${appName}_simplex_npoints npoints
    upvar #0 ${appName}_simplex_points points
    upvar #0 ${appName}_simplex_low low
    upvar #0 ${appName}_simplex_high high
    upvar #0 ${appName}_simplex_centroid centroid

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
    upvar #0 ${appName}_simplex_low low
    upvar #0 ${appName}_simplex_high high
    upvar #0 ${appName}_simplex_centroid centroid

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

    # @@@ too much work to find min domain that is larger than value
    for {set i 0} {($i < [llength $domain]) && ([lindex $domain $i] < $value)} {incr i} { }
    # puts "i = $i value = $value d = [lindex $domain $i]"
    
    # do not move infeasible points to the constraint boundary
    if {$i==0} {
        set ${bundleName}(value) $value
    } elseif {$i >= [llength $domain]} {
        set ${bundleName}(value) $value
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
    upvar #0 ${appName}_bundle_${bun}(deplocals) deps
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

    # Update any dependant local variables, if they exist.
    if { [info exists deps] } {
        foreach dep $deps {
            upvar #0 $dep dep_bun
            set dep_bun(value) $bunv
        }
    }

    global ${appName}_time
    incr ${appName}_time
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

# This function checks if the current simplex has already been explored before.
proc check_for_convergence {appName} {
    # I followed the implementation of MATLAB fminsearch.m for term_x and term_f tests.
    # But term_x and term_f tests are ignored for now because we now mostly work on discrete integer domains.
    # In discrete domains, history test works better than tolerance tests (term_x and term_f).
    # For continuous domains, we can set tolerance values as small as we want.
    # But for discrete domains, it is hard to decide tolerance values
    # If it is too small, the algorithm will not be sometimes converged.
    # And if it is too large, the algorithm will provide a bad non-optimal solution.
    set converged_x 0
    set converged_f 0
    if {0} {
        # 1) term_x: domain convergence test, simplex is sufficiently small?
        set converged_x 0
        set tol_x 1
        set points [arraytolist ${appName}_simplex_points]
        set max_x -1000
        for {set i 0} {$i < [llength $points]} {incr i} {
            set x_i [lindex $points $i]
            for {set j [expr $i + 1]} {$j < [llength $points]} {incr j} {
                set x_j [lindex $points $j]
                # compute ||x_i - x_j||_\infty
                set max_k -1000
                for {set k 0} {$k < [llength $x_i]} {incr k} {
                    set a [expr abs([lindex $x_i $k] - [lindex $x_j $k])]
                    #puts "a = ${a}"
                    if {$a > $max_k} {set max_k $a}
                }
                #puts "max_k = ${max_k}"
                if {$max_k > $max_x} {set max_x $max_k}
            }
        }
        if {$max_x <= $tol_x} {set converged_x 1}
        set converged_x 0

        # 2) term_f: function value convergence test, simplex is sufficiently flat?
        set converged_f 0
        set tol_f 10000000
        upvar #0 ${appName}_simplex_performance fvalues
        puts "fvalues = $fvalues"
        set max_f -1000
        for {set i 0} {$i < [llength $fvalues]} {incr i} {
            set f_i [lindex $fvalues $i]
            for {set j [expr $i + 1]} {$j < [llength $fvalues]} {incr j} {
                set f_j [lindex $fvalues $j]
                # compute |f_i - f_j|
                set a [expr abs($f_i - $f_j)]
                if {$a > $max_f} {set max_f $a}
            }
        }
        if {$max_f <= $tol_f} {set converged_f 1}
        set converged_f 0
    }
    # end of if 0

    # 3) num steps
    set converged_time 0
    set points [arraytolist ${appName}_simplex_points]
    set num_variables [llength [lindex $points 0]]
    #puts "num_variables = $num_variables"
    set tol_time [expr 200 * $num_variables]
    #puts "tol_time = $tol_time"
    upvar #0 ${appName}_simplex_time simplex_time
    if {$simplex_time >= $tol_time} {set converged_time 1}

    # 4) history
    set converged_history 0
    # 4a) set the current simplex
    set curr_simplex [arraytolist ${appName}_simplex_points]
    # sort curr_simplex 0 - numVar
    lsort $curr_simplex
    upvar #0 ${appName}_simplex_history simplex_history
    # 4b) history existence check
    set ii 0
    foreach i_simplex $simplex_history {
        puts "simplex_history $ii $i_simplex"
        if {$i_simplex == $curr_simplex} {
            puts "simplex_history MATCH $ii $curr_simplex"
            set converged_history 1
            break
        }
        incr ii
    }
    # 4c) add curr_simplex to history
    lappend simplex_history $curr_simplex
    if {[llength $simplex_history] > 5} {
        set simplex_history [lreplace $simplex_history 0 0]
    }

    upvar #0 ${appName}_search_done simplex_converged
    if {($converged_x == 1 && $converged_f == 1) || $converged_time == 1 || $converged_history == 1} {
        set simplex_converged 1
    } else {
        set simplex_converged 0
    }
    #set simplex_converged 0
    puts "${appName}_search_done = $simplex_converged"
    #puts "converged_x = $converged_x  tol_x = $tol_x  max_x = $max_x"
    #puts "converged_f = $converged_f  tol_f = $tol_f  max_f = $max_f"
    puts "converged_time = $converged_time  tol_time = $tol_time"
    puts "converged_history = $converged_history"
}

proc arraytolist {a} {
    upvar #0 $a arr
    set l {}
    foreach tag [lsort [array names arr]] {
        lappend l $arr($tag)
    }
    return $l
}
