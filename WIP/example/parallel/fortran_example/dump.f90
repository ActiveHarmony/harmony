c
c Copyright 2003-2013 Jeffrey K. Hollingsworth
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
module dump

use mykind, only : dp_kind, i8_kind
use cscmpi

implicit none
public

contains

subroutine rawdump(filename, T, nx, ny, nz, ndim)
  implicit none
  character(len=*), intent(in) :: filename
  integer, intent(in) :: nx, ny, nz, ndim
  real(dp_kind), intent(in) :: T(0:nx+1, 0:ny+1, 0:nz+1)
  integer, parameter :: root = 0
  integer fh, k, ierr
  integer r8size, nrecl, ncount, icnt
  integer(i8_kind) :: nbytes
  real(dp_kind) :: tim(0:1), dtim, bw, dummy(1)
  real(dp_kind), allocatable :: recvbuf(:), slice(:,:)
  integer :: dx, dy
  integer :: ix1, ix2, iy1, iy2
  integer :: tmpbuf(0:npes-1)
  integer :: dxpe(0:npes-1), dype(0:npes-1)
  integer :: sendcount, recvcounts(0:npes-1), displ(0:npes-1)
  integer :: pe, pe_x, pe_y, ix, iy, ir
  integer :: z_mype, z_pe, k1, k2, kk

  r8size = dp_kind
  ncount = (ndim+2) * (ndim+2)
  nrecl = ncount * r8size ! A slice

  if (mype == root) then
     allocate(recvbuf(ncount))
     allocate(slice(0:ndim+1,0:ndim+1))
  endif

  dx = nx ; ix1 = 1 ; ix2 = nx
  if (prevpe_x == undef_PE) then
     dx = dx + 1 ; ix1 = 0
  endif
  if (nextpe_x == undef_PE) then
     dx = dx + 1 ; ix2 = nx + 1
  endif

  dy = ny ; iy1 = 1 ; iy2 = ny
  if (prevpe_y == undef_PE) then
     dy = dy + 1 ; iy1 = 0
  endif
  if (nextpe_y == undef_PE) then
     dy = dy + 1 ; iy2 = ny + 1
  endif

  dxpe(:) = 0 ; dxpe(mype) = dx
  dype(:) = 0 ; dype(mype) = dy

  call MPI_reduce(dxpe, tmpbuf, npes, MPI_INTEGER, MPI_SUM, root, wcomm, ierr)
  if (mype == root) dxpe(:) = tmpbuf(:)
  call MPI_reduce(dype, tmpbuf, npes, MPI_INTEGER, MPI_SUM, root, wcomm, ierr)
  if (mype == root) dype(:) = tmpbuf(:)

  tim(0) = MPI_wtime()

  if (mype == root) then
     fh = 1
     open(fh, file=trim(filename), form='unformatted', &
          & access='direct', recl=nrecl, status='unknown')
  else
     fh = -1
  endif

!  if (mype == root) write(0,*) 'ndim, nz = ',ndim, nz
  
  kk = 0
  do z_mype = 0,npes_z - 1
     k1 = 1  ; if (z_mype == 0) k1 = 0 
     k2 = ndim / npes_z ; if (z_mype < mod(ndim,npes_z)) k2 = k2 + 1
     if (z_mype == npes_z - 1) k2 = k2 + 1

!     if (mype == root) write(0,*) 'z_mype, k1, k2, kk = ',z_mype, k1, k2, kk

     if (mype_z == z_mype) then
        sendcount = dx * dy
     else
        sendcount = 0
     endif

     if (mype == root) then
        do pe=0,npes-1
           z_pe = pe / (npes_x * npes_y)
           if (z_pe == z_mype) then
              recvcounts(pe) = dxpe(pe) * dype(pe)
           else
              recvcounts(pe) = 0
           endif
        enddo
        displ(0) = 0
        do pe=1,npes-1
           displ(pe) = displ(pe-1) + recvcounts(pe-1)
        enddo
     endif

     do k=k1,k2
!        if (mype == root) write(0,*) 'kk, k = ',kk, k

        call MPI_barrier(wcomm, ierr)
        if (sendcount > 0) then
           call MPI_gatherv( &
                & T(ix1:ix2,iy1:iy2,k), sendcount, MPI_DOUBLE_PRECISION, &
                & recvbuf, recvcounts, displ, MPI_DOUBLE_PRECISION, &
                & root, wcomm, ierr)
        else
           call MPI_gatherv( &
                & dummy, 0, MPI_DOUBLE_PRECISION, &
                & recvbuf, recvcounts, displ, MPI_DOUBLE_PRECISION, &
                & root, wcomm, ierr)
        endif

        if (mype == root) then
           pe = z_mype * npes_x * npes_y - 1
           iy = 0
           do pe_y=0,npes_y-1
              ix = 0
              do pe_x=0,npes_x-1
                 pe = pe + 1
                 ir = displ(pe)
                 icnt = recvcounts(pe)
                 slice(ix:ix+dxpe(pe)-1,iy:iy+dype(pe)-1) = &
                      & reshape(recvbuf(ir+1:ir+icnt), (/dxpe(pe), dype(pe)/))
                 ix = ix + dxpe(pe)
              enddo
              iy = iy + dype(pe)
           enddo
           write(fh, rec=kk+1) slice(:,:)
           kk = kk + 1
        endif
     end do
  end do

  if (mype == root) then
     close(fh)
     deallocate(recvbuf)
     deallocate(slice)
  endif

  tim(1) = MPI_wtime()

  if (mype == root) then
     nbytes = nrecl * (ndim + 2)
     dtim = tim(1) - tim(0)
     bw = nbytes / dtim
     bw = bw / (real(1024, dp_kind) ** 2)

     write(0,1000) 'File = "'//trim(filename)//'", ',&
          & nbytes,' bytes written in ',dtim,' secs, ',bw,' MBytes/s'
  endif

1000 format(1x,a,i15,a,f8.3,a,f8.3,a)
end subroutine rawdump

end module dump
