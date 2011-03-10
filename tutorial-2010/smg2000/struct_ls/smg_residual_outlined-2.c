/* ROSE-HPCT propagated metrics WALLCLK:20279[SgGlobal at 0x9c1d4a0] */
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
/*--------------------------------------------------------------------------
 * hypre_SMGResidualData data structure
 *--------------------------------------------------------------------------*/
typedef struct __unnamed_class___F0_L21_C9__variable_declaration__variable_type_L13R_variable_name_base_index__DELIMITER___variable_declaration__variable_type_L13R_variable_name_base_stride__DELIMITER___variable_declaration__variable_type___Pb__L14R__Pe___variable_name_A__DELIMITER___variable_declaration__variable_type___Pb__L15R__Pe___variable_name_x__DELIMITER___variable_declaration__variable_type___Pb__L15R__Pe___variable_name_b__DELIMITER___variable_declaration__variable_type___Pb__L15R__Pe___variable_name_r__DELIMITER___variable_declaration__variable_type___Pb__L16R__Pe___variable_name_base_points__DELIMITER___variable_declaration__variable_type___Pb__L17R__Pe___variable_name_compute_pkg__DELIMITER___variable_declaration__variable_type_i_variable_name_time_index__DELIMITER___variable_declaration__variable_type_i_variable_name_flops {
hypre_Index base_index;
hypre_Index base_stride;
hypre_StructMatrix *A;
hypre_StructVector *x;
hypre_StructVector *b;
hypre_StructVector *r;
hypre_BoxArray *base_points;
hypre_ComputePkg *compute_pkg;
int time_index;
int flops;}hypre_SMGResidualData;
/*--------------------------------------------------------------------------
 * hypre_SMGResidualCreate
 *--------------------------------------------------------------------------*/

void *hypre_SMGResidualCreate()
{
  hypre_SMGResidualData *residual_data;
  residual_data = ((hypre_SMGResidualData *)(hypre_CAlloc((((unsigned int )1)),(((unsigned int )(sizeof(hypre_SMGResidualData )))))));
  residual_data -> time_index = hypre_InitializeTiming("SMGResidual");
/* set defaults */
  (((((residual_data -> base_index)[0] = 0) , ((residual_data -> base_index)[1] = 0))) , ((residual_data -> base_index)[2] = 0));
  (((((residual_data -> base_stride)[0] = 1) , ((residual_data -> base_stride)[1] = 1))) , ((residual_data -> base_stride)[2] = 1));
  return (void *)residual_data;
}

/*--------------------------------------------------------------------------
 * hypre_SMGResidualSetup
 *--------------------------------------------------------------------------*/

int hypre_SMGResidualSetup(void *residual_vdata,hypre_StructMatrix *A,hypre_StructVector *x,hypre_StructVector *b,hypre_StructVector *r)
{
  int ierr = 0;
  hypre_SMGResidualData *residual_data = (hypre_SMGResidualData *)residual_vdata;
  hypre_IndexRef base_index = (residual_data -> base_index);
  hypre_IndexRef base_stride = (residual_data -> base_stride);
  hypre_Index unit_stride;
  hypre_StructGrid *grid;
  hypre_StructStencil *stencil;
  hypre_BoxArrayArray *send_boxes;
  hypre_BoxArrayArray *recv_boxes;
  int **send_processes;
  int **recv_processes;
  hypre_BoxArrayArray *indt_boxes;
  hypre_BoxArrayArray *dept_boxes;
  hypre_BoxArray *base_points;
  hypre_ComputePkg *compute_pkg;
/*----------------------------------------------------------
    * Set up base points and the compute package
    *----------------------------------------------------------*/
  grid = (A -> grid);
  stencil = (A -> stencil);
  ((((unit_stride[0] = 1) , (unit_stride[1] = 1))) , (unit_stride[2] = 1));
  base_points = hypre_BoxArrayDuplicate((grid -> boxes));
  hypre_ProjectBoxArray(base_points,base_index,base_stride);
  hypre_CreateComputeInfo(grid,stencil,&send_boxes,&recv_boxes,&send_processes,&recv_processes,&indt_boxes,&dept_boxes);
  hypre_ProjectBoxArrayArray(indt_boxes,base_index,base_stride);
  hypre_ProjectBoxArrayArray(dept_boxes,base_index,base_stride);
  hypre_ComputePkgCreate(send_boxes,recv_boxes,unit_stride,unit_stride,send_processes,recv_processes,indt_boxes,dept_boxes,base_stride,grid,(x -> data_space),1,&compute_pkg);
/*----------------------------------------------------------
    * Set up the residual data structure
    *----------------------------------------------------------*/
  residual_data -> A = hypre_StructMatrixRef(A);
  residual_data -> x = hypre_StructVectorRef(x);
  residual_data -> b = hypre_StructVectorRef(b);
  residual_data -> r = hypre_StructVectorRef(r);
  residual_data -> base_points = base_points;
  residual_data -> compute_pkg = compute_pkg;
/*-----------------------------------------------------
    * Compute flops
    *-----------------------------------------------------*/
  residual_data -> flops = (((A -> global_size) + (x -> global_size)) / (((base_stride[0]) * (base_stride[1])) * (base_stride[2])));
  return ierr;
}

