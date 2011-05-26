# we are going to have the following global variables:

# AppName_predGoodness - array containing the following
# 		fields:

#                       min - the minimum "acceptable" value
#                       max - the maximum "acceptable" value
#                       we say "acceptable" because this are values that will
#                       eventually get changed by the server and are not
#                       communicated to the client.

# AppName_obsGoodness - array containing the following
#			fields:
#                       min - the minimum "acceptable" value
#                       max - the maximum "acceptable" value
#                       we say "acceptable" because this are values that will
#                       eventually get changed by the server and are not
#                       communicated to the client.
#                       isglobal
#                       time
#                       value
#                       coordinates

proc predGoodness {min max appName} {
  #define global variable AppName_predGoodness
    global ${appName}_predGoodness
    set ${appName}_predGoodness(min) $min
    set ${appName}_predGoodness(max) $max
}

proc obsGoodness {min max args} {
    set appName [lindex $args end]

    global ${appName}_obsGoodness
    global ${appName}_obsGoodness(isglobal)

    if {[lindex $args 0]=="global"} {
	set ${appName}_obsGoodness(isglobal) 1
	# now we also have to create it in the global window
	set aName [string range $appName 0 [expr [string last "_" $appName]-1]]
	if {![info exists ${aName}_obsGoodness]} {
	    predGoodness [expr ($min+$max-1)/2] [expr ($min+$max+1)/2] $aName
	    obsGoodness $min $max $aName
	    upvar #0 ${aName}_obsGoodness(isglobal) isglobal
	    set isglobal -1
	    upvar #0 ${aName}_obsGoodness(deplocals) deplocals
	    lappend deplocals ${appName}_obsGoodness
	    upvar #0 ${aName}_obsGoodness(queue_${appName}_obsGoodness) qGood
	    set qGood {}
	} else {
	    upvar #0 ${aName}_obsGoodness(deplocals) deplocals
	    lappend deplocals ${appName}_obsGoodness
	    upvar #0 ${aName}_obsGoodness(queue_${appName}_obsGoodness) qGood
	    set qGood {}
	}
    } else {
	set ${appName}_obsGoodness(isglobal) 0
    }

    set ${appName}_obsGoodness(min) $min
    set ${appName}_obsGoodness(max) $max
    set ${appName}_obsGoodness(time) 0
    set ${appName}_obsGoodness(value) -1

    lappend ${appName}_obsGoodness(coordinates) 0 0 0 0

    writeTableHeadToDisk $appName

    # we are using parallel simplex
    parallel_simplex_init $appName
}

proc writeTableHeadToDisk {appName} {
    set fname [open "${appName}.log" {WRONLY APPEND CREAT} 0600]
    global ${appName}_fname
    set ${appName}_fname $fname
    upvar #0 ${appName}_bundles bundles
    append bundle_list "time" " ; "
    foreach bun $bundles {
	append bundle_list $bun " ; "
    }
    append bundle_list "obsGoodness"
    puts $fname $bundle_list
    close $fname
}


