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

# we take care of the projection here.


proc project_points { points appName } {
    upvar #0 projection_method method_num
    set projected_points {}
    if { $method_num == 1 } {
        return [projection_using_ann $points]
    } elseif { $method_num == 2 } {
        foreach point $points {
            lappend projected_points [projection_onto_boundary $appName $point]
        }
    }
    return $projected_points
}

proc projection_using_ann { points } {
    set return_points $points
    set str_points ""
    set index 0
    foreach point $points {
        foreach dim_value $point {
            append str_points $dim_value " "
        }
        if { $index != [expr [llength $points]-1] } {
            # need a trailing ':'
            append str_points ": "
        }
        incr index 1
    }
    
    set return_point [expr [do_projection_entire_simplex $str_points]]
    return $return_point
}

proc projection_using_ann_deprecated { point tree } {    
    set return_point $point
    # preliminaries : convert list to string
    #  make a string object
    set temp_point [list_to_string $point]
    set temp_obj [make_string $temp_point]
    # make a query point
    set query_point [make_point $temp_obj]
    # check if the point is valid
    set validity [is_valid $tree $query_point]
    if { $validity != 1} {
        # number of neighbor argument is set to 2 -- Self matches in ANN are
        #  allowed. 
        set return_point [expr [get_neighbor $tree $query_point 2]]
    }
    return $return_point
}

# project each coordinate component to its boundary
#   also need to make sure that the points follow the coarse limits
# if you get "cant find "minimum" error, check the init_point parameter
#  in the pro_init file
proc projection_onto_boundary { appName point } {
    upvar #0 ${appName}_bundles bundles
    set return_point {}
    foreach dim_value $point bun $bundles {
        upvar  #0 ${appName}_bundle_${bun}(domain) domain
        upvar #0 ${appName}_bundle_${bun}(minv) minimum
        upvar #0 ${appName}_bundle_${bun}(maxv) maximum
        lappend return_point [projection_middle $bun $dim_value $domain]
    }
    return $return_point
}

proc projection_middle {bundleName point domain} {
    set return_value 0
    # find the value in the domain that is closest to the "value" passed
    for {set i 0} {($i < [llength $domain]) && ([lindex $domain $i] < $point)} {incr i} { }
    if {$i==0} {
        set return_value [lindex $domain $i]
    } elseif {$i >= [llength $domain]} {
        set return_value [lindex $domain end]
    } else {
        # set the new value as close as possible.
        # look at the values that are one step lower than $i
        # use the value which is smaller in difference
        set dif1 [expr [lindex $domain $i]-$point]
        set dif2 [expr $point - [lindex $domain [expr $i-1]]]

        if {$dif1 > $dif2} {
            set return_value [lindex $domain [expr $i-1]]
        } else {
            set return_value [lindex $domain $i]
        }
    }
    return $return_value
}
