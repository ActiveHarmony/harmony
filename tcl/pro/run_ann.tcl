#!/usr/bin/tclsh
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
load ./nearest_neighbor.so

set filename [make_string "test_2_vars"]
set tree [construct_tree 2 5 $filename]

puts "querying 51 63"
set str [make_string "51 63"]
set point [make_point $str]
#puts  -nonewline "Is this a valid point --> "
#set valid [is_valid $tree $point]
#puts $valid
puts [get_neighbor $tree $point]

#puts [getNeighbor $tree]
#
#puts "querying 100 34 100"
#set str [make_string "100 34 100"]
#set point [make_point $str]
#puts  -nonewline "Is this a valid point --> "
#set valid [is_valid $tree $point]
#puts $valid
#puts [get_neighbor $tree $point]
#
#puts ""
#puts "querying 100 1 100"
#set str [make_string "100 1 100"]
#set point [make_point $str]
#puts  -nonewline "Is this a valid point --> "
#set valid [is_valid $tree $point]
#puts $valid
#puts [get_neighbor $tree $point]
#
#puts ""
#puts "querying 100 10 246"
#set str [make_string "100 10 246"]
#set point [make_point $str]
#puts  -nonewline "Is this a valid point --> "
#set valid [is_valid $tree $point]
#puts $valid
#puts [get_neighbor $tree $point]
#
#puts ""
#puts "querying 264 10 100"
#set str [make_string "264 10 100"]
#set point [make_point $str]
#puts  -nonewline "Is this a valid point --> "
#set valid [is_valid $tree $point]
#puts $valid
#puts [get_neighbor $tree $point]
#
#puts ""
#puts "querying 100 10 10"
#set str [make_string "100 10 10"]
#set point [make_point $str]
#puts  -nonewline "Is this a valid point --> "
#set valid [is_valid $tree $point]
#puts $valid
#
#
#set temp [get_neighbor $tree $point]
#puts -nonewline "temp:: "
#puts $temp
#puts -nonewline "Index 0:: " 
#puts [lindex [expr $temp] 0]
#puts -nonewline "Index 1:: " 
#puts [lindex [expr $temp] 1]
#
