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
program l3d

!-- MPI-version with symmetric 3D-decomposition

use cscmpi
use solve3d, only : init, avg, solve
use mykind, only  : dp_kind, i8_kind
use dump

implicit none

real(dp_kind) :: tim(0:3)
real(dp_kind) :: eps, Tavg
integer :: ndim, maxiter, s
integer :: nx, ny, nz
integer(i8_kind) :: dofs
real(dp_kind), allocatable :: T(:,:,:)
character(len=20) :: ch
character(len=80) :: filename
integer :: ierr, pe

call MPI_init(ierr)

tim(0) = MPI_wtime()

wcomm = MPI_COMM_WORLD
call MPI_comm_rank(wcomm, mype, ierr)
call MPI_comm_size(wcomm, npes, ierr)

if (mype == 0) write(0,*) '*** MPI-version with symmetric 3D-decomposition ***'
if (mype == 0) write(0,*) 'Number of MPI-tasks = ',npes

npes_x = nint(real(npes,dp_kind)**(1.0_dp_kind/3.0_dp_kind))
npes_y = npes_x
npes_z = npes_x

if (mype == 0) write(0,*) 'npes, npes_{x,y,z} = ',npes, npes_x, npes_y, npes_z

if (npes_x * npes_y * npes_z /= npes) then
   if (mype == 0) write(0,*) 'npes_x * npes_y * npes_z /= npes'
   if (mype > 0) call sleep(2)
   call MPI_abort(wcomm, -1, -1)
endif

call setup_pes()

call getarg(1,ch)
read(ch,'(i20)') ndim
call getarg(2,ch)
read(ch,'(i20)') maxiter
call getarg(3,ch)
read(ch,'(f20.0)') eps

if (mype == 0) write(0,*) 'ndim=',ndim,', maxiter=',maxiter
if (mype == 0) write(0,*) 'eps=',eps

dofs = ndim ** 3
if (mype == 0) write(0,*) 'number of degrees of freedom = ',dofs

nx = ndim / npes_x ; if (mype_x < mod(ndim,npes_x)) nx = nx + 1
ny = ndim / npes_y ; if (mype_y < mod(ndim,npes_y)) ny = ny + 1
nz = ndim / npes_z ; if (mype_z < mod(ndim,npes_z)) nz = nz + 1

if (mype == 0) write(0,1002) 'ndim = ',ndim, ', nx * ny * nz =',nx, ny, nz

allocate(T(0:nx+1, 0:ny+1, 0:nz+1))

call init(T, nx, ny, nz)

Tavg = avg(T, nx, ny, nz, ndim)
if (mype == 0) write(0,1001) 'T-average at start = ',Tavg



!     register the tunable parameter(s)
!      call fharmony_add_variable('SimplexT'//CHAR(0),'x'//CHAR(0),
!     &  int_type,x)
!
!c     main loop
!      do 30 i = 1, 200
!c     update the performance result
!         call fharmony_performance_update(y(x))
!c     update the variable x
!         call fharmony_request_variable('x'//CHAR(0),x)

tim(1) = MPI_wtime()
call solve(T, nx, ny, nz, ndim, eps, maxiter, s, mype)
tim(2) = MPI_wtime()

Tavg = avg(T, nx, ny, nz, ndim)
if (mype == 0) write(0,1001) 'T-average at   end = ',Tavg, ' after ',s,' iterations'

!write(filename,'("rawdump.",i4.4,"#",i4.4)') ndim, npes

!call rawdump(filename, T, nx, ny, nz, ndim)

deallocate(T)

tim(3) = MPI_wtime()

if (mype == 0) then
   write(0,1000) 'Solution time : ',tim(2)-tim(1),' secs'
   write(0,1000) '>> Total time : ',tim(3)-tim(0),' secs'
endif

call MPI_finalize(ierr)

1000 format(1x,a,f12.3,a)
1001 format(1x,a,f22.14,:,a,i6,a)
1002 format(1x,a,i4,a,3(i4,:,' *'))
end program l3d