/*--------------------------------------------------------------------------
 * hypre_SMGResidual
 *--------------------------------------------------------------------------*/
void OUT__1__6119__(void **__out_argv);
/* ROSE-HPCT propagated metrics WALLCLK:20279[SgFunctionDeclaration at 0xb7bc86a0] */
/* ROSE-HPCT propagated metrics WALLCLK:20279[SgFunctionDefinition at 0xb7c8a2c4] */

int hypre_SMGResidual(void *residual_vdata,hypre_StructMatrix *A,hypre_StructVector *x,hypre_StructVector *b,hypre_StructVector *r)
/* ROSE-HPCT propagated metrics WALLCLK:20279[SgBasicBlock at 0x9f89f54] */
{
  int ierr = 0;
  hypre_SMGResidualData *residual_data = (hypre_SMGResidualData *)residual_vdata;
  hypre_IndexRef base_stride = (residual_data -> base_stride);
  hypre_BoxArray *base_points = (residual_data -> base_points);
  hypre_ComputePkg *compute_pkg = (residual_data -> compute_pkg);
  hypre_CommHandle *comm_handle;
  hypre_BoxArrayArray *compute_box_aa;
  hypre_BoxArray *compute_box_a;
  hypre_Box *compute_box;
  hypre_Box *A_data_box;
  hypre_Box *x_data_box;
  hypre_Box *b_data_box;
  hypre_Box *r_data_box;
  int Ai;
  int xi;
  int bi;
  int ri;
  double *Ap;
  double *xp;
  double *bp;
  double *rp;
  hypre_Index loop_size;
  hypre_IndexRef start;
  hypre_StructStencil *stencil;
  hypre_Index *stencil_shape;
  int stencil_size;
  int compute_i;
  int i;
  int j;
  int si;
  int loopi;
  int loopj;
  int loopk;
  hypre_BeginTiming((residual_data -> time_index));
/*-----------------------------------------------------------------------
    * Compute residual r = b - Ax
    *-----------------------------------------------------------------------*/
  stencil = (A -> stencil);
  stencil_shape = (stencil -> shape);
  stencil_size = (stencil -> size);
/* ROSE-HPCT propagated metrics WALLCLK:20276[SgForStatement at 0xb7b00008] */
  for (compute_i = 0; compute_i < 2; compute_i++) 
/* ROSE-HPCT propagated metrics WALLCLK:20276[SgBasicBlock at 0x9f89fd0] */
{
    switch(compute_i){
      case 0:
/* ROSE-HPCT propagated metrics WALLCLK:3448[SgBasicBlock at 0x9f8a0c8] */
{
/* ROSE-HPCT propagated metrics WALLCLK:3448[SgBasicBlock at 0x9f8a144] */
{
          xp = (x -> data);
          hypre_InitializeIndtComputations(compute_pkg,xp,&comm_handle);
          compute_box_aa = (compute_pkg -> indt_boxes);
/*----------------------------------------
             * Copy b into r
             *----------------------------------------*/
          compute_box_a = base_points;
/* ROSE-HPCT propagated metrics WALLCLK:3448[SgForStatement at 0xb7b0008c] */
          for (i = 0; i < (compute_box_a -> size); i++) 
/* ROSE-HPCT propagated metrics WALLCLK:3448[SgBasicBlock at 0x9f8a1c0] */
{
            compute_box = ((compute_box_a -> boxes) + i);
            start = (compute_box -> imin);
            b_data_box = ((( *(b -> data_space)).boxes) + i);
            r_data_box = ((( *(r -> data_space)).boxes) + i);
            bp = ((b -> data) + ((b -> data_indices)[i]));
            rp = ((r -> data) + ((r -> data_indices)[i]));
            hypre_BoxGetStrideSize(compute_box,base_stride,loop_size);
/* ROSE-HPCT propagated metrics WALLCLK:3446[SgBasicBlock at 0x9f8a23c] */
{
              int hypre__i1start = (((start[0]) - ((b_data_box -> imin)[0])) + ((((start[1]) - ((b_data_box -> imin)[1])) + (((start[2]) - ((b_data_box -> imin)[2])) * (((0 < ((((b_data_box -> imax)[1]) - ((b_data_box -> imin)[1])) + 1))?((((b_data_box -> imax)[1]) - ((b_data_box -> imin)[1])) + 1):0)))) * (((0 < ((((b_data_box -> imax)[0]) - ((b_data_box -> imin)[0])) + 1))?((((b_data_box -> imax)[0]) - ((b_data_box -> imin)[0])) + 1):0))));
              int hypre__i2start = (((start[0]) - ((r_data_box -> imin)[0])) + ((((start[1]) - ((r_data_box -> imin)[1])) + (((start[2]) - ((r_data_box -> imin)[2])) * (((0 < ((((r_data_box -> imax)[1]) - ((r_data_box -> imin)[1])) + 1))?((((r_data_box -> imax)[1]) - ((r_data_box -> imin)[1])) + 1):0)))) * (((0 < ((((r_data_box -> imax)[0]) - ((r_data_box -> imin)[0])) + 1))?((((r_data_box -> imax)[0]) - ((r_data_box -> imin)[0])) + 1):0))));
              int hypre__sx1 = (base_stride[0]);
              int hypre__sy1 = ((base_stride[1]) * (((0 < ((((b_data_box -> imax)[0]) - ((b_data_box -> imin)[0])) + 1))?((((b_data_box -> imax)[0]) - ((b_data_box -> imin)[0])) + 1):0)));
              int hypre__sz1 = (((base_stride[2]) * (((0 < ((((b_data_box -> imax)[0]) - ((b_data_box -> imin)[0])) + 1))?((((b_data_box -> imax)[0]) - ((b_data_box -> imin)[0])) + 1):0))) * (((0 < ((((b_data_box -> imax)[1]) - ((b_data_box -> imin)[1])) + 1))?((((b_data_box -> imax)[1]) - ((b_data_box -> imin)[1])) + 1):0)));
              int hypre__sx2 = (base_stride[0]);
              int hypre__sy2 = ((base_stride[1]) * (((0 < ((((r_data_box -> imax)[0]) - ((r_data_box -> imin)[0])) + 1))?((((r_data_box -> imax)[0]) - ((r_data_box -> imin)[0])) + 1):0)));
              int hypre__sz2 = (((base_stride[2]) * (((0 < ((((r_data_box -> imax)[0]) - ((r_data_box -> imin)[0])) + 1))?((((r_data_box -> imax)[0]) - ((r_data_box -> imin)[0])) + 1):0))) * (((0 < ((((r_data_box -> imax)[1]) - ((r_data_box -> imin)[1])) + 1))?((((r_data_box -> imax)[1]) - ((r_data_box -> imin)[1])) + 1):0)));
              int hypre__nx = (loop_size[0]);
              int hypre__ny = (loop_size[1]);
              int hypre__nz = (loop_size[2]);
              int hypre__mx = hypre__nx;
              int hypre__my = hypre__ny;
              int hypre__mz = hypre__nz;
              int hypre__dir;
              int hypre__max;
              int hypre__div;
              int hypre__mod;
              int hypre__block;
              int hypre__num_blocks;
              hypre__dir = 0;
              hypre__max = hypre__nx;
              if (hypre__ny > hypre__max) {
                hypre__dir = 1;
                hypre__max = hypre__ny;
              }
              if (hypre__nz > hypre__max) {
                hypre__dir = 2;
                hypre__max = hypre__nz;
              }
              hypre__num_blocks = 1;
              if (hypre__max < hypre__num_blocks) {
                hypre__num_blocks = hypre__max;
              }
              if (hypre__num_blocks > 0) {
                hypre__div = (hypre__max / hypre__num_blocks);
                hypre__mod = (hypre__max % hypre__num_blocks);
              }
#define HYPRE_BOX_SMP_PRIVATE loopk,loopi,loopj,bi,ri
#include "hypre_box_smp_forloop.h"
/* ROSE-HPCT propagated metrics WALLCLK:3444[SgForStatement at 0xb7b00110] */
              for (hypre__block = 0; hypre__block < hypre__num_blocks; hypre__block++) 
/* ROSE-HPCT propagated metrics WALLCLK:3157[SgBasicBlock at 0x9f8a4a8] */
{
                loopi = 0;
                loopj = 0;
                loopk = 0;
                hypre__nx = hypre__mx;
                hypre__ny = hypre__my;
                hypre__nz = hypre__mz;
                if (hypre__num_blocks > 1) {
                  if (hypre__dir == 0) {
                    loopi = ((hypre__block * hypre__div) + (((hypre__mod < hypre__block)?hypre__mod:hypre__block)));
                    hypre__nx = (hypre__div + (((hypre__mod > hypre__block)?1:0)));
                  }
                  else if (hypre__dir == 1) {
                    loopj = ((hypre__block * hypre__div) + (((hypre__mod < hypre__block)?hypre__mod:hypre__block)));
                    hypre__ny = (hypre__div + (((hypre__mod > hypre__block)?1:0)));
                  }
                  else if (hypre__dir == 2) {
                    loopk = ((hypre__block * hypre__div) + (((hypre__mod < hypre__block)?hypre__mod:hypre__block)));
                    hypre__nz = (hypre__div + (((hypre__mod > hypre__block)?1:0)));
                  }
                }
                bi = (((hypre__i1start + (loopi * hypre__sx1)) + (loopj * hypre__sy1)) + (loopk * hypre__sz1));
                ri = (((hypre__i2start + (loopi * hypre__sx2)) + (loopj * hypre__sy2)) + (loopk * hypre__sz2));
/* ROSE-HPCT propagated metrics WALLCLK:3157[SgForStatement at 0xb7b00194] */
                for (loopk = 0; loopk < hypre__nz; loopk++) 
/* ROSE-HPCT propagated metrics WALLCLK:3157[SgBasicBlock at 0x9f8a714] */
{
/* ROSE-HPCT propagated metrics WALLCLK:3157[SgForStatement at 0xb7b00218] */
                  for (loopj = 0; loopj < hypre__ny; loopj++) 
/* ROSE-HPCT propagated metrics WALLCLK:3157[SgBasicBlock at 0x9f8a790] */
{
/* ROSE-HPCT propagated metrics WALLCLK:3157[SgForStatement at 0xb7b0029c] */
                    for (loopi = 0; loopi < hypre__nx; loopi++) 
/* ROSE-HPCT propagated metrics WALLCLK:3157[SgBasicBlock at 0x9f8a80c] */
{
/* ROSE-HPCT propagated metrics WALLCLK:1783[SgBasicBlock at 0x9f8a888] */
{
                        rp[ri] = (bp[bi]);
                      }
                      bi += hypre__sx1;
                      ri += hypre__sx2;
                    }
                    bi += (hypre__sy1 - (hypre__nx * hypre__sx1));
                    ri += (hypre__sy2 - (hypre__nx * hypre__sx2));
                  }
                  bi += (hypre__sz1 - (hypre__ny * hypre__sy1));
                  ri += (hypre__sz2 - (hypre__ny * hypre__sy2));
                }
              }
            }
          }
        }
        break; 
      }
      case 1:
{
{
          hypre_FinalizeIndtComputations(comm_handle);
          compute_box_aa = (compute_pkg -> dept_boxes);
        }
        break; 
      }
    }
/*--------------------------------------------------------------------
       * Compute r -= A*x
       *--------------------------------------------------------------------*/
/* ROSE-HPCT propagated metrics WALLCLK:20276[SgForStatement at 0xb7b00320] */
    for (i = 0; i < (compute_box_aa -> size); i++) 
/* ROSE-HPCT propagated metrics WALLCLK:20274[SgBasicBlock at 0x9f8a9fc] */
{
      compute_box_a = ((compute_box_aa -> box_arrays)[i]);
      A_data_box = ((( *(A -> data_space)).boxes) + i);
      x_data_box = ((( *(x -> data_space)).boxes) + i);
      r_data_box = ((( *(r -> data_space)).boxes) + i);
      rp = ((r -> data) + ((r -> data_indices)[i]));
/* ROSE-HPCT propagated metrics WALLCLK:20272[SgForStatement at 0xb7b003a4] */
      for (j = 0; j < (compute_box_a -> size); j++) 
/* ROSE-HPCT propagated metrics WALLCLK:20271[SgBasicBlock at 0x9f8aa78] */
{
        compute_box = ((compute_box_a -> boxes) + j);
        start = (compute_box -> imin);
/* ROSE-HPCT propagated metrics WALLCLK:20271[SgForStatement at 0xb7b00428] */
        for (si = 0; si < stencil_size; si++) 
/* ROSE-HPCT propagated metrics WALLCLK:20271[SgBasicBlock at 0x9f8aaf4] */
{
          Ap = ((A -> data) + (((A -> data_indices)[i])[si]));
          xp = (((x -> data) + ((x -> data_indices)[i])) + (((stencil_shape[si])[0]) + ((((stencil_shape[si])[1]) + (((stencil_shape[si])[2]) * (((0 < ((((x_data_box -> imax)[1]) - ((x_data_box -> imin)[1])) + 1))?((((x_data_box -> imax)[1]) - ((x_data_box -> imin)[1])) + 1):0)))) * (((0 < ((((x_data_box -> imax)[0]) - ((x_data_box -> imin)[0])) + 1))?((((x_data_box -> imax)[0]) - ((x_data_box -> imin)[0])) + 1):0)))));
          hypre_BoxGetStrideSize(compute_box,base_stride,loop_size);
/* ROSE-HPCT propagated metrics WALLCLK:20266[SgBasicBlock at 0x9f8ab70] */
{
            int hypre__i1start = (((start[0]) - ((A_data_box -> imin)[0])) + ((((start[1]) - ((A_data_box -> imin)[1])) + (((start[2]) - ((A_data_box -> imin)[2])) * (((0 < ((((A_data_box -> imax)[1]) - ((A_data_box -> imin)[1])) + 1))?((((A_data_box -> imax)[1]) - ((A_data_box -> imin)[1])) + 1):0)))) * (((0 < ((((A_data_box -> imax)[0]) - ((A_data_box -> imin)[0])) + 1))?((((A_data_box -> imax)[0]) - ((A_data_box -> imin)[0])) + 1):0))));
            int hypre__i2start = (((start[0]) - ((x_data_box -> imin)[0])) + ((((start[1]) - ((x_data_box -> imin)[1])) + (((start[2]) - ((x_data_box -> imin)[2])) * (((0 < ((((x_data_box -> imax)[1]) - ((x_data_box -> imin)[1])) + 1))?((((x_data_box -> imax)[1]) - ((x_data_box -> imin)[1])) + 1):0)))) * (((0 < ((((x_data_box -> imax)[0]) - ((x_data_box -> imin)[0])) + 1))?((((x_data_box -> imax)[0]) - ((x_data_box -> imin)[0])) + 1):0))));
            int hypre__i3start = (((start[0]) - ((r_data_box -> imin)[0])) + ((((start[1]) - ((r_data_box -> imin)[1])) + (((start[2]) - ((r_data_box -> imin)[2])) * (((0 < ((((r_data_box -> imax)[1]) - ((r_data_box -> imin)[1])) + 1))?((((r_data_box -> imax)[1]) - ((r_data_box -> imin)[1])) + 1):0)))) * (((0 < ((((r_data_box -> imax)[0]) - ((r_data_box -> imin)[0])) + 1))?((((r_data_box -> imax)[0]) - ((r_data_box -> imin)[0])) + 1):0))));
            int hypre__sx1 = (base_stride[0]);
            int hypre__sy1 = ((base_stride[1]) * (((0 < ((((A_data_box -> imax)[0]) - ((A_data_box -> imin)[0])) + 1))?((((A_data_box -> imax)[0]) - ((A_data_box -> imin)[0])) + 1):0)));
            int hypre__sz1 = (((base_stride[2]) * (((0 < ((((A_data_box -> imax)[0]) - ((A_data_box -> imin)[0])) + 1))?((((A_data_box -> imax)[0]) - ((A_data_box -> imin)[0])) + 1):0))) * (((0 < ((((A_data_box -> imax)[1]) - ((A_data_box -> imin)[1])) + 1))?((((A_data_box -> imax)[1]) - ((A_data_box -> imin)[1])) + 1):0)));
            int hypre__sx2 = (base_stride[0]);
            int hypre__sy2 = ((base_stride[1]) * (((0 < ((((x_data_box -> imax)[0]) - ((x_data_box -> imin)[0])) + 1))?((((x_data_box -> imax)[0]) - ((x_data_box -> imin)[0])) + 1):0)));
            int hypre__sz2 = (((base_stride[2]) * (((0 < ((((x_data_box -> imax)[0]) - ((x_data_box -> imin)[0])) + 1))?((((x_data_box -> imax)[0]) - ((x_data_box -> imin)[0])) + 1):0))) * (((0 < ((((x_data_box -> imax)[1]) - ((x_data_box -> imin)[1])) + 1))?((((x_data_box -> imax)[1]) - ((x_data_box -> imin)[1])) + 1):0)));
            int hypre__sx3 = (base_stride[0]);
            int hypre__sy3 = ((base_stride[1]) * (((0 < ((((r_data_box -> imax)[0]) - ((r_data_box -> imin)[0])) + 1))?((((r_data_box -> imax)[0]) - ((r_data_box -> imin)[0])) + 1):0)));
            int hypre__sz3 = (((base_stride[2]) * (((0 < ((((r_data_box -> imax)[0]) - ((r_data_box -> imin)[0])) + 1))?((((r_data_box -> imax)[0]) - ((r_data_box -> imin)[0])) + 1):0))) * (((0 < ((((r_data_box -> imax)[1]) - ((r_data_box -> imin)[1])) + 1))?((((r_data_box -> imax)[1]) - ((r_data_box -> imin)[1])) + 1):0)));
            int hypre__nx = (loop_size[0]);
            int hypre__ny = (loop_size[1]);
            int hypre__nz = (loop_size[2]);
            int hypre__mx = hypre__nx;
            int hypre__my = hypre__ny;
            int hypre__mz = hypre__nz;
            int hypre__dir;
            int hypre__max;
            int hypre__div;
            int hypre__mod;
            int hypre__block;
            int hypre__num_blocks;
            hypre__dir = 0;
            hypre__max = hypre__nx;
            if (hypre__ny > hypre__max) {
              hypre__dir = 1;
              hypre__max = hypre__ny;
            }
            if (hypre__nz > hypre__max) {
              hypre__dir = 2;
              hypre__max = hypre__nz;
            }
            hypre__num_blocks = 1;
            if (hypre__max < hypre__num_blocks) {
              hypre__num_blocks = hypre__max;
            }
            if (hypre__num_blocks > 0) {
              hypre__div = (hypre__max / hypre__num_blocks);
              hypre__mod = (hypre__max % hypre__num_blocks);
            }
#define HYPRE_BOX_SMP_PRIVATE loopk,loopi,loopj,Ai,xi,ri
#include "hypre_box_smp_forloop.h"
/* ROSE-HPCT propagated metrics WALLCLK:20251[SgForStatement at 0xb7b004ac] */
            for (hypre__block = 0; hypre__block < hypre__num_blocks; hypre__block++) 
/* ROSE-HPCT propagated metrics WALLCLK:18704[SgBasicBlock at 0x9f8addc] */
{
              loopi = 0;
              loopj = 0;
              loopk = 0;
              hypre__nx = hypre__mx;
              hypre__ny = hypre__my;
              hypre__nz = hypre__mz;
              if (hypre__num_blocks > 1) {
                if (hypre__dir == 0) {
                  loopi = ((hypre__block * hypre__div) + (((hypre__mod < hypre__block)?hypre__mod:hypre__block)));
                  hypre__nx = (hypre__div + (((hypre__mod > hypre__block)?1:0)));
                }
                else if (hypre__dir == 1) {
                  loopj = ((hypre__block * hypre__div) + (((hypre__mod < hypre__block)?hypre__mod:hypre__block)));
                  hypre__ny = (hypre__div + (((hypre__mod > hypre__block)?1:0)));
                }
                else if (hypre__dir == 2) {
                  loopk = ((hypre__block * hypre__div) + (((hypre__mod < hypre__block)?hypre__mod:hypre__block)));
                  hypre__nz = (hypre__div + (((hypre__mod > hypre__block)?1:0)));
                }
              }
              Ai = (((hypre__i1start + (loopi * hypre__sx1)) + (loopj * hypre__sy1)) + (loopk * hypre__sz1));
              xi = (((hypre__i2start + (loopi * hypre__sx2)) + (loopj * hypre__sy2)) + (loopk * hypre__sz2));
              ri = (((hypre__i3start + (loopi * hypre__sx3)) + (loopj * hypre__sy3)) + (loopk * hypre__sz3));
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
              OUT__1__6119__(__out_argv1__1527__);
            }
          }
        }
      }
    }
  }
/*-----------------------------------------------------------------------
    * Return
    *-----------------------------------------------------------------------*/
  hypre_IncFLOPCount((residual_data -> flops));
  hypre_EndTiming((residual_data -> time_index));
  return ierr;
}

