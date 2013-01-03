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
module solve3d

use mykind, only : dp_kind, i8_kind
use cscmpi

implicit none

private

public :: init
public :: avg
public :: solve

contains

! private functions

function norm(T1, T2, nx, ny, nz) result(x)
  implicit none
  integer, intent(in) :: nx, ny, nz
  real(dp_kind), intent(in) :: T1(0:nx+1,0:ny+1,0:nz+1), T2(0:nx+1,0:ny+1,0:nz+1)
  integer :: ierr
  real(dp_kind) :: x, xlocal
  xlocal = maxval(abs(T2(1:nx,1:ny,1:nz) - T1(1:nx,1:ny,1:nz)))
  call MPI_allreduce(xlocal, x, 1, MPI_DOUBLE_PRECISION, MPI_MAX, wcomm, ierr)
end function norm

subroutine jacobi_(T2, T1, nx, ny, nz,TI, TJ,time)
  implicit none
  integer, intent(in) :: nx, ny, nz, TI,TJ
   double precision, intent(out) :: time
  real(dp_kind), parameter :: one_sixth = 1.0_dp_kind / 6.0_dp_kind
  real(dp_kind), intent(out) :: T2(0:nx+1,0:ny+1,0:nz+1)
  real(dp_kind), intent(in)  :: T1(0:nx+1,0:ny+1,0:nz+1)
  integer :: i, j, k


  double precision :: start, end

  call rtclock_(start)
  do k=1,nz
     do j=1,ny
        do i=1,nx
           T2(i,j,k) = one_sixth * ( &
                & T1(i+1,j,k) + T1(i-1,j,k) + &
                & T1(i,j+1,k) + T1(i,j-1,k) + &
                & T1(i,j,k+1) + T1(i,j,k-1) )
        end do ! i=1,nx
     end do ! j=1,ny
  end do ! k=1,nz
  call rtclock_(end)
  time=end-start
  call exchangeXdir(T2, nx, ny, nz)
  call exchangeYdir(T2, nx, ny, nz)
  call exchangeZdir(T2, nx, ny, nz)
end subroutine jacobi_


subroutine jacobi(T2, T1, nx, ny, nz, TI, TJ, time)
  implicit none
  integer, intent(in) :: nx, ny, nz, TI, TJ
  double precision, intent(out) :: time
  real(dp_kind), parameter :: one_sixth = 1.0_dp_kind / 6.0_dp_kind
  real(dp_kind), intent(out) :: T2(0:nx+1,0:ny+1,0:nz+1)
  real(dp_kind), intent(in)  :: T1(0:nx+1,0:ny+1,0:nz+1)
  integer :: i, j, k, II, JJ
  double precision :: start, end


!  TI=128
!  TJ=4
  !print*, nx,ny,nz

  call rtclock_(start)
  do JJ=1,ny,TJ
     do II=1,nx,TI
        do k=1,nz
           do j=JJ,min(JJ+TJ-1,ny)
              do i=II,min(II+TI-1,nx)

                 T2(i,j,k) = one_sixth * ( &
                      & T1(i+1,j,k) + T1(i-1,j,k) + &
                      & T1(i,j+1,k) + T1(i,j-1,k) + &
                      & T1(i,j,k+1) + T1(i,j,k-1) )
              end do ! i=1,nx
           end do ! j=1,ny
        end do ! k=1,nz
     end do
  end do
  call rtclock_(end)
  time=end-start
  call exchangeXdir(T2, nx, ny, nz)
  call exchangeYdir(T2, nx, ny, nz)
  call exchangeZdir(T2, nx, ny, nz)
end subroutine jacobi




subroutine exchangeXdir(T, nx, ny, nz)
  implicit none
  integer, intent(in) :: nx, ny, nz
  real(dp_kind), intent(inout) :: T(0:nx+1, 0:ny+1, 0:nz+1)
  integer :: ierr, tag, ncount
  integer :: ireq, reqid(2)
  real(dp_kind) :: sendbuf1(ny * nz), sendbuf2(ny * nz)

  ireq = 0
  tag = 1000
  ncount = ny * nz

  if (prevpe_x /= undef_PE) then ! Initiate send to the previous task (North)
     ireq = ireq + 1
     sendbuf1(1:ncount) = reshape(T(1:1,1:ny,1:nz), (/ncount/))
     call MPI_isend(sendbuf1(1), ncount, MPI_DOUBLE_PRECISION, &
          & prevpe_x, tag - 1, wcomm, reqid(ireq), ierr)
  endif

  if (nextpe_x /= undef_PE) then ! Initiate send to the next task (South)
     ireq = ireq + 1
     sendbuf2(1:ncount) = reshape(T(nx:nx,1:ny,1:nz), (/ncount/))
     call MPI_isend(sendbuf2(1), ncount, MPI_DOUBLE_PRECISION, &
          & nextpe_x, tag + 1, wcomm, reqid(ireq), ierr)
  endif

  if (prevpe_x /= undef_PE) then ! Receive from the task North
     call MPI_recv(T(0:0,1:ny,1:nz), ncount, MPI_DOUBLE_PRECISION, &
          & prevpe_x, tag + 1, wcomm, MPI_STATUS_IGNORE, ierr)
  endif

  if (nextpe_x /= undef_PE) then ! Receive from the task South
     call MPI_recv(T(nx+1:nx+1,1:ny,1:nz), ncount, MPI_DOUBLE_PRECISION, &
          & nextpe_x, tag - 1, wcomm, MPI_STATUS_IGNORE, ierr)
  endif

  if (ireq > 0) call MPI_waitall(ireq, reqid, MPI_STATUSes_IGNORE, ierr)
