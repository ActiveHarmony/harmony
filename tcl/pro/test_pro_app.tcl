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
source logo.tk
source parseApp_version_8.tcl
source drawApp.tk
source sched.tcl
source proparseApp_version_8.tcl
source newdrawApp.tk
source pro_version_8.tcl
source pro_init.tcl
source initial_simplex_construction.tcl
source ./random_uniform.tcl
load ./nearest_neighbor.so
load ./round.so
source ./utilities.tcl
source ./matrix.tcl
source ./pro_transformations.tcl
source ./projection.tcl
source combine.tcl
#source simplex.tcl
harmonyApp OfflineT {
        { harmonyBundle TI {
            int {2 248 1 20} global
        }
        }
        { harmonyBundle TJ {
            int {1 62 1 2} global
        }
        }
        { harmonyBundle TK {
            int {2 248 1 20} global
        }
        }
        { obsGoodness 1000 8000 global }
        { predGoodness 1000 8000}
    } 1
harmonyApp OfflineT {
        { harmonyBundle TI {
            int {2 248 1 20} global
        }
        }
        { harmonyBundle TJ {
            int {1 62 1 2} global
        }
        }
        { harmonyBundle TK {
            int {2 248 1 20} global
        }
        }
        { obsGoodness 1000 8000 global }
        { predGoodness 1000 8000}
    } 2

harmonyApp OfflineT {
        { harmonyBundle TI {
            int {2 248 1 20} global
        }
        }
        { harmonyBundle TJ {
            int {1 62 1 2} global
        }
        }
        { harmonyBundle TK {
            int {2 248 1 20} global
        }
        }
        { obsGoodness 1000 8000 global }
        { predGoodness 1000 8000}
    } 3
harmonyApp OfflineT {
        { harmonyBundle TI {
            int {2 248 1 20} global
        }
        }
        { harmonyBundle TJ {
            int {2 62 1 2} global
        }
        }
        { harmonyBundle TK {
            int {2 248 1 20} global
        }
        }
        { obsGoodness 1000 8000 global }
        { predGoodness 1000 8000}
    } 4
harmonyApp OfflineT {
        { harmonyBundle TI {
            int {2 248 1 20} global
        }
        }
        { harmonyBundle TJ {
            int {1 62 1 2} global
        }
        }
        { harmonyBundle TK {
            int {2 248 1 20} global
        }
        }
        { obsGoodness 1000 8000 global }
        { predGoodness 1000 8000}
    } 5
harmonyApp OfflineT {
        { harmonyBundle TI {
            int {2 248 1 20} global
        }
        }
        { harmonyBundle TJ {
            int {1 62 1 2} global
        }
        }
        { harmonyBundle TK {
            int {2 248 1 20} global
        }
        }
        { obsGoodness 1000 8000 global }
        { predGoodness 1000 8000}
    } 6
harmonyApp OfflineT {
        { harmonyBundle TI {
            int {2 248 1 20} global
        }
        }
        { harmonyBundle TJ {
            int {1 62 1 2} global
        }
        }
        { harmonyBundle TK {
            int {2 248 1 20} global
        }
        }
        { obsGoodness 1000 8000 global }
        { predGoodness 1000 8000}
    } 7
harmonyApp OfflineT {
        { harmonyBundle TI {
            int {2 248 1 20} global
        }
        }
        { harmonyBundle TJ {
            int {1 62 1 2} global
        }
        }
        { harmonyBundle TK {
            int {2 248 1 20} global
        }
        }
        { obsGoodness 1000 8000 global }
        { predGoodness 1000 8000}
    } 8

harmonyApp OfflineT {
        { harmonyBundle TI {
            int {2 248 1 20} global
        }
        }
        { harmonyBundle TJ {
            int {1 62 1 2} global
        }
        }
        { harmonyBundle TK {
            int {2 248 1 20} global
        }
        }
        { obsGoodness 1000 8000 global }
        { predGoodness 1000 8000}
    } 9
harmonyApp OfflineT {
        { harmonyBundle TI {
            int {2 248 1 20} global
        }
        }
        { harmonyBundle TJ {
            int {2 62 1 2} global
        }
        }
        { harmonyBundle TK {
            int {2 248 1 20} global
        }
        }
        { obsGoodness 1000 8000 global }
        { predGoodness 1000 8000}
    } 10
harmonyApp OfflineT {
        { harmonyBundle TI {
            int {2 248 1 20} global
        }
        }
        { harmonyBundle TJ {
            int {1 62 1 2} global
        }
        }
        { harmonyBundle TK {
            int {2 248 1 20} global
        }
        }
        { obsGoodness 1000 8000 global }
        { predGoodness 1000 8000}
    } 11
harmonyApp OfflineT {
        { harmonyBundle TI {
            int {2 248 1 20} global
        }
        }
        { harmonyBundle TJ {
            int {1 62 1 2} global
        }
        }
        { harmonyBundle TK {
            int {2 248 1 20} global
        }
        }
        { obsGoodness 1000 8000 global }
        { predGoodness 1000 8000}
    } 12

    
