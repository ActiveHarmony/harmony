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
harmonyApp gemm { 
    { harmonyBundle TI  { 
       int {2 500 2} global 
       }
        }
    { harmonyBundle TJ  { 
       int {2 500 2} global 
       }
        }
    { harmonyBundle TK  { 
       int {2 500 2} global 
       }
        }
    { harmonyBundle UI  { 
       int {1 8 1} global 
       }
        }
    { harmonyBundle UJ  { 
       int {1 8 1} global 
       }
        }
    { obsGoodness 2000 6200 global }
    { predGoodness 2300 3500 }
}
