module mykind
  implicit none
  public
  save
  integer, parameter :: i4_kind = selected_int_kind(9)
  integer, parameter :: i8_kind = selected_int_kind(12)
  integer, parameter :: sp_kind = selected_real_kind(6,37)
  integer, parameter :: dp_kind = selected_real_kind(13,300)
end module mykind