#    puts [get_test_configuration OfflineT_2]


# Applicaton part:
#simplex_init $appName
set fname [open "update_goodness.log" {WRONLY APPEND CREAT} 0600]
puts $fname "------------------"
set str ""

for {set i 0} {$i < 30} {incr i} {
    set appName "OfflineT"
    set lsperf {}
    #if {$i == 0} {
    #    pro $appName
    #}
    upvar #0 ${appName}_procs procs
    
    foreach proc $procs {
        upvar #0 ${proc}_performance perf
        upvar #0 ${proc}_bundle_TI(value) x
        upvar #0 ${proc}_bundle_TJ(value) y
        upvar #0 ${proc}_bundle_TK(value) z
        #upvar #0 ${proc}_bundle_a(value) a
        #upvar #0 ${proc}_bundle_b(value) b
        #upvar #0 ${proc}_bundle_c(value) c
        #set perf [expr $x + $y - $z + [random_uniform 2 40 1]]
        set bool__ [expr ($y*2)*(4+(($z-1)*4)) ]
        if { $bool__ > 4096 } {
                set perf 100000
        } else {
                set perf [expr $x - $y - $z]
        }
        append str "updateObsGoodness " $proc " " $perf " 0\n"
        puts $fname $str
        set str ""
        updateObsGoodness $proc $perf 0
        lappend lsperf $perf
        #puts "tesT"
    }
    puts $fname ""
    set str ""
    #pro $appName
}
close $fname

