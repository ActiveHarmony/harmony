void OUT__1__6755__(void **__out_argv)
{
  int *Ai = (int *)(__out_argv[20]);
  int *xi = (int *)(__out_argv[19]);
  int *ri = (int *)(__out_argv[18]);
  double **Ap = (double **)(__out_argv[17]);
  double **xp = (double **)(__out_argv[16]);
  double **rp = (double **)(__out_argv[15]);
  int *loopi = (int *)(__out_argv[14]);
  int *loopj = (int *)(__out_argv[13]);
  int *loopk = (int *)(__out_argv[12]);
  int *hypre__sx1 = (int *)(__out_argv[11]);
  int *hypre__sy1 = (int *)(__out_argv[10]);
  int *hypre__sz1 = (int *)(__out_argv[9]);
  int *hypre__sx2 = (int *)(__out_argv[8]);
  int *hypre__sy2 = (int *)(__out_argv[7]);
  int *hypre__sz2 = (int *)(__out_argv[6]);
  int *hypre__sx3 = (int *)(__out_argv[5]);
  int *hypre__sy3 = (int *)(__out_argv[4]);
  int *hypre__sz3 = (int *)(__out_argv[3]);
  int *hypre__nx = (int *)(__out_argv[2]);
  int *hypre__ny = (int *)(__out_argv[1]);
  int *hypre__nz = (int *)(__out_argv[0]);
  for ( *loopk = 0;  *loopk <  *hypre__nz; ( *loopk)++) {
    for ( *loopj = 0;  *loopj <  *hypre__ny; ( *loopj)++) {
      for ( *loopi = 0;  *loopi <  *hypre__nx; ( *loopi)++) {{
          ( *rp)[ *ri] -= ((( *Ap)[ *Ai]) * (( *xp)[ *xi]));
        }
         *Ai +=  *hypre__sx1;
         *xi +=  *hypre__sx2;
         *ri +=  *hypre__sx3;
      }
       *Ai += ( *hypre__sy1 - ( *hypre__nx *  *hypre__sx1));
       *xi += ( *hypre__sy2 - ( *hypre__nx *  *hypre__sx2));
       *ri += ( *hypre__sy3 - ( *hypre__nx *  *hypre__sx3));
    }
     *Ai += ( *hypre__sz1 - ( *hypre__ny *  *hypre__sy1));
     *xi += ( *hypre__sz2 - ( *hypre__ny *  *hypre__sy2));
     *ri += ( *hypre__sz3 - ( *hypre__ny *  *hypre__sy3));
  }
}
