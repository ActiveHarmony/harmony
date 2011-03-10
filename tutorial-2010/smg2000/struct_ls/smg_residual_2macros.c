/*BHEADER**********************************************************************
 * (c) 1997   The Regents of the University of California
 *
 * See the file COPYRIGHT_and_DISCLAIMER for a complete copyright
 * notice, contact person, and disclaimer.
 *
 * $Revision: 1.1.1.1 $
 *********************************************************************EHEADER*/
/******************************************************************************
 *
 * Routine for computing residuals in the SMG code
 *
 *****************************************************************************/

#include "headers.h"

// prepare BLCR support -------------
#ifdef BLCR_CHECKPOINTING 
//#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "libcr.h"
#ifndef O_LARGEFILE
  #define O_LARGEFILE 0
#endif

extern cr_client_id_t cr;
#endif

//prepare DLOPEN support-----------
#ifdef USE_DLOPEN  
static int g_execution_flag =0;
extern double time_stamp();
// dlopen() etc
#include  <dlfcn.h>
void * FunctionLib; /* Handle to the shared lib file */
/*  pointer to a loaded routine */
void (*OUT__1__6755__)(void **__out_argv);
const char *dlError;    /*  Pointer to error string   */
#endif

/*--------------------------------------------------------------------------
 * hypre_SMGResidualData data structure
 *--------------------------------------------------------------------------*/

typedef struct
{
   hypre_Index          base_index;
   hypre_Index          base_stride;

   hypre_StructMatrix  *A;
   hypre_StructVector  *x;
   hypre_StructVector  *b;
   hypre_StructVector  *r;
   hypre_BoxArray      *base_points;
   hypre_ComputePkg    *compute_pkg;

   int                  time_index;
   int                  flops;

} hypre_SMGResidualData;

/*--------------------------------------------------------------------------
 * hypre_SMGResidualCreate
 *--------------------------------------------------------------------------*/

void *
hypre_SMGResidualCreate( )
{
   hypre_SMGResidualData *residual_data;

   residual_data = hypre_CTAlloc(hypre_SMGResidualData, 1);

   (residual_data -> time_index)  = hypre_InitializeTiming("SMGResidual");

   /* set defaults */
   hypre_SetIndex((residual_data -> base_index), 0, 0, 0);
   hypre_SetIndex((residual_data -> base_stride), 1, 1, 1);

   return (void *) residual_data;
}

/*--------------------------------------------------------------------------
 * hypre_SMGResidualSetup
 *--------------------------------------------------------------------------*/

int
hypre_SMGResidualSetup( void               *residual_vdata,
                        hypre_StructMatrix *A,
                        hypre_StructVector *x,
                        hypre_StructVector *b,
                        hypre_StructVector *r              )
{
   int ierr = 0;

   hypre_SMGResidualData *residual_data = (hypre_SMGResidualData *)residual_vdata;

   hypre_IndexRef          base_index  = (residual_data -> base_index);
   hypre_IndexRef          base_stride = (residual_data -> base_stride);
   hypre_Index             unit_stride;

   hypre_StructGrid       *grid;
   hypre_StructStencil    *stencil;
                       
   hypre_BoxArrayArray    *send_boxes;
   hypre_BoxArrayArray    *recv_boxes;
   int                   **send_processes;
   int                   **recv_processes;
   hypre_BoxArrayArray    *indt_boxes;
   hypre_BoxArrayArray    *dept_boxes;
                       
   hypre_BoxArray         *base_points;
   hypre_ComputePkg       *compute_pkg;

   /*----------------------------------------------------------
    * Set up base points and the compute package
    *----------------------------------------------------------*/

   grid    = hypre_StructMatrixGrid(A);
   stencil = hypre_StructMatrixStencil(A);

   hypre_SetIndex(unit_stride, 1, 1, 1);

   base_points = hypre_BoxArrayDuplicate(hypre_StructGridBoxes(grid));
   hypre_ProjectBoxArray(base_points, base_index, base_stride);

   hypre_CreateComputeInfo(grid, stencil,
                        &send_boxes, &recv_boxes,
                        &send_processes, &recv_processes,
                        &indt_boxes, &dept_boxes);

   hypre_ProjectBoxArrayArray(indt_boxes, base_index, base_stride);
   hypre_ProjectBoxArrayArray(dept_boxes, base_index, base_stride);

   hypre_ComputePkgCreate(send_boxes, recv_boxes,
                       unit_stride, unit_stride,
                       send_processes, recv_processes,
                       indt_boxes, dept_boxes,
                       base_stride, grid,
                       hypre_StructVectorDataSpace(x), 1,
                       &compute_pkg);

   /*----------------------------------------------------------
    * Set up the residual data structure
    *----------------------------------------------------------*/

   (residual_data -> A)           = hypre_StructMatrixRef(A);
   (residual_data -> x)           = hypre_StructVectorRef(x);
   (residual_data -> b)           = hypre_StructVectorRef(b);
   (residual_data -> r)           = hypre_StructVectorRef(r);
   (residual_data -> base_points) = base_points;
   (residual_data -> compute_pkg) = compute_pkg;

   /*-----------------------------------------------------
    * Compute flops
    *-----------------------------------------------------*/

   (residual_data -> flops) =
      (hypre_StructMatrixGlobalSize(A) + hypre_StructVectorGlobalSize(x)) /
      (hypre_IndexX(base_stride) *
       hypre_IndexY(base_stride) *
       hypre_IndexZ(base_stride)  );

   return ierr;
}