#updateObsGoodness OfflineT_5 27986 -1
##updateObsGoodness OfflineT_8 22143 -1
##updateObsGoodness OfflineT_7 26873 -1
#updateObsGoodness OfflineT_6 41298 -1
##updateObsGoodness OfflineT_9 29254 -1
#updateObsGoodness OfflineT_1 21969 -1
#updateObsGoodness OfflineT_2 25922 -1
##updateObsGoodness OfflineT_10 24380 -1
#updateObsGoodness OfflineT_3 66436 -1
#updateObsGoodness OfflineT_4 60694 -1
#
#updateObsGoodness OfflineT_1 21748 -1
#updateObsGoodness OfflineT_6 65251 -1
#updateObsGoodness OfflineT_2 64147 -1
##updateObsGoodness OfflineT_10 50912 -1
##updateObsGoodness OfflineT_9 52226 -1
#updateObsGoodness OfflineT_5 84360 -1
##updateObsGoodness OfflineT_7 92101 -1
#updateObsGoodness OfflineT_4 102471 -1
##updateObsGoodness OfflineT_8 101186 -1
#updateObsGoodness OfflineT_3 104025 -1
#
#updateObsGoodness OfflineT_3 22234 -1
##updateObsGoodness OfflineT_9 22190 -1
#updateObsGoodness OfflineT_2 21974 -1
##updateObsGoodness OfflineT_8 22258 -1
##updateObsGoodness OfflineT_7 22288 -1
#updateObsGoodness OfflineT_6 22236 -1
##updateObsGoodness OfflineT_10 21813 -1
#updateObsGoodness OfflineT_1 21855 -1
#updateObsGoodness OfflineT_5 21937 -1
#updateObsGoodness OfflineT_4 22209 -1
#
#updateObsGoodness OfflineT_1 21771 -1
#updateObsGoodness OfflineT_2 54002 -1
#updateObsGoodness OfflineT_6 56460 -1
##updateObsGoodness OfflineT_10 48112 -1
##updateObsGoodness OfflineT_9 51334 -1
##updateObsGoodness OfflineT_7 85821 -1
#updateObsGoodness OfflineT_4 99685 -1
#updateObsGoodness OfflineT_3 104096 -1
##updateObsGoodness OfflineT_8 130027 -1
#updateObsGoodness OfflineT_5 131111 -1
#
##updateObsGoodness OfflineT_8 55386 -1
#updateObsGoodness OfflineT_5 23861 -1
#updateObsGoodness OfflineT_4 27547 -1
#updateObsGoodness OfflineT_3 28890 -1
#updateObsGoodness OfflineT_1 22251 -1
##updateObsGoodness OfflineT_7 23221 -1
#updateObsGoodness OfflineT_2 21824 -1
#updateObsGoodness OfflineT_6 22277 -1
##updateObsGoodness OfflineT_9 24395 -1
##updateObsGoodness OfflineT_10 52254 -1
#
##updateObsGoodness OfflineT_8 24577 -1
##updateObsGoodness OfflineT_7 21643 -1
#updateObsGoodness OfflineT_5 28053 -1
#updateObsGoodness OfflineT_3 21721 -1
#updateObsGoodness OfflineT_4 22025 -1
#updateObsGoodness OfflineT_1 21844 -1
##updateObsGoodness OfflineT_10 23097 -1
##updateObsGoodness OfflineT_9 23106 -1
#updateObsGoodness OfflineT_2 23002 -1
#updateObsGoodness OfflineT_6 23651 -1
#
#updateObsGoodness OfflineT_6 22401 -1
##updateObsGoodness OfflineT_10 22170 -1
#updateObsGoodness OfflineT_1 22133 -1
##updateObsGoodness OfflineT_8 22105 -1
#updateObsGoodness OfflineT_5 22384 -1
#updateObsGoodness OfflineT_3 22372 -1
##updateObsGoodness OfflineT_7 22452 -1
#updateObsGoodness OfflineT_4 22123 -1
##updateObsGoodness OfflineT_9 22304 -1
#updateObsGoodness OfflineT_2 22394 -1
#
#updateObsGoodness OfflineT_2 23332 -1
#updateObsGoodness OfflineT_6 21597 -1
##updateObsGoodness OfflineT_9 22003 -1
#updateObsGoodness OfflineT_5 25054 -1
#updateObsGoodness OfflineT_3 21514 -1
#updateObsGoodness OfflineT_4 21585 -1
##updateObsGoodness OfflineT_7 21721 -1
##updateObsGoodness OfflineT_8 45140 -1
#updateObsGoodness OfflineT_1 76423 -1
##updateObsGoodness OfflineT_10 46454 -1
#
#updateObsGoodness OfflineT_6 21650 -1
##updateObsGoodness OfflineT_9 21892 -1
#updateObsGoodness OfflineT_3 21659 -1
##updateObsGoodness OfflineT_10 21611 -1
#updateObsGoodness OfflineT_5 21619 -1
##updateObsGoodness OfflineT_8 21939 -1
##updateObsGoodness OfflineT_7 21824 -1
#updateObsGoodness OfflineT_4 21710 -1
#updateObsGoodness OfflineT_1 21652 -1
#updateObsGoodness OfflineT_2 21857 -1
#
##updateObsGoodness OfflineT_9 21918 -1
##updateObsGoodness OfflineT_8 44761 -1
#updateObsGoodness OfflineT_5 44559 -1
##updateObsGoodness OfflineT_10 46402 -1
#updateObsGoodness OfflineT_3 21623 -1
##updateObsGoodness OfflineT_7 21577 -1
#updateObsGoodness OfflineT_4 21533 -1
#updateObsGoodness OfflineT_1 82779 -1
#updateObsGoodness OfflineT_2 25676 -1
#updateObsGoodness OfflineT_6 46522 -1
#
#updateObsGoodness OfflineT_1 22086 -1
#updateObsGoodness OfflineT_3 21520 -1
#updateObsGoodness OfflineT_6 23141 -1
#updateObsGoodness OfflineT_4 21550 -1
##updateObsGoodness OfflineT_8 25008 -1
#updateObsGoodness OfflineT_5 28249 -1
##updateObsGoodness OfflineT_7 21970 -1
##updateObsGoodness OfflineT_10 23333 -1
##updateObsGoodness OfflineT_9 23016 -1
#updateObsGoodness OfflineT_2 22913 -1
#
#updateObsGoodness OfflineT_2 21421 -1
#updateObsGoodness OfflineT_1 21381 -1
##updateObsGoodness OfflineT_7 21342 -1
##updateObsGoodness OfflineT_8 21707 -1
#updateObsGoodness OfflineT_3 21372 -1
#updateObsGoodness OfflineT_5 21298 -1
#updateObsGoodness OfflineT_6 21442 -1
##updateObsGoodness OfflineT_10 21786 -1
##updateObsGoodness OfflineT_9 21446 -1
#updateObsGoodness OfflineT_4 21346 -1
#
##updateObsGoodness OfflineT_8 45686 -1
#updateObsGoodness OfflineT_6 25446 -1
#updateObsGoodness OfflineT_4 21461 -1
##updateObsGoodness OfflineT_9 24707 -1
##updateObsGoodness OfflineT_7 21447 -1
#updateObsGoodness OfflineT_1 21224 -1
#updateObsGoodness OfflineT_3 21457 -1
#updateObsGoodness OfflineT_2 25213 -1
##updateObsGoodness OfflineT_10 25619 -1
#updateObsGoodness OfflineT_5 65049 -1
#
#updateObsGoodness OfflineT_5 10000000 -1
##updateObsGoodness OfflineT_8 10000000 -1
#updateObsGoodness OfflineT_2 22080 -1
#updateObsGoodness OfflineT_6 22089 -1
##updateObsGoodness OfflineT_9 22087 -1
##updateObsGoodness OfflineT_10 22058 -1
#updateObsGoodness OfflineT_1 21302 -1
#updateObsGoodness OfflineT_3 21229 -1
#updateObsGoodness OfflineT_4 21416 -1
##updateObsGoodness OfflineT_7 21625 -1
#
##updateObsGoodness OfflineT_7 21953 -1
#updateObsGoodness OfflineT_4 21828 -1
#updateObsGoodness OfflineT_6 23306 -1
##updateObsGoodness OfflineT_9 23602 -1
#updateObsGoodness OfflineT_3 22058 -1
#updateObsGoodness OfflineT_2 23406 -1
##updateObsGoodness OfflineT_10 23435 -1
##updateObsGoodness OfflineT_8 24384 -1
#updateObsGoodness OfflineT_1 21231 -1
#updateObsGoodness OfflineT_5 27574 -1
##
#updateObsGoodness OfflineT_1 21330 -1
#updateObsGoodness OfflineT_3 21253 -1
#updateObsGoodness OfflineT_5 21093 -1
#updateObsGoodness OfflineT_6 21248 -1
##updateObsGoodness OfflineT_10 21253 -1
##updateObsGoodness OfflineT_9 21124 -1
##updateObsGoodness OfflineT_8 21319 -1
##updateObsGoodness OfflineT_7 21410 -1
#updateObsGoodness OfflineT_4 21191 -1
#updateObsGoodness OfflineT_2 21214 -1
##
#updateObsGoodness OfflineT_5 21066 -1
#updateObsGoodness OfflineT_4 21155 -1
#updateObsGoodness OfflineT_1 21104 -1
##updateObsGoodness OfflineT_8 21280 -1
#updateObsGoodness OfflineT_6 20999 -1
##updateObsGoodness OfflineT_10 21074 -1
##updateObsGoodness OfflineT_7 21012 -1
##updateObsGoodness OfflineT_9 21351 -1
#updateObsGoodness OfflineT_3 21257 -1
#updateObsGoodness OfflineT_2 21359 -1
##
#updateObsGoodness OfflineT_2 21144 -1
##updateObsGoodness OfflineT_9 21051 -1
#updateObsGoodness OfflineT_4 21112 -1
##updateObsGoodness OfflineT_10 21160 -1
##updateObsGoodness OfflineT_7 21038 -1
##updateObsGoodness OfflineT_8 21075 -1
#updateObsGoodness OfflineT_6 20979 -1
#updateObsGoodness OfflineT_5 21224 -1
#updateObsGoodness OfflineT_1 21277 -1
#updateObsGoodness OfflineT_3 21054 -1
##
#updateObsGoodness OfflineT_3 21335 -1
##updateObsGoodness OfflineT_8 21036 -1
#updateObsGoodness OfflineT_4 21087 -1
#updateObsGoodness OfflineT_2 21138 -1
##updateObsGoodness OfflineT_9 21123 -1
#updateObsGoodness OfflineT_6 21110 -1
##updateObsGoodness OfflineT_7 21091 -1
##updateObsGoodness OfflineT_10 20888 -1
#updateObsGoodness OfflineT_1 21378 -1
#updateObsGoodness OfflineT_5 21154 -1
##
##updateObsGoodness OfflineT_10 21103 -1
#updateObsGoodness OfflineT_2 21080 -1
#updateObsGoodness OfflineT_3 21353 -1
#updateObsGoodness OfflineT_5 21446 -1
#updateObsGoodness OfflineT_4 21064 -1
##updateObsGoodness OfflineT_9 21040 -1
#updateObsGoodness OfflineT_1 21148 -1
##updateObsGoodness OfflineT_8 20903 -1
##updateObsGoodness OfflineT_7 21341 -1
#updateObsGoodness OfflineT_6 21343 -1
##
#updateObsGoodness OfflineT_3 21003 -1
#updateObsGoodness OfflineT_4 21366 -1
##updateObsGoodness OfflineT_10 20968 -1
#updateObsGoodness OfflineT_6 21149 -1
#updateObsGoodness OfflineT_5 21066 -1
#updateObsGoodness OfflineT_2 21020 -1
##updateObsGoodness OfflineT_8 20981 -1
##updateObsGoodness OfflineT_9 21257 -1
#updateObsGoodness OfflineT_1 20680 -1
##updateObsGoodness OfflineT_7 21302 -1
##
##updateObsGoodness OfflineT_7 20959 -1
#updateObsGoodness OfflineT_3 20884 -1
##updateObsGoodness OfflineT_9 20881 -1
#updateObsGoodness OfflineT_6 20819 -1
#updateObsGoodness OfflineT_4 20699 -1
#updateObsGoodness OfflineT_5 20833 -1
##updateObsGoodness OfflineT_10 20959 -1
##updateObsGoodness OfflineT_8 20960 -1
#updateObsGoodness OfflineT_1 20875 -1
#updateObsGoodness OfflineT_2 21041 -1
##
#updateObsGoodness OfflineT_2 21489 -1
##updateObsGoodness OfflineT_10 21063 -1
#updateObsGoodness OfflineT_3 21273 -1
##updateObsGoodness OfflineT_7 21359 -1
#updateObsGoodness OfflineT_1 20730 -1
#updateObsGoodness OfflineT_6 21012 -1
#updateObsGoodness OfflineT_4 21288 -1
##updateObsGoodness OfflineT_9 21040 -1
#updateObsGoodness OfflineT_5 21163 -1
##updateObsGoodness OfflineT_8 21112 -1
##
#updateObsGoodness OfflineT_3 21209 -1
#updateObsGoodness OfflineT_5 20983 -1
#updateObsGoodness OfflineT_6 20896 -1
##updateObsGoodness OfflineT_9 21039 -1
##updateObsGoodness OfflineT_8 21189 -1
##updateObsGoodness OfflineT_7 21051 -1
#updateObsGoodness OfflineT_2 20874 -1
#updateObsGoodness OfflineT_1 20735 -1
##updateObsGoodness OfflineT_10 21263 -1
#updateObsGoodness OfflineT_4 20804 -1
##
#updateObsGoodness OfflineT_4 21288 -1
##updateObsGoodness OfflineT_9 21528 -1
#updateObsGoodness OfflineT_3 21178 -1
##updateObsGoodness OfflineT_10 21070 -1
#updateObsGoodness OfflineT_6 21219 -1
#updateObsGoodness OfflineT_5 21275 -1
#updateObsGoodness OfflineT_1 20801 -1
##updateObsGoodness OfflineT_7 21131 -1
##updateObsGoodness OfflineT_8 21260 -1
#updateObsGoodness OfflineT_2 21192 -1
##
#updateObsGoodness OfflineT_6 22461 -1
##updateObsGoodness OfflineT_9 20984 -1
#updateObsGoodness OfflineT_2 21545 -1
##updateObsGoodness OfflineT_10 20797 -1
##updateObsGoodness OfflineT_7 21097 -1
#updateObsGoodness OfflineT_3 20314 -1
##updateObsGoodness OfflineT_8 20822 -1
#updateObsGoodness OfflineT_1 20959 -1
#updateObsGoodness OfflineT_4 22183 -1
#updateObsGoodness OfflineT_5 25250 -1
##
#updateObsGoodness OfflineT_5 21124 -1
##updateObsGoodness OfflineT_10 20489 -1
##updateObsGoodness OfflineT_8 20487 -1
#updateObsGoodness OfflineT_2 20306 -1
##updateObsGoodness OfflineT_7 20705 -1
##updateObsGoodness OfflineT_9 20595 -1
#updateObsGoodness OfflineT_3 20642 -1
#updateObsGoodness OfflineT_1 20445 -1
#updateObsGoodness OfflineT_4 24267 -1
#updateObsGoodness OfflineT_6 81804 -1
##
#updateObsGoodness OfflineT_3 20759 -1
#updateObsGoodness OfflineT_4 21000 -1
#updateObsGoodness OfflineT_6 20618 -1
##updateObsGoodness OfflineT_9 20607 -1
#updateObsGoodness OfflineT_1 21005 -1
##updateObsGoodness OfflineT_10 20764 -1
##updateObsGoodness OfflineT_7 21000 -1 
#updateObsGoodness OfflineT_5 20675 -1
#updateObsGoodness OfflineT_2 20640 -1
##updateObsGoodness OfflineT_8 21068 -1
##
##updateObsGoodness OfflineT_8 20640 -1
##updateObsGoodness OfflineT_7 20556 -1
#updateObsGoodness OfflineT_3 21047 -1
#updateObsGoodness OfflineT_2 20558 -1
##updateObsGoodness OfflineT_10 20694 -1
#updateObsGoodness OfflineT_4 22621 -1
#updateObsGoodness OfflineT_1 20662 -1
##updateObsGoodness OfflineT_9 20746 -1
#updateObsGoodness OfflineT_6 21122 -1
#updateObsGoodness OfflineT_5 23786 -1
##
##updateObsGoodness OfflineT_8 20633 -1
#updateObsGoodness OfflineT_1 20270 -1
#updateObsGoodness OfflineT_5 20498 -1
#updateObsGoodness OfflineT_4 20582 -1
##updateObsGoodness OfflineT_9 20325 -1
#updateObsGoodness OfflineT_3 20381 -1
#updateObsGoodness OfflineT_2 20254 -1
#updateObsGoodness OfflineT_6 21196 -1
##updateObsGoodness OfflineT_7 20201 -1
##updateObsGoodness OfflineT_10 20312 -1
##
##updateObsGoodness OfflineT_10 20278 -1
#updateObsGoodness OfflineT_1 20250 -1
##updateObsGoodness OfflineT_8 20325 -1
#updateObsGoodness OfflineT_2 20254 -1
#updateObsGoodness OfflineT_4 20313 -1
##updateObsGoodness OfflineT_7 20141 -1
#updateObsGoodness OfflineT_5 20631 -1
##updateObsGoodness OfflineT_9 20338 -1
#updateObsGoodness OfflineT_3 20507 -1
#updateObsGoodness OfflineT_6 20567 -1
##
##updateObsGoodness OfflineT_7 20412 -1
##updateObsGoodness OfflineT_2 20322 -1
##updateObsGoodness OfflineT_1 20232 -1
##updateObsGoodness OfflineT_8 20327 -1
##updateObsGoodness OfflineT_5 20645 -1
##updateObsGoodness OfflineT_3 20429 -1
##updateObsGoodness OfflineT_6 20691 -1
##updateObsGoodness OfflineT_10 20497 -1
##updateObsGoodness OfflineT_4 20264 -1
##updateObsGoodness OfflineT_9 20299 -1
##
##updateObsGoodness OfflineT_9 20616 -1
##updateObsGoodness OfflineT_2 20252 -1
##updateObsGoodness OfflineT_7 20497 -1
##updateObsGoodness OfflineT_3 20603 -1
##updateObsGoodness OfflineT_1 20351 -1
##updateObsGoodness OfflineT_5 20393 -1
##updateObsGoodness OfflineT_10 20385 -1
##updateObsGoodness OfflineT_8 20573 -1
##updateObsGoodness OfflineT_4 20290 -1
##updateObsGoodness OfflineT_6 81218 -1
##
##updateObsGoodness OfflineT_10 20362 -1
##updateObsGoodness OfflineT_1 20103 -1
##updateObsGoodness OfflineT_6 20063 -1
##updateObsGoodness OfflineT_7 20189 -1
##updateObsGoodness OfflineT_2 20411 -1
##updateObsGoodness OfflineT_4 20617 -1
##updateObsGoodness OfflineT_8 20320 -1
##updateObsGoodness OfflineT_5 20345 -1
##updateObsGoodness OfflineT_9 20392 -1
##updateObsGoodness OfflineT_3 20221 -1
##
##updateObsGoodness OfflineT_3 81835 -1
##updateObsGoodness OfflineT_6 81234 -1
##updateObsGoodness OfflineT_7 81983 -1
##updateObsGoodness OfflineT_1 81142 -1
##updateObsGoodness OfflineT_2 81290 -1
##updateObsGoodness OfflineT_10 81828 -1
##updateObsGoodness OfflineT_4 81370 -1
##updateObsGoodness OfflineT_8 81432 -1
##updateObsGoodness OfflineT_9 81327 -1
##updateObsGoodness OfflineT_5 81764 -1
##
##updateObsGoodness OfflineT_3 20463 -1
##updateObsGoodness OfflineT_6 20552 -1
##updateObsGoodness OfflineT_5 20794 -1
##updateObsGoodness OfflineT_2 20764 -1
##updateObsGoodness OfflineT_10 21084 -1
##updateObsGoodness OfflineT_7 20573 -1
##updateObsGoodness OfflineT_8 20826 -1
##updateObsGoodness OfflineT_1 20670 -1
##updateObsGoodness OfflineT_4 20817 -1
##updateObsGoodness OfflineT_9 20758 -1
##
##updateObsGoodness OfflineT_9 20591 -1
##updateObsGoodness OfflineT_10 21022 -1
##updateObsGoodness OfflineT_6 20175 -1
##updateObsGoodness OfflineT_3 21004 -1
##updateObsGoodness OfflineT_2 20579 -1
##updateObsGoodness OfflineT_4 20681 -1
##updateObsGoodness OfflineT_5 20595 -1
##updateObsGoodness OfflineT_1 20758 -1
##updateObsGoodness OfflineT_7 20556 -1
##updateObsGoodness OfflineT_8 20918 -1
##
##updateObsGoodness OfflineT_10 20106 -1
##updateObsGoodness OfflineT_8 20168 -1
##updateObsGoodness OfflineT_3 20237 -1
##updateObsGoodness OfflineT_7 20356 -1
##updateObsGoodness OfflineT_1 20184 -1
##updateObsGoodness OfflineT_9 20500 -1
##updateObsGoodness OfflineT_2 20111 -1
##updateObsGoodness OfflineT_6 20165 -1
##updateObsGoodness OfflineT_4 20155 -1
##updateObsGoodness OfflineT_5 19963 -1
##
##updateObsGoodness OfflineT_5 20134 -1
##updateObsGoodness OfflineT_8 19929 -1
##updateObsGoodness OfflineT_10 19873 -1
##updateObsGoodness OfflineT_3 19930 -1
##updateObsGoodness OfflineT_4 20068 -1
##updateObsGoodness OfflineT_1 20296 -1
##updateObsGoodness OfflineT_6 20004 -1
##updateObsGoodness OfflineT_9 19978 -1
##updateObsGoodness OfflineT_2 20257 -1
##updateObsGoodness OfflineT_7 19847 -1
##
##updateObsGoodness OfflineT_5 20311 -1
##updateObsGoodness OfflineT_3 19897 -1
##updateObsGoodness OfflineT_10 20109 -1
##updateObsGoodness OfflineT_7 19984 -1
##updateObsGoodness OfflineT_6 20040 -1
##updateObsGoodness OfflineT_8 20248 -1
##updateObsGoodness OfflineT_4 19986 -1
##updateObsGoodness OfflineT_9 20426 -1
##updateObsGoodness OfflineT_2 19905 -1
##updateObsGoodness OfflineT_1 19905 -1
##
##updateObsGoodness OfflineT_1 20050 -1
##updateObsGoodness OfflineT_9 20035 -1
##updateObsGoodness OfflineT_7 19956 -1
##updateObsGoodness OfflineT_10 19981 -1
##updateObsGoodness OfflineT_3 19978 -1
##updateObsGoodness OfflineT_5 20023 -1
##updateObsGoodness OfflineT_8 20062 -1
##updateObsGoodness OfflineT_2 20137 -1
##updateObsGoodness OfflineT_4 19993 -1
##updateObsGoodness OfflineT_6 20776 -1
##
##updateObsGoodness OfflineT_9 19986 -1
##updateObsGoodness OfflineT_7 20035 -1
##updateObsGoodness OfflineT_3 19987 -1
##updateObsGoodness OfflineT_10 19908 -1
##updateObsGoodness OfflineT_6 49478 -1
##updateObsGoodness OfflineT_5 19990 -1
##updateObsGoodness OfflineT_1 19985 -1
##updateObsGoodness OfflineT_8 19947 -1
##updateObsGoodness OfflineT_2 20215 -1
##updateObsGoodness OfflineT_4 20042 -1
##
##updateObsGoodness OfflineT_4 20134 -1
##updateObsGoodness OfflineT_3 19987 -1
##updateObsGoodness OfflineT_7 19964 -1
##updateObsGoodness OfflineT_5 20075 -1
##updateObsGoodness OfflineT_10 20027 -1
##updateObsGoodness OfflineT_2 20192 -1
##updateObsGoodness OfflineT_1 19958 -1
##updateObsGoodness OfflineT_9 19968 -1
##updateObsGoodness OfflineT_6 20823 -1
##updateObsGoodness OfflineT_8 19914 -1
##
##updateObsGoodness OfflineT_8 20060 -1
##updateObsGoodness OfflineT_3 19959 -1
##updateObsGoodness OfflineT_7 19977 -1
##updateObsGoodness OfflineT_6 20359 -1
##updateObsGoodness OfflineT_10 19998 -1
##updateObsGoodness OfflineT_5 20270 -1
##updateObsGoodness OfflineT_2 19966 -1
##updateObsGoodness OfflineT_9 20218 -1
##updateObsGoodness OfflineT_4 19975 -1
##updateObsGoodness OfflineT_1 20225 -1
##
##updateObsGoodness OfflineT_6 22332 -1
##updateObsGoodness OfflineT_8 20006 -1
##updateObsGoodness OfflineT_1 19824 -1
##updateObsGoodness OfflineT_9 20292 -1
##updateObsGoodness OfflineT_3 20367 -1
##updateObsGoodness OfflineT_4 21836 -1
##updateObsGoodness OfflineT_7 19997 -1
##updateObsGoodness OfflineT_10 19997 -1
##updateObsGoodness OfflineT_2 20324 -1
##updateObsGoodness OfflineT_5 25253 -1
##
##updateObsGoodness OfflineT_5 20839 -1
##updateObsGoodness OfflineT_9 19819 -1
##updateObsGoodness OfflineT_10 19939 -1
##updateObsGoodness OfflineT_8 20093 -1
##updateObsGoodness OfflineT_1 19989 -1
##updateObsGoodness OfflineT_2 21033 -1
##updateObsGoodness OfflineT_4 23703 -1
##updateObsGoodness OfflineT_3 20017 -1
##updateObsGoodness OfflineT_7 20265 -1
##updateObsGoodness OfflineT_6 80929 -1
##
##updateObsGoodness OfflineT_9 20272 -1
##updateObsGoodness OfflineT_5 20174 -1
##updateObsGoodness OfflineT_8 20139 -1
##updateObsGoodness OfflineT_4 20079 -1
##updateObsGoodness OfflineT_6 20020 -1
##updateObsGoodness OfflineT_3 19983 -1
##updateObsGoodness OfflineT_10 20178 -1
##updateObsGoodness OfflineT_2 20242 -1
##updateObsGoodness OfflineT_7 20300 -1
##updateObsGoodness OfflineT_1 19902 -1
##
##updateObsGoodness OfflineT_10 20061 -1
##updateObsGoodness OfflineT_7 19865 -1
##updateObsGoodness OfflineT_8 19940 -1
##updateObsGoodness OfflineT_6 19902 -1
##updateObsGoodness OfflineT_3 19928 -1
##updateObsGoodness OfflineT_1 20351 -1
##updateObsGoodness OfflineT_2 20305 -1
##updateObsGoodness OfflineT_9 20304 -1
##updateObsGoodness OfflineT_4 21644 -1
##updateObsGoodness OfflineT_5 22769 -1
##
##updateObsGoodness OfflineT_6 20999 -1
##updateObsGoodness OfflineT_7 20008 -1
##updateObsGoodness OfflineT_8 19948 -1
##updateObsGoodness OfflineT_10 19952 -1
##updateObsGoodness OfflineT_5 20228 -1
##updateObsGoodness OfflineT_2 19927 -1
##updateObsGoodness OfflineT_1 19885 -1
##updateObsGoodness OfflineT_3 20333 -1
##updateObsGoodness OfflineT_9 20294 -1
##updateObsGoodness OfflineT_4 19964 -1
##
##updateObsGoodness OfflineT_10 20084 -1
##updateObsGoodness OfflineT_8 20319 -1
##updateObsGoodness OfflineT_7 19758 -1
##updateObsGoodness OfflineT_1 19907 -1
##updateObsGoodness OfflineT_3 19958 -1
##updateObsGoodness OfflineT_9 20301 -1
##updateObsGoodness OfflineT_2 19879 -1
##updateObsGoodness OfflineT_5 20117 -1
##updateObsGoodness OfflineT_6 20613 -1
##updateObsGoodness OfflineT_4 20418 -1
##
##updateObsGoodness OfflineT_3 20284 -1
##updateObsGoodness OfflineT_7 20004 -1
##updateObsGoodness OfflineT_9 19890 -1
##updateObsGoodness OfflineT_1 20273 -1
##updateObsGoodness OfflineT_2 20215 -1
##updateObsGoodness OfflineT_10 19851 -1
##updateObsGoodness OfflineT_8 19741 -1
##updateObsGoodness OfflineT_5 19986 -1
##updateObsGoodness OfflineT_6 20040 -1
##updateObsGoodness OfflineT_4 19873 -1
##
##updateObsGoodness OfflineT_7 20017 -1
##updateObsGoodness OfflineT_1 19992 -1
##updateObsGoodness OfflineT_9 20062 -1
##updateObsGoodness OfflineT_5 20240 -1
##updateObsGoodness OfflineT_10 19838 -1
##updateObsGoodness OfflineT_3 19971 -1
##updateObsGoodness OfflineT_2 19928 -1
##updateObsGoodness OfflineT_8 19895 -1
##updateObsGoodness OfflineT_6 20683 -1
##updateObsGoodness OfflineT_4 20078 -1
##
##updateObsGoodness OfflineT_10 20039 -1
##updateObsGoodness OfflineT_6 20420 -1
##updateObsGoodness OfflineT_8 20085 -1
##updateObsGoodness OfflineT_5 20211 -1
##updateObsGoodness OfflineT_4 19955 -1
##updateObsGoodness OfflineT_2 20031 -1
##updateObsGoodness OfflineT_7 20050 -1
##updateObsGoodness OfflineT_3 20291 -1
##updateObsGoodness OfflineT_9 20383 -1
##updateObsGoodness OfflineT_1 19959 -1
##
##updateObsGoodness OfflineT_1 20283 -1
##updateObsGoodness OfflineT_8 20252 -1
##updateObsGoodness OfflineT_5 20140 -1
##updateObsGoodness OfflineT_10 20080 -1
##updateObsGoodness OfflineT_6 20561 -1
##updateObsGoodness OfflineT_3 20233 -1
##updateObsGoodness OfflineT_4 20311 -1
##updateObsGoodness OfflineT_7 19941 -1
##updateObsGoodness OfflineT_2 20294 -1
##updateObsGoodness OfflineT_9 19943 -1
##
##updateObsGoodness OfflineT_10 20032 -1
##updateObsGoodness OfflineT_2 20020 -1
##updateObsGoodness OfflineT_9 19859 -1
##updateObsGoodness OfflineT_7 20080 -1
##updateObsGoodness OfflineT_8 19882 -1
##updateObsGoodness OfflineT_6 20350 -1
##updateObsGoodness OfflineT_3 20133 -1
##updateObsGoodness OfflineT_5 20253 -1
##updateObsGoodness OfflineT_4 20123 -1
##updateObsGoodness OfflineT_1 20255 -1
##
##updateObsGoodness OfflineT_8 19988 -1
##updateObsGoodness OfflineT_2 20004 -1
##updateObsGoodness OfflineT_9 20041 -1
##updateObsGoodness OfflineT_1 19963 -1
##updateObsGoodness OfflineT_4 20053 -1
##updateObsGoodness OfflineT_7 20068 -1
##updateObsGoodness OfflineT_5 19924 -1
##updateObsGoodness OfflineT_6 21006 -1
##updateObsGoodness OfflineT_3 19860 -1
##updateObsGoodness OfflineT_10 20138 -1
#
#
#
#
#
#
#
#
#
#
#
#
#
#
#
#
#
#
#
#
#
#
#

