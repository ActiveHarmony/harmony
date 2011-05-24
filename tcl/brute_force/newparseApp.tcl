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

# we are going to have the following global variables:
# 
# AppName_predGoodness - array containing the following
#			fields: 
#                       min - the minimum "acceptable" value
#                       max - the maximum "acceptable" value
#                       we say "acceptable" because this are values that will 
#                       eventually get changed by the server and are not 
#                       communicated to the client. 
#
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

  #define global variable AppName_obsGoodness

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
	    puts "Printing Goodness : $isglobal ; $aName"
	    #parr ${aName}_obsGoodness
	} else {
	    upvar #0 ${aName}_obsGoodness(deplocals) deplocals
	    lappend deplocals ${appName}_obsGoodness
	    upvar #0 ${aName}_obsGoodness(queue_${appName}_obsGoodness) qGood
	    set qGood {}
	    puts "Printing Goodness : $isglobal ; $aName"
	    #parr ${aName}_obsGoodness
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

    #simplex_init $appName
    brute_force_init $appName
}


proc writeTableHeadToDisk {appName} {
    
    set fname [open "./${appName}.log" {WRONLY APPEND CREAT} 0600]

    global ${appName}_fname
    set ${appName}_fname $fname

    upvar #0 ${appName}_bundles bundles

    puts $appName

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
    

    puts "Entered update obsGoondess $appName ; $value" 
    #parr ${appName}_obsGoodness
    puts $isglobal
    puts "isglobal = ${appName}_obsGoodness(islgobal)"

    if {$isglobal>=0} {
	set ${appName}_obsGoodness(value) $value
	
	incr time
	
	lappend coords $time [canvasyCoord $appName $value]
	
	drawharmonyObsGoodness $appName 
	
	writeBundlesToDisk $appName
    }

 # first we analyze the case when performance goes to global
 # in this case we only send the value and the timestamp to the 
 # global instance

    if {$isglobal==1} {
	set aName [string range $appName 0 [expr [string last "_" $appName]-1]]     
	# send the values to the global instance
	puts "Sending the value $value to the global instance $aName"
	updateObsGoodness $aName $value $timestamp $appName
    } elseif {$isglobal==0} {

	# this describes the case when the performance goes local
	# here we must make sure that what we got reflects 
	# the new values of the parameters.
	# For this we introduce the timestamps. 
	
	global ${appName}_simplex_time
	upvar #0 ${appName}_simplex_time stime
	
	puts "Timestamp : $timestamp ; Stime : $stime"
	
	if {$stime < $timestamp} {
	    puts "Warning : timestamp has changed !"
	} 
	if {$stime<=$timestamp} {
	    puts "Entering simplex method!"
	    #simplex_method $appName $value
	    #puts "Exit simplex method!"
	    brute_force $appName
	} else {
	    # performance does not reflect changes in params
	}
    } else {
	puts "ObsGoodness is Global"
	# this is the case when the performance is global
	# first we have to add the new value to its queue
	if {[llength $args]} {
	    set depend [lindex $args 0]
	    upvar #0 ${appName}_obsGoodness(queue_${depend}_obsGoodness) q_${depend}
	    lappend q_${depend} $value
	    #parr ${appName}_obsGoodness
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
	    # here we want to collect at least ... x (x=2) performance values
	    # from each instance that is declared global
	    # if we want to increase the number of samples we can do it
#	    puts $depend
#	    parr ${appName}_obsGoodness
	    if {[llength $qdepend]>=1} { incr i }
#	    puts $qdepend
	}
	puts "Counted : $i"
	if {$i == [llength $deplocals]} {
	    # means we got samples from everybody, so we can just go ahead
	    # and agregate individually using ${appName}_agregation_local
	    set temp {}
	    foreach depend $deplocals {
		upvar #0 ${appName}_obsGoodness(queue_${depend}) qdepend
		if {[info procs ${appName}_aggregation_local]!={}} {
		    lappend temp [${appName}_aggregation_local $qdepend]
		} else {
		    lappend temp [general_aggregation $qdepend]
		}
	    }
	    
	    #next we call the global agregation function
	    if {[info procs ${appName}_aggregation]!={}} {
		set nvalue [${appName}_aggregation $temp]
	    } else {
		set nvalue [general_aggregation $temp]
	    }

	    #parr ${appName}_obsGoodness

	    # now clean everything behind us
	    foreach depend $deplocals {
		upvar #0 ${appName}_obsGoodness(queue_${depend}) qdepend
		set qdepend {}
	    }

	    #parr ${appName}_obsGoodness

	    #now we just go ahead and do with this value whatever we usually do
	    set ${appName}_obsGoodness(value) $nvalue
	    
	    incr time
	    
	    lappend coords $time [canvasyCoord $appName $nvalue]
	    
	    drawharmonyObsGoodness $appName 
	    
	    writeBundlesToDisk $appName

	    puts "Entering simplex method!"
	    #simplex_method $appName $nvalue
	    #puts "Exit simplex method!"	    
	    brute_force $appName
	}
    }
}


proc general_aggregation {values} {
    set sum 0
    set i 0
    foreach value $values {
	incr sum $value
	incr i
    }
    return [expr $sum/$i];
}

proc writeBundlesToDisk {appName} {
   
    set fname [open "./${appName}.log" {WRONLY APPEND CREAT} 0600]

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

    puts "Enter cleanup!"

    upvar #0 ${appName}_bundles bundles

    foreach bun $bundles {
	upvar #0 ${appName}_bundle_${bun}(isglobal) isglobal
	set isglobal 0
#	puts "Appname = $appName ; Bundle = $bun"
	update_bundle_isglobal $bun $appName
#	parr ${appName}_bundle_${bun}
    }

    upvar #0 ${appName}_obsGoodness(isglobal) isglobal
    set isglobal 0
    update_perf_isglobal $appName
    
#    parr ${appName}_obsGoodness

}
