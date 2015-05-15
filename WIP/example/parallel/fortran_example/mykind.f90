c
c Copyright 2003-2015 Jeffrey K. Hollingsworth
c
c This file is part of Active Harmony.
c
c Active Harmony is free software: you can redistribute it and/or modify
c it under the terms of the GNU Lesser General Public License as published
c by the Free Software Foundation, either version 3 of the License, or
c (at your option) any later version.
c
c Active Harmony is distributed in the hope that it will be useful,
c but WITHOUT ANY WARRANTY; without even the implied warranty of
c MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
c GNU Lesser General Public License for more details.
c
c You should have received a copy of the GNU Lesser General Public License
c along with Active Harmony.  If not, see <http://www.gnu.org/licenses/>.
c
module mykind
  implicit none
  public
  save
  integer, parameter :: i4_kind = selected_int_kind(9)
  integer, parameter :: i8_kind = selected_int_kind(12)
  integer, parameter :: sp_kind = selected_real_kind(6,37)
  integer, parameter :: dp_kind = selected_real_kind(13,300)
end module mykind
