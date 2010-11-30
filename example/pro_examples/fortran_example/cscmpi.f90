module cscmpi
  implicit none
  public
  include 'mpif.h'

  integer, save :: mype = 0
  integer, save :: npes = 1
  integer, save :: wcomm = -1

  integer, parameter :: undef_PE = -1

  integer, save :: npes_x = 1, npes_y = 1, npes_z = 1
  integer, save :: mype_x = 0, mype_y = 0, mype_z = 0
  integer, save :: prevpe_x = undef_PE , prevpe_y = undef_PE, prevpe_z = undef_PE
  integer, save :: nextpe_x = undef_PE , nextpe_y = undef_PE, nextpe_z = undef_PE

contains
  subroutine setup_pes()
    implicit none
    integer :: pe, ierr

    mype_x = mod(mype , npes_x)
    nextpe_x = mype + 1 ; if (mype_x == npes_x - 1) nextpe_x = undef_PE
    prevpe_x = mype - 1 ; if (mype_x ==          0) prevpe_x = undef_PE

    mype_y = mod(mype / npes_x, npes_y)
    nextpe_y = mype + npes_x ; if (mype_y == npes_y - 1) nextpe_y = undef_PE
    prevpe_y = mype - npes_x ; if (mype_y ==          0) prevpe_y = undef_PE

    mype_z =     mype / (npes_x * npes_y)
    nextpe_z = mype + npes_x * npes_y ; if (mype_z == npes_z - 1) nextpe_z = undef_PE
    prevpe_z = mype - npes_x * npes_y ; if (mype_z ==          0) prevpe_z = undef_PE

#ifdef DEBUG
    do pe=0,npes-1
       if (pe == mype) then
          write(0,*) mype,': mype_{x,y,z}   = ',mype_x, mype_y, mype_z
          write(0,*) mype,': prevpe_{x,y,z} = ',prevpe_x, prevpe_y, prevpe_z
          write(0,*) mype,': nextpe_{x,y,z} = ',nextpe_x, nextpe_y, nextpe_z
          call flush(0)
       end if
       call MPI_barrier(wcomm, ierr)
    enddo
#endif
  end subroutine setup_pes

end module cscmpi
