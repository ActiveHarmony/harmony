#
# Copyright 2003-2016 Jeffrey K. Hollingsworth
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
harmonyApp SimplexT {
  { harmonyBundle x {
      int {0 19 1 10 4} global
}   }
  { harmonyBundle y {
      int {0 19 2 10 6} global
}   }
    { obsGoodness -10 100 global }
  { predGoodness -10 -100}
}