/*--------------------------------------------------------------------------
 * hypre_SMGResidual
 *--------------------------------------------------------------------------*/

int
hypre_SMGResidual( void               *residual_vdata,
                   hypre_StructMatrix *A,
                   hypre_StructVector *x,
                   hypre_StructVector *b,
                   hypre_StructVector *r              )
{
   int ierr = 0;

   hypre_SMGResidualData *residual_data = (hypre_SMGResidualData *)residual_vdata;

   hypre_IndexRef          base_stride = (residual_data -> base_stride);
   hypre_BoxArray         *base_points = (residual_data -> base_points);
   hypre_ComputePkg       *compute_pkg = (residual_data -> compute_pkg);

   hypre_CommHandle       *comm_handle;
                       
   hypre_BoxArrayArray    *compute_box_aa;
   hypre_BoxArray         *compute_box_a;
   hypre_Box              *compute_box;
                       
   hypre_Box              *A_data_box;
   hypre_Box              *x_data_box;
   hypre_Box              *b_data_box;
   hypre_Box              *r_data_box;
                       
   int                     Ai;
   int                     xi;
   int                     bi;
   int                     ri;
                         
   double                 *Ap;
   double                 *xp;
   double                 *bp;
   double                 *rp;
                       
   hypre_Index             loop_size;
   hypre_IndexRef          start;
                       
   hypre_StructStencil    *stencil;
   hypre_Index            *stencil_shape;
   int                     stencil_size;

   int                     compute_i, i, j, si;
   int                     loopi, loopj, loopk;

   hypre_BeginTiming(residual_data -> time_index);

   /*-----------------------------------------------------------------------
    * Compute residual r = b - Ax
    *-----------------------------------------------------------------------*/

   stencil       = hypre_StructMatrixStencil(A);
   stencil_shape = hypre_StructStencilShape(stencil);
   stencil_size  = hypre_StructStencilSize(stencil);

   for (compute_i = 0; compute_i < 2; compute_i++)
   {
      switch(compute_i)
      {
         case 0:
         {
            xp = hypre_StructVectorData(x);
            hypre_InitializeIndtComputations(compute_pkg, xp, &comm_handle);
            compute_box_aa = hypre_ComputePkgIndtBoxes(compute_pkg);

            /*----------------------------------------
             * Copy b into r
             *----------------------------------------*/

            compute_box_a = base_points;
            hypre_ForBoxI(i, compute_box_a)
               {
                  compute_box = hypre_BoxArrayBox(compute_box_a, i);
                  start = hypre_BoxIMin(compute_box);

                  b_data_box =
                     hypre_BoxArrayBox(hypre_StructVectorDataSpace(b), i);
                  r_data_box =
                     hypre_BoxArrayBox(hypre_StructVectorDataSpace(r), i);

                  bp = hypre_StructVectorBoxData(b, i);
                  rp = hypre_StructVectorBoxData(r, i);

                  hypre_BoxGetStrideSize(compute_box, base_stride, loop_size);
                  hypre_BoxLoop2Begin(loop_size,
                                      b_data_box, start, base_stride, bi,
                                      r_data_box, start, base_stride, ri);
#define HYPRE_BOX_SMP_PRIVATE loopk,loopi,loopj,bi,ri
#include "hypre_box_smp_forloop.h"
                  hypre_BoxLoop2For(loopi, loopj, loopk, bi, ri)
                     {
                        rp[ri] = bp[bi];
                     }
                  hypre_BoxLoop2End(bi, ri);
               }
         }
         break;

         case 1:
         {
            hypre_FinalizeIndtComputations(comm_handle);
            compute_box_aa = hypre_ComputePkgDeptBoxes(compute_pkg);
         }
         break;
      }

      /*--------------------------------------------------------------------
       * Compute r -= A*x
       *--------------------------------------------------------------------*/

      hypre_ForBoxArrayI(i, compute_box_aa)
         {
            compute_box_a = hypre_BoxArrayArrayBoxArray(compute_box_aa, i);

            A_data_box = hypre_BoxArrayBox(hypre_StructMatrixDataSpace(A), i);
            x_data_box = hypre_BoxArrayBox(hypre_StructVectorDataSpace(x), i);
            r_data_box = hypre_BoxArrayBox(hypre_StructVectorDataSpace(r), i);

            rp = hypre_StructVectorBoxData(r, i);

            hypre_ForBoxI(j, compute_box_a)
               {
                  compute_box = hypre_BoxArrayBox(compute_box_a, j);

                  start  = hypre_BoxIMin(compute_box);

                  for (si = 0; si < stencil_size; si++)
                  {
                     Ap = hypre_StructMatrixBoxData(A, i, si);
                     xp = hypre_StructVectorBoxData(x, i) +
                        hypre_BoxOffsetDistance(x_data_box, stencil_shape[si]);

                     hypre_BoxGetStrideSize(compute_box, base_stride,
                                            loop_size);
                     hypre_BoxLoop3Begin(loop_size,
                                         A_data_box, start, base_stride, Ai,
                                         x_data_box, start, base_stride, xi,
                                         r_data_box, start, base_stride, ri);
#if 0          // orginal version using macros           
/*  The following portion is preprocessed to be handled by ROSE outliner */
#define HYPRE_BOX_SMP_PRIVATE loopk,loopi,loopj,Ai,xi,ri
#include "hypre_box_smp_forloop.h"
                     hypre_BoxLoop3For(loopi, loopj, loopk, Ai, xi, ri)
                        {
                           rp[ri] -= Ap[Ai] * xp[xi];
                        }
                     hypre_BoxLoop3End(Ai, xi, ri);
#else         // new version using expanded macros
                    for (hypre__block = 0; hypre__block < hypre__num_blocks;
                         hypre__block++)
                      {
                        loopi = 0;
                        loopj = 0;
                        loopk = 0;
                        hypre__nx = hypre__mx;
                        hypre__ny = hypre__my;
                        hypre__nz = hypre__mz;
                        if (hypre__num_blocks > 1)
                          {
                            if (hypre__dir == 0)
                              {
                                loopi =
                                  hypre__block * hypre__div +
                                  (((hypre__mod) <
                                    (hypre__block)) ? (hypre__mod)
                                   : (hypre__block));
                                hypre__nx =
                                  hypre__div +
                                  ((hypre__mod > hypre__block) ? 1 : 0);
                              }
                            else if (hypre__dir == 1)
                              {
                                loopj =
                                  hypre__block * hypre__div +
                                  (((hypre__mod) <
                                    (hypre__block)) ? (hypre__mod)
                                   : (hypre__block));
                                hypre__ny =
                                  hypre__div +
                                  ((hypre__mod > hypre__block) ? 1 : 0);
                              }
                            else if (hypre__dir == 2)
                              {
                                loopk =
                                  hypre__block * hypre__div +
                                  (((hypre__mod) <
                                    (hypre__block)) ? (hypre__mod)
                                   : (hypre__block));
                                hypre__nz =
                                  hypre__div +
                                  ((hypre__mod > hypre__block) ? 1 : 0);
                              }
                          };
                        Ai =
                          hypre__i1start + loopi * hypre__sx1 +
                          loopj * hypre__sy1 + loopk * hypre__sz1;
                        xi =
                          hypre__i2start + loopi * hypre__sx2 +
                          loopj * hypre__sy2 + loopk * hypre__sz2;
                        ri =
                          hypre__i3start + loopi * hypre__sx3 +
                          loopj * hypre__sy3 + loopk * hypre__sz3;
#ifdef USE_DLOPEN
#ifdef BLCR_CHECKPOINTING
                        // Only checkpoint it at the first occurrence.
                        if (g_execution_flag == 0)
                        {
                          int err;
                          cr_checkpoint_args_t cr_args;  
                          cr_checkpoint_handle_t cr_handle;
                          cr_initialize_checkpoint_args_t(&cr_args);
                          cr_args.cr_scope = CR_SCOPE_PROC;// a process
                          cr_args.cr_target = 0; //self
                          cr_args.cr_signal = SIGKILL; // kill after checkpointing
                          remove("dump.yy");
                          cr_args.cr_fd = open("dump.yy", O_WRONLY|O_CREAT|O_LARGEFILE, 0400);
                          if (cr_args.cr_fd < 0) {
                              printf("Error: cannot open file for checkpoiting context\n");
                              abort();
                          }

                          printf("Checkpointing: stopping here ..\n");

                          err = cr_request_checkpoint(&cr_args, &cr_handle);
                          if (err < 0) {
                            printf("cannot request checkpoining! err=%d\n",err);
                            abort();
                          }
                          // block until the request is served
                          cr_enter_cs(cr);
                          cr_leave_cs(cr);

                          printf("Checkpointing: restarting here ..\n");
#else
                      if (g_execution_flag == 0)
                          {
#endif
          /* Only load the lib for the first time, reuse it later on */
                           printf("Openning the .so file ...\n");
                           FunctionLib = dlopen("/home/liao/svnrepos/benchmarks/smg2000/struct_ls/OUT__1__6755__.so",RTLD_LAZY);
                           dlError = dlerror();
                          if( dlError ) {
                             printf("cannot open .so file!\n");
                             exit(1);
                           }

                          /* Find the first loaded function */
                          OUT__1__6755__ = dlsym( FunctionLib, "OUT__1__6755__");
                          dlError = dlerror();
                          if( dlError ) 
                          {
                            printf("cannot find OUT__1__6755__() !\n");
                            exit(1);
                           }
                        } // end if (flag ==0)

                        g_execution_flag ++;
                        double time1, time2;
        if (g_execution_flag==1)                        
        {
//timing code, how to handle multiple invoking? save the first time eventually for now

                        remove("/tmp/peri.result");
                        time1=time_stamp();
        }
               void *__out_argv1__1527__[21];
               *(__out_argv1__1527__ + 0) = ((void *)(&hypre__nz));
               *(__out_argv1__1527__ + 1) = ((void *)(&hypre__ny));
               *(__out_argv1__1527__ + 2) = ((void *)(&hypre__nx));
               *(__out_argv1__1527__ + 3) = ((void *)(&hypre__sz3));
               *(__out_argv1__1527__ + 4) = ((void *)(&hypre__sy3));
               *(__out_argv1__1527__ + 5) = ((void *)(&hypre__sx3));
               *(__out_argv1__1527__ + 6) = ((void *)(&hypre__sz2));
               *(__out_argv1__1527__ + 7) = ((void *)(&hypre__sy2));
               *(__out_argv1__1527__ + 8) = ((void *)(&hypre__sx2));
               *(__out_argv1__1527__ + 9) = ((void *)(&hypre__sz1));
               *(__out_argv1__1527__ + 10) = ((void *)(&hypre__sy1));
               *(__out_argv1__1527__ + 11) = ((void *)(&hypre__sx1));
               *(__out_argv1__1527__ + 12) = ((void *)(&loopk));
               *(__out_argv1__1527__ + 13) = ((void *)(&loopj));
               *(__out_argv1__1527__ + 14) = ((void *)(&loopi));
               *(__out_argv1__1527__ + 15) = ((void *)(&rp));
               *(__out_argv1__1527__ + 16) = ((void *)(&xp));
               *(__out_argv1__1527__ + 17) = ((void *)(&Ap));
               *(__out_argv1__1527__ + 18) = ((void *)(&ri));
               *(__out_argv1__1527__ + 19) = ((void *)(&xi));
               *(__out_argv1__1527__ + 20) = ((void *)(&Ai));
               (*OUT__1__6755__)(__out_argv1__1527__);
         if (g_execution_flag==1)               
         {
           time2=time_stamp();
           FILE* pfile;
           pfile=fopen("/tmp/peri.result","a+");
           if (pfile != NULL)
           {
           fprintf(pfile,"%f\n",time2-time1);
           fclose(pfile);
           }
           else
             printf("Fatal error: Cannot open /tmp/peri.result!\n");

         }
#ifdef BLCR_CHECKPOINTING         
         //should we abort here? not now for execution validation
         exit(0);
#endif

#else
                          //begin of the loop
                        for (loopk = 0; loopk < hypre__nz; loopk++)
                          {
                            for (loopj = 0; loopj < hypre__ny; loopj++)
                              {
                                for (loopi = 0; loopi < hypre__nx; loopi++)
                                  {
                                    {
                                      rp[ri] -= Ap[Ai] * xp[xi];
                                    }
                                    Ai += hypre__sx1;
                                    xi += hypre__sx2;
                                    ri += hypre__sx3;
                                  }
                                Ai += hypre__sy1 - hypre__nx * hypre__sx1;
                                xi += hypre__sy2 - hypre__nx * hypre__sx2;
                                ri += hypre__sy3 - hypre__nx * hypre__sx3;
                              }
                            Ai += hypre__sz1 - hypre__ny * hypre__sy1;
                            xi += hypre__sz2 - hypre__ny * hypre__sy2;
                            ri += hypre__sz3 - hypre__ny * hypre__sy3;
                          } // end of the loop


#endif   // end of dlopen version

                      }
                  };
#endif    // end of  preprocessed version                 
                  }
               }
         }
   }
   
   /*-----------------------------------------------------------------------
    * Return
    *-----------------------------------------------------------------------*/

   hypre_IncFLOPCount(residual_data -> flops);
   hypre_EndTiming(residual_data -> time_index);

   return ierr;
}