end subroutine exchangeXdir

subroutine exchangeYdir(T, nx, ny, nz)
  implicit none
  integer, intent(in) :: nx, ny, nz
  real(dp_kind), intent(inout) :: T(0:nx+1, 0:ny+1, 0:nz+1)
  integer :: ierr, tag, ncount
  integer :: ireq, reqid(2)
  real(dp_kind) :: sendbuf1(nx * nz), sendbuf2(nx * nz)

  ireq = 0
  tag = 2000
  ncount = nx * nz

  if (prevpe_y /= undef_PE) then ! Initiate send to the previous task (West)
     ireq = ireq + 1
     sendbuf1(1:ncount) = reshape(T(1:nx,1:1,1:nz), (/ncount/))
     call MPI_isend(sendbuf1(1), ncount, MPI_DOUBLE_PRECISION, &
          & prevpe_y, tag - 1, wcomm, reqid(ireq), ierr)
  endif

  if (nextpe_y /= undef_PE) then ! Initiate send to the next task (East)
     ireq = ireq + 1
     sendbuf2(1:ncount) = reshape(T(1:nx,ny:ny,1:nz), (/ncount/))
     call MPI_isend(sendbuf2(1), ncount, MPI_DOUBLE_PRECISION, &
          & nextpe_y, tag + 1, wcomm, reqid(ireq), ierr)
  endif

  if (prevpe_y /= undef_PE) then ! Receive from the task West
     call MPI_recv(T(1:nx,0:0,1:nz), ncount, MPI_DOUBLE_PRECISION, &
          & prevpe_y, tag + 1, wcomm, MPI_STATUS_IGNORE, ierr)
  endif

  if (nextpe_y /= undef_PE) then ! Receive from the task East
     call MPI_recv(T(1:nx,ny+1:ny+1,1:nz), ncount, MPI_DOUBLE_PRECISION, &
          & nextpe_y, tag - 1, wcomm, MPI_STATUS_IGNORE, ierr)
  endif

  if (ireq > 0) call MPI_waitall(ireq, reqid, MPI_STATUSes_IGNORE, ierr)
end subroutine exchangeYdir

subroutine exchangeZdir(T, nx, ny, nz)
  implicit none
  integer, intent(in) :: nx, ny, nz
  real(dp_kind), intent(inout) :: T(0:nx+1, 0:ny+1, 0:nz+1)
  integer :: ierr, tag, ncount
  integer :: ireq, reqid(2)

  ireq = 0
  tag = 2000
  ncount = (nx + 2) * (ny + 2)

  if (prevpe_z /= undef_PE) then ! Initiate send to the previous task (Bottom)
     ireq = ireq + 1
     call MPI_isend(T(0,0,1), ncount, MPI_DOUBLE_PRECISION, &
          & prevpe_z, tag - 1, wcomm, reqid(ireq), ierr)
  endif

  if (nextpe_z /= undef_PE) then ! Initiate send to the next task (Top)
     ireq = ireq + 1
     call MPI_isend(T(0,0,nz), ncount, MPI_DOUBLE_PRECISION, &
          & nextpe_z, tag + 1, wcomm, reqid(ireq), ierr)
  endif

  if (prevpe_z /= undef_PE) then ! Receive from the task Top
     call MPI_recv(T(0,0,0), ncount, MPI_DOUBLE_PRECISION, &
          & prevpe_z, tag + 1, wcomm, MPI_STATUS_IGNORE, ierr)
  endif

  if (nextpe_z /= undef_PE) then ! Receive from the task Bottom
     call MPI_recv(T(0,0,nz+1), ncount, MPI_DOUBLE_PRECISION, &
          & nextpe_z, tag - 1, wcomm, MPI_STATUS_IGNORE, ierr)
  endif

  if (ireq > 0) call MPI_waitall(ireq, reqid, MPI_STATUSes_IGNORE, ierr)
end subroutine exchangeZdir

! public functions

subroutine init(T, nx, ny, nz)
  implicit none
  integer, intent(in) :: nx, ny, nz
  real(dp_kind), intent(out) :: T(0:nx+1, 0:ny+1, 0:nz+1)
  real(dp_kind), parameter :: Tinit = 20.0_dp_kind
  real(dp_kind), parameter :: TsurfBOT = 25.0_dp_kind
  real(dp_kind), parameter :: TsurfTOP = 50.0_dp_kind
  real(dp_kind), parameter :: Tcorner = 100.0_dp_kind
  if (prevpe_z == undef_PE) T(:,:,   0) = TsurfBOT
  T(:,:,1:nz) = Tinit
  if (nextpe_z == undef_PE) T(:,:,nz+1) = TsurfTOP
  if (prevpe_z == undef_PE) T(:,0,0) = Tcorner
  call exchangeXdir(T, nx, ny, nz)
  call exchangeYdir(T, nx, ny, nz)
  call exchangeZdir(T, nx, ny, nz)
