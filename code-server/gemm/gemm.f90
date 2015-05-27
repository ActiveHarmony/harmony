      subroutine gemm(N,A,B,C)
      integer N
      real*8  A(N,N), B(N,N), C(N,N)
      
      integer I,J,K

      integer M

      M = N

      do J=1,M
         do K=1,M
            do I=1,M
               C(I,J) = C(I,J)+A(I,K)*B(K,J)
            end do
         end do
      end do
      END