/*--------------------------------------------------------------------------
 * hypre_SMGResidualSetBase
 *--------------------------------------------------------------------------*/
 
int
hypre_SMGResidualSetBase( void        *residual_vdata,
                          hypre_Index  base_index,
                          hypre_Index  base_stride )
{
   hypre_SMGResidualData *residual_data = (hypre_SMGResidualData *)residual_vdata;
   int                    d;
   int                    ierr = 0;
 
   for (d = 0; d < 3; d++)
   {
      hypre_IndexD((residual_data -> base_index),  d)
         = hypre_IndexD(base_index,  d);
      hypre_IndexD((residual_data -> base_stride), d)
         = hypre_IndexD(base_stride, d);
   }
 
   return ierr;
}

/*--------------------------------------------------------------------------
 * hypre_SMGResidualDestroy
 *--------------------------------------------------------------------------*/

int
hypre_SMGResidualDestroy( void *residual_vdata )
{
   int ierr = 0;

   hypre_SMGResidualData *residual_data = (hypre_SMGResidualData *)residual_vdata;

   if (residual_data)
   {
      hypre_StructMatrixDestroy(residual_data -> A);
      hypre_StructVectorDestroy(residual_data -> x);
      hypre_StructVectorDestroy(residual_data -> b);
      hypre_StructVectorDestroy(residual_data -> r);
      hypre_BoxArrayDestroy(residual_data -> base_points);
      hypre_ComputePkgDestroy(residual_data -> compute_pkg );
      hypre_FinalizeTiming(residual_data -> time_index);
      hypre_TFree(residual_data);
   }

   return ierr;
}