end subroutine init

function avg(T, nx, ny, nz, ndim) result(x)
  implicit none
  integer, intent(in) :: nx, ny, nz, ndim
  real(dp_kind), intent(in) :: T(0:nx+1, 0:ny+1, 0:nz+1)
  integer, parameter :: root = 0
  integer :: ierr
  real(dp_kind) :: x, xlocal
  integer(i8_kind) :: n
  xlocal = sum(T(1:nx,1:ny,1:nz))
  call MPI_reduce(xlocal, x, 1, MPI_DOUBLE_PRECISION, MPI_SUM, root, wcomm, ierr)
  if (mype /= root) x = xlocal
  n = ndim ** 3
  x = x/n
end function avg

subroutine solve(T, nx, ny, nz, ndim, eps, maxiter, s, mype)
  implicit none
  integer, intent(in) :: nx, ny, nz, ndim, maxiter, mype
  real(dp_kind), intent(inout), target :: T(0:nx+1, 0:ny+1, 0:nz+1)
  real(dp_kind), intent(in) :: eps
  integer, intent(out) :: s
  real(dp_kind), allocatable, target :: tmp(:,:,:)
  real(dp_kind), pointer :: T1(:,:,:), T2(:,:,:)
  integer :: iprt, ichk, TI, TJ, int_type, irr, next_iteration, perf, granularity, all_done, harmony_ended
  real(dp_kind) :: Tnorm, Tavg, tim(0:1)
  logical :: Lconverged
  real(dp_kind) :: timer(0:1)
  DOUBLE PRECISION :: start, end, perf_f, time
  allocate(tmp(0:nx+1, 0:ny+1, 0:nz+1))
  tmp = T

  tim(0) = MPI_wtime()

  Tnorm = huge(eps)
  Lconverged = .FALSE.
  iprt = 1000
  ichk = 1

  s = 0

  int_type=1

  granularity=3
  harmony_ended = 0


! first the harmony bits
  call fharmony_startup_(0)
  call fharmony_application_setup_file_('jacobi.tcl'//CHAR(0))

  call fharmony_add_variable_('jacobi'//CHAR(0),'TI'//CHAR(0), int_type, TI)

  call fharmony_add_variable_('jacobi'//CHAR(0),'TJ'//CHAR(0),  int_type, TJ)

  call fharmony_performance_update_(2147483647)

  CALL MPI_BARRIER(MPI_COMM_WORLD, irr)

  call fharmony_request_variable_('TI'//CHAR(0),TI)
  call fharmony_request_variable_('TJ'//CHAR(0),TJ)

  write(*,*)'iteration #', s, 'TI: ', TI, 'TJ: ', TJ, 'rank: ', mype

  perf_f = 0.0
  do while (.not.Lconverged)
     s = s + 1

     if (mod(s,2) == 1) then
        T1 => T   ; T2 => tmp
     else
        T1 => tmp ; T2 => T
     end if



     call jacobi(T2, T1, nx, ny, nz, TI, TJ, time)


     perf_f = time+perf_f

     if (mype == 0) then
        write(*,*) time
     end if

     if (all_done /= 1) then
        if (mod(s,granularity) == 0) then
           perf=(perf_f/granularity)*1000
           call fharmony_performance_update_(perf)

           call fharmony_request_tcl_variable_('all_done'//CHAR(0), int_type, all_done)

           call fharmony_request_variable_('TI'//CHAR(0),TI)
           call fharmony_request_variable_('TJ'//CHAR(0),TJ)
           perf_f=0.0
           if(mype == 0) then
              write(*,*)'iteration #', s, 'TI: ', TI, 'TJ: ', TJ, 'rank: ', mype
           end if
        end if
     else
        if (harmony_ended == 0) then
           call fharmony_end_()
           harmony_ended = 1
        end if
     end if


     if (mod(s,ichk) == 0) then
        Tnorm = norm(T1, T2, nx, ny, nz)
        ichk = iprt
        if (int(Tnorm/eps) < 2) ichk = 1
     endif

     Lconverged = (s == maxiter .or. Tnorm < eps)

     if (Lconverged .or. s == 1 .or. mod(s,iprt) == 0) then
        Tavg = avg(T2, nx, ny, nz, ndim)
        tim(1) = MPI_wtime()
        if (mype == 0) write(0,1000) s, maxiter, Tnorm, Tavg, tim(1) - tim(0)
        tim(0) = tim(1)
     end if

1000 format(i7,' of',i7,' : ',f22.14,' : Tavg=',f22.14,' : dt = ',f8.3,' secs')

  end do ! do while (.not.Lconverged)

  if (mod(s,2) == 1) T = tmp
  deallocate(tmp)

end subroutine solve

end module solve3d
