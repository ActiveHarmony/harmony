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
