      program example

      integer i

c     simple performance function array
      integer y(20)

      integer x, int_type, perf

      int_type = 1

c     populate the performance array
      do 40 i=1, 20
         y(i)=(i-9)*(i-9)*(i-9)-75*(i-9)+24
 40   continue

c     initialize the harmony server
      call fharmony_startup(0)

c     send in the application description
      call fharmony_application_setup_file('./simplext.tcl'//CHAR(0))

c     register the tunable parameter(s)
      call fharmony_add_variable('SimplexT'//CHAR(0),'x'//CHAR(0),
     &  int_type,x)

c     main loop
      do 30 i = 1, 200
c     update the performance result
         call fharmony_performance_update(y(x))
c     update the variable x
         call fharmony_request_variable('x'//CHAR(0),x)

 30   continue

c     close the session
      call fharmony_end()
      end program example