proc updateObsGoodness {appName value timestamp args} {

    global ${appName}_obsGoodness
    global ${appName}_obsGoodness(value)
    global ${appName}_obsGoodness(isglobal)
    upvar #0 ${appName}_obsGoodness(time) time
    upvar #0 ${appName}_obsGoodness(coordinates) coords
    upvar #0 ${appName}_obsGoodness(value) Gvalue
    upvar #0 ${appName}_obsGoodness(isglobal) isglobal
    upvar #0 draw_har_windows draw_windows
    upvar #0 ${appName}_performance perf_this
    set perf_this $value

    puts "check point 1"

    if {$isglobal>=0} {
        set ${appName}_obsGoodness(value) $value
	puts "got the value from obsgoodness"
        incr time
	puts "appending lappend"
        lappend coords $time [canvasyCoord $appName $value]
	puts "done appending"
        #puts "coords $coords"
        if { $draw_windows == 1 } {
	    puts "drawing"
            drawharmonyObsGoodness $appName
	    puts "done drawing"
        }
	puts "writing to a file"
        writeBundlesToDisk $appName
	puts "done writing"
    }
     puts "check point 2"
    # first we analyze the case when performance goes to global
    # in this case we only send the value and the timestamp to the
    # global instance

    if {$isglobal==1} {
	
        set aName [string range $appName 0 [expr [string last "_" $appName]-1]]
        # send the values to the global instance
        updateObsGoodness $aName $value $timestamp $appName
	 puts "check point 3"
    } elseif {$isglobal==0} {
        # this describes the case when the performance goes local
        # here we must make sure that what we got reflects
        # the new values of the parameters.
        # For this we introduce the timestamps.

        global ${appName}_simplex_time
        upvar #0 ${appName}_simplex_time stime

        if {$stime < $timestamp} {
            puts "Warning : timestamp has changed !"
        }
        if {$stime<=$timestamp} {
            pro $appName
        } else {
            # performance does not reflect changes in params
        }

    } else {
        # this is the case when the performance is global
        # first we have to add the new value to its queue
        if {[llength $args]} {
            set depend [lindex $args 0]
            upvar #0 ${appName}_obsGoodness(queue_${depend}_obsGoodness) q_${depend}
            lappend q_${depend} $value
	    puts -nonewline "q_{depend} array : "
	    parr q_${depend}
        } else {
            return
        }

        # next we have to check if we have received a value from all the
        # currently declared global instances.
        upvar #0 ${appName}_obsGoodness(deplocals) deplocals
        set i 0
        foreach depend $deplocals {
            global ${appName}_obsGoodness(queue_${depend})
            upvar #0 ${appName}_obsGoodness(queue_${depend}) qdepend
            # here we want to collect at least ... x (x=1) performance values
            # from each instance that is declared global
            # if we want to increase the number of samples we can do it
            # puts $depend
            # parr ${appName}_obsGoodness
            if {[llength $qdepend]==1} {
                incr i
                upvar #0 ${args}_next_iteration next
                set next 0
            }
        }
        if {$i == [llength $deplocals]} {

	    puts "got samples from all the clients: ie from all $i clients"

            # means we got samples from everybody, so we can just go ahead
            # and get the best perf out of all to set the global performance
            # for the global performance always get the current simplex low:
     	    #  the value displayed will lag behind by one timestep.

            # now clean everything behind us
            foreach depend $deplocals {
                upvar #0 ${appName}_obsGoodness(queue_${depend}) qdepend
                set qdepend {}
            }

            #now we just go ahead and do with this value whatever we usually do
            #set ${appName}_obsGoodness(value) $current_low

            incr time

            # check if best performance so far is defined already?
            upvar #0 best_coordinate_so_far disp_low_point
            upvar #0 best_perf_so_far best_perf_so_far
            lappend coords $time [canvasyCoord $appName $best_perf_so_far]

            upvar #0 draw_har_windows draw_windows
            if { $draw_windows == 1 } {
                drawharmonyObsGoodness $appName
            }
            writeBundlesToDisk $appName
	    puts "calling pro"
            pro $appName
	    puts "pro exited normally"
        }
    }
}


proc general_aggregation {values} {
    set maximum 0
    set i 0

    foreach value $values {
        if {$i==0} {
            set maximum $value
        } else {
            if {$maximum < $value} {
                set maximum $value
            }
        }
	incr i
    }
    return [expr $maximum];
}

proc writeBundlesToDisk {appName} {


    set fname [open "${appName}.log" {WRONLY APPEND CREAT} 0600]

    global ${appName}_fname
    set ${appName}_fname $fname

    upvar #0 ${appName}_bundles bundles
    upvar #0 ${appName}_obsGoodness(time) Gtime
    upvar #0 ${appName}_obsGoodness(value) Gvalue

    append bundle_list $Gtime " ; "
    foreach bun $bundles {
	upvar #0 ${appName}_bundle_${bun}(value) $bun
	append bundle_list [expr $$bun] " ; "
    }

    append bundle_list $Gvalue

    puts $fname $bundle_list

    close $fname

}

# this function cleans up after a harmony application exit
# it has to set all the bundles to non-global, as well as the performance
# function to non-global
#
proc harmony_cleanup {appName} {

    upvar #0 ${appName}_bundles bundles

    foreach bun $bundles {
        upvar #0 ${appName}_bundle_${bun}(isglobal) isglobal
        set isglobal 0
        update_bundle_isglobal $bun $appName
    }

    upvar #0 ${appName}_obsGoodness(isglobal) isglobal
    set isglobal 0
    update_perf_isglobal $appName
}
