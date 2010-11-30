#!/usr/bin/tclsh

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