/*--------------------------------------------------------------------------
 * hypre_SMGResidualSetBase
 *--------------------------------------------------------------------------*/

int hypre_SMGResidualSetBase(void *residual_vdata,hypre_Index base_index,hypre_Index base_stride)
{
  hypre_SMGResidualData *residual_data = (hypre_SMGResidualData *)residual_vdata;
  int d;
  int ierr = 0;
  for (d = 0; d < 3; d++) {
    (residual_data -> base_index)[d] = (base_index[d]);
    (residual_data -> base_stride)[d] = (base_stride[d]);
  }
  return ierr;
}

/*--------------------------------------------------------------------------
 * hypre_SMGResidualDestroy
 *--------------------------------------------------------------------------*/

int hypre_SMGResidualDestroy(void *residual_vdata)
{
  int ierr = 0;
  hypre_SMGResidualData *residual_data = (hypre_SMGResidualData *)residual_vdata;
  if (residual_data != (0)) {
    hypre_StructMatrixDestroy((residual_data -> A));
    hypre_StructVectorDestroy((residual_data -> x));
    hypre_StructVectorDestroy((residual_data -> b));
    hypre_StructVectorDestroy((residual_data -> r));
    hypre_BoxArrayDestroy((residual_data -> base_points));
    hypre_ComputePkgDestroy((residual_data -> compute_pkg));
    hypre_FinalizeTiming((residual_data -> time_index));
    (hypre_Free(((char *)residual_data)) , (residual_data = ((0))));
  }
  return ierr;
}

