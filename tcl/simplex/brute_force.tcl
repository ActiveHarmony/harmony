proc brute_force_init {appName} {
    global ${appName}_brute_force_bundles
    upvar #0 ${appName}_bundles bundles

    set ${appName}_brute_force_bundles [lsort $bundles]
#set ${appName}_brute_force_bundles $bundles
#    set ${appName}_brute_force_bundles {tileSize lowW maxReads}
    upvar #0 ${appName}_brute_force_bundles brutefb

    foreach bun $brutefb {
	upvar #0 ${appName}_bundle_${bun}(value) bunv
	upvar \#0 ${appName}_bundle_${bun}(minv) minv
	upvar #0 ${appName}_bundle_${bun} $bun
	
	puts $bunv
	puts $minv

	set bunv $minv
#	redraw_dependencies $bun $appName 0 0
    }
}


proc brute_force {appName} {

    brute_force_modify $appName 0

}

proc brute_force_modify {appName param} {
    upvar #0 ${appName}_brute_force_bundles bundles
    set bun [lindex $bundles $param]
    
    puts "Brute force Modify: $bun"

    upvar #0 ${appName}_brute_force_values brutef

    upvar #0 ${appName}_bundle_${bun}(value) bunv
    upvar \#0 ${appName}_bundle_${bun}(minv) minv
    upvar \#0 ${appName}_bundle_${bun}(maxv) maxv
    upvar #0 ${appName}_bundle_${bun}(stepv) stepv
    upvar #0 ${appName}_bundle_${bun} $bun

    set overflow 0

    if {$bunv == $maxv} {
	incr overflow
    } else {
	incr bunv $stepv
	redraw_dependencies $bun $appName 0 0
    }

    puts "Param value: $bunv"
    if {$overflow > 0} {
	puts "Reached max"
	brute_force_modify $appName [expr $param+1]
	set bunv $minv
	redraw_dependencies $bun $appName 0 0
    }

}
