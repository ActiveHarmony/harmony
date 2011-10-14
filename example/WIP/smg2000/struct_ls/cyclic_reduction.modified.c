#include "headers.h"
typedef struct
{
   MPI_Comm              comm;

   int                   num_levels;

   int                   cdir;         /* coarsening direction */
   hypre_Index           base_index;
   hypre_Index           base_stride;

   hypre_StructGrid    **grid_l;

   hypre_BoxArray       *base_points;
   hypre_BoxArray      **fine_points_l;

   double               *data;
   hypre_StructMatrix  **A_l;
   hypre_StructVector  **x_l;

   hypre_ComputePkg    **down_compute_pkg_l;
   hypre_ComputePkg    **up_compute_pkg_l;

   int                   time_index;
   int                   solve_flops;

} hypre_CyclicReductionData;

void *
hypre_CyclicReductionCreate( MPI_Comm comm )
{
   hypre_CyclicReductionData *cyc_red_data;

   cyc_red_data = ( (hypre_CyclicReductionData *)hypre_CAlloc((unsigned int)(1), (unsigned int)sizeof(hypre_CyclicReductionData)) );

   (cyc_red_data -> comm) = comm;
   (cyc_red_data -> cdir) = 0;
   (cyc_red_data -> time_index) = 0;


   ( ((cyc_red_data -> base_index)[0]) = 0, ((cyc_red_data -> base_index)[1]) = 0, ((cyc_red_data -> base_index)[2]) = 0 );
   ( ((cyc_red_data -> base_stride)[0]) = 1, ((cyc_red_data -> base_stride)[1]) = 1, ((cyc_red_data -> base_stride)[2]) = 1 );

   return (void *) cyc_red_data;
}





hypre_StructMatrix *
hypre_CycRedCreateCoarseOp( hypre_StructMatrix *A,
                            hypre_StructGrid *coarse_grid,
                            int cdir )
{
   hypre_StructMatrix *Ac;

   hypre_Index *Ac_stencil_shape;
   hypre_StructStencil *Ac_stencil;
   int Ac_stencil_size;
   int Ac_stencil_dim;
   int Ac_num_ghost[] = {0, 0, 0, 0, 0, 0};

   int i;
   int stencil_rank;

   Ac_stencil_dim = 1;





   stencil_rank = 0;







   if (!((A) -> symmetric))
   {
      Ac_stencil_size = 3;
      Ac_stencil_shape = ( (hypre_Index *)hypre_CAlloc((unsigned int)(Ac_stencil_size), (unsigned int)sizeof(hypre_Index)) );
      for (i = -1; i < 2; i++)
      {

         ( (Ac_stencil_shape[stencil_rank][0]) = i, (Ac_stencil_shape[stencil_rank][1]) = 0, (Ac_stencil_shape[stencil_rank][2]) = 0 );
         stencil_rank++;
      }
   }
//# 158 "cyclic_reduction.c"
   else
   {
      Ac_stencil_size = 2;
      Ac_stencil_shape = ( (hypre_Index *)hypre_CAlloc((unsigned int)(Ac_stencil_size), (unsigned int)sizeof(hypre_Index)) );
      for (i = -1; i < 1; i++)
      {


         ( (Ac_stencil_shape[stencil_rank][0]) = i, (Ac_stencil_shape[stencil_rank][1]) = 0, (Ac_stencil_shape[stencil_rank][2]) = 0 );
         stencil_rank++;
      }
   }

   Ac_stencil = hypre_StructStencilCreate(Ac_stencil_dim, Ac_stencil_size,
                                       Ac_stencil_shape);

   Ac = hypre_StructMatrixCreate(((A) -> comm),
                              coarse_grid, Ac_stencil);

   hypre_StructStencilDestroy(Ac_stencil);





   ((Ac) -> symmetric) = ((A) -> symmetric);





   Ac_num_ghost[2*cdir] = 1;
   if (!((A) -> symmetric))
   {
     Ac_num_ghost[2*cdir + 1] = 1;
   }
   hypre_StructMatrixSetNumGhost(Ac, Ac_num_ghost);

   hypre_StructMatrixInitializeShell(Ac);

   return Ac;
}





int
hypre_CycRedSetupCoarseOp( hypre_StructMatrix *A,
                           hypre_StructMatrix *Ac,
                           hypre_Index cindex,
                           hypre_Index cstride )

{
   hypre_Index index;

   hypre_StructGrid *fgrid;
   int *fgrid_ids;
   hypre_StructGrid *cgrid;
   hypre_BoxArray *cgrid_boxes;
   int *cgrid_ids;
   hypre_Box *cgrid_box;
   hypre_IndexRef cstart;
   hypre_Index stridec;
   hypre_Index fstart;
   hypre_IndexRef stridef;
   hypre_Index loop_size;

   int fi, ci;
   int loopi, loopj, loopk;

   hypre_Box *A_dbox;
   hypre_Box *Ac_dbox;

   double *a_cc, *a_cw, *a_ce;
   double *ac_cc, *ac_cw, *ac_ce;

   int iA, iAm1, iAp1;
   int iAc;

   int xOffsetA;

   int ierr = 0;

   stridef = cstride;
   ( (stridec[0]) = 1, (stridec[1]) = 1, (stridec[2]) = 1 );

   fgrid = ((A) -> grid);
   fgrid_ids = ((fgrid) -> ids);

   cgrid = ((Ac) -> grid);
   cgrid_boxes = ((cgrid) -> boxes);
   cgrid_ids = ((cgrid) -> ids);

   fi = 0;
   for (ci = 0; ci < ((cgrid_boxes) -> size); ci++)
      {
         while (fgrid_ids[fi] != cgrid_ids[ci])
         {
            fi++;
         }

         cgrid_box = &((cgrid_boxes) -> boxes[(ci)]);

         cstart = ((cgrid_box) -> imin);
         hypre_StructMapCoarseToFine(cstart, cindex, cstride, fstart);

         A_dbox = &((((A) -> data_space)) -> boxes[(fi)]);
         Ac_dbox = &((((Ac) -> data_space)) -> boxes[(ci)]);
//# 276 "cyclic_reduction.c"
         ( (index[0]) = 0, (index[1]) = 0, (index[2]) = 0 );
         a_cc = hypre_StructMatrixExtractPointerByIndex(A, fi, index);

         ( (index[0]) = -1, (index[1]) = 0, (index[2]) = 0 );
         a_cw = hypre_StructMatrixExtractPointerByIndex(A, fi, index);

         ( (index[0]) = 1, (index[1]) = 0, (index[2]) = 0 );
         a_ce = hypre_StructMatrixExtractPointerByIndex(A, fi, index);
//# 294 "cyclic_reduction.c"
         ( (index[0]) = 0, (index[1]) = 0, (index[2]) = 0 );
         ac_cc = hypre_StructMatrixExtractPointerByIndex(Ac, ci, index);

         ( (index[0]) = -1, (index[1]) = 0, (index[2]) = 0 );
         ac_cw = hypre_StructMatrixExtractPointerByIndex(Ac, ci, index);

         if(!((A) -> symmetric))
         {
            ( (index[0]) = 1, (index[1]) = 0, (index[2]) = 0 );
            ac_ce = hypre_StructMatrixExtractPointerByIndex(Ac, ci, index);
         }
//# 316 "cyclic_reduction.c"
         ( (index[0]) = 1, (index[1]) = 0, (index[2]) = 0 );
         xOffsetA = ((index[0]) + ((index[1]) + ((index[2]) * (((0)<((((((A_dbox) -> imax)[1])) - ((((A_dbox) -> imin)[1])) + 1))) ? ((((((A_dbox) -> imax)[1])) - ((((A_dbox) -> imin)[1])) + 1)) : (0)))) * (((0)<((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1))) ? ((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1)) : (0)));





         if(!((A) -> symmetric))
         {
            hypre_BoxGetSize(cgrid_box, loop_size);

            { int hypre__i1start = (((fstart[0]) - ((((A_dbox) -> imin)[0]))) + (((fstart[1]) - ((((A_dbox) -> imin)[1]))) + (((fstart[2]) - ((((A_dbox) -> imin)[2]))) * (((0)<((((((A_dbox) -> imax)[1])) - ((((A_dbox) -> imin)[1])) + 1))) ? ((((((A_dbox) -> imax)[1])) - ((((A_dbox) -> imin)[1])) + 1)) : (0)))) * (((0)<((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1))) ? ((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1)) : (0))); int hypre__i2start = (((cstart[0]) - ((((Ac_dbox) -> imin)[0]))) + (((cstart[1]) - ((((Ac_dbox) -> imin)[1]))) + (((cstart[2]) - ((((Ac_dbox) -> imin)[2]))) * (((0)<((((((Ac_dbox) -> imax)[1])) - ((((Ac_dbox) -> imin)[1])) + 1))) ? ((((((Ac_dbox) -> imax)[1])) - ((((Ac_dbox) -> imin)[1])) + 1)) : (0)))) * (((0)<((((((Ac_dbox) -> imax)[0])) - ((((Ac_dbox) -> imin)[0])) + 1))) ? ((((((Ac_dbox) -> imax)[0])) - ((((Ac_dbox) -> imin)[0])) + 1)) : (0))); int hypre__sx1 = ((stridef[0]));int hypre__sy1 = ((stridef[1])*(((0)<((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1))) ? ((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1)) : (0)));int hypre__sz1 = ((stridef[2])* (((0)<((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1))) ? ((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1)) : (0))*(((0)<((((((A_dbox) -> imax)[1])) - ((((A_dbox) -> imin)[1])) + 1))) ? ((((((A_dbox) -> imax)[1])) - ((((A_dbox) -> imin)[1])) + 1)) : (0))); int hypre__sx2 = ((stridec[0]));int hypre__sy2 = ((stridec[1])*(((0)<((((((Ac_dbox) -> imax)[0])) - ((((Ac_dbox) -> imin)[0])) + 1))) ? ((((((Ac_dbox) -> imax)[0])) - ((((Ac_dbox) -> imin)[0])) + 1)) : (0)));int hypre__sz2 = ((stridec[2])* (((0)<((((((Ac_dbox) -> imax)[0])) - ((((Ac_dbox) -> imin)[0])) + 1))) ? ((((((Ac_dbox) -> imax)[0])) - ((((Ac_dbox) -> imin)[0])) + 1)) : (0))*(((0)<((((((Ac_dbox) -> imax)[1])) - ((((Ac_dbox) -> imin)[1])) + 1))) ? ((((((Ac_dbox) -> imax)[1])) - ((((Ac_dbox) -> imin)[1])) + 1)) : (0))); int hypre__nx = (loop_size[0]);int hypre__ny = (loop_size[1]);int hypre__nz = (loop_size[2]);int hypre__mx = hypre__nx;int hypre__my = hypre__ny;int hypre__mz = hypre__nz;int hypre__dir, hypre__max;int hypre__div, hypre__mod;int hypre__block, hypre__num_blocks;hypre__dir = 0;hypre__max = hypre__nx;if (hypre__ny > hypre__max){ hypre__dir = 1; hypre__max = hypre__ny;}if (hypre__nz > hypre__max){ hypre__dir = 2; hypre__max = hypre__nz;}hypre__num_blocks = 1;if (hypre__max < hypre__num_blocks){ hypre__num_blocks = hypre__max;}if (hypre__num_blocks > 0){ hypre__div = hypre__max / hypre__num_blocks; hypre__mod = hypre__max % hypre__num_blocks;};;




//# 1 "../struct_mv/hypre_box_smp_forloop.h" 1
//# 17 "../struct_mv/hypre_box_smp_forloop.h"

//# 1 "../struct_mv/../utilities/hypre_smp_forloop.h" 1
//# 18 "../struct_mv/hypre_box_smp_forloop.h" 2
//# 332 "cyclic_reduction.c" 2
            for (hypre__block = 0; hypre__block < hypre__num_blocks; hypre__block++) { loopi = 0;loopj = 0;loopk = 0;hypre__nx = hypre__mx;hypre__ny = hypre__my;hypre__nz = hypre__mz;if (hypre__num_blocks > 1){ if (hypre__dir == 0) { loopi = hypre__block * hypre__div + (((hypre__mod)<(hypre__block)) ? (hypre__mod) : (hypre__block)); hypre__nx = hypre__div + ((hypre__mod > hypre__block) ? 1 : 0); } else if (hypre__dir == 1) { loopj = hypre__block * hypre__div + (((hypre__mod)<(hypre__block)) ? (hypre__mod) : (hypre__block)); hypre__ny = hypre__div + ((hypre__mod > hypre__block) ? 1 : 0); } else if (hypre__dir == 2) { loopk = hypre__block * hypre__div + (((hypre__mod)<(hypre__block)) ? (hypre__mod) : (hypre__block)); hypre__nz = hypre__div + ((hypre__mod > hypre__block) ? 1 : 0); }}; iA = hypre__i1start + loopi*hypre__sx1 + loopj*hypre__sy1 + loopk*hypre__sz1; iAc = hypre__i2start + loopi*hypre__sx2 + loopj*hypre__sy2 + loopk*hypre__sz2; for (loopk = 0; loopk < hypre__nz; loopk++) { for (loopj = 0; loopj < hypre__ny; loopj++) { for (loopi = 0; loopi < hypre__nx; loopi++) {
               {
                  iAm1 = iA - xOffsetA;
                  iAp1 = iA + xOffsetA;

                  ac_cw[iAc] = - a_cw[iA] *a_cw[iAm1] / a_cc[iAm1];

                  ac_cc[iAc] = a_cc[iA]
                     - a_cw[iA] * a_ce[iAm1] / a_cc[iAm1]
                     - a_ce[iA] * a_cw[iAp1] / a_cc[iAp1];

                  ac_ce[iAc] = - a_ce[iA] *a_ce[iAp1] / a_cc[iAp1];

               }
            iA += hypre__sx1; iAc += hypre__sx2; } iA += hypre__sy1 - hypre__nx*hypre__sx1; iAc += hypre__sy2 - hypre__nx*hypre__sx2; } iA += hypre__sz1 - hypre__ny*hypre__sy1; iAc += hypre__sz2 - hypre__ny*hypre__sy2; } }};
         }





         else
         {
            hypre_BoxGetSize(cgrid_box, loop_size);

            { int hypre__i1start = (((fstart[0]) - ((((A_dbox) -> imin)[0]))) + (((fstart[1]) - ((((A_dbox) -> imin)[1]))) + (((fstart[2]) - ((((A_dbox) -> imin)[2]))) * (((0)<((((((A_dbox) -> imax)[1])) - ((((A_dbox) -> imin)[1])) + 1))) ? ((((((A_dbox) -> imax)[1])) - ((((A_dbox) -> imin)[1])) + 1)) : (0)))) * (((0)<((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1))) ? ((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1)) : (0))); int hypre__i2start = (((cstart[0]) - ((((Ac_dbox) -> imin)[0]))) + (((cstart[1]) - ((((Ac_dbox) -> imin)[1]))) + (((cstart[2]) - ((((Ac_dbox) -> imin)[2]))) * (((0)<((((((Ac_dbox) -> imax)[1])) - ((((Ac_dbox) -> imin)[1])) + 1))) ? ((((((Ac_dbox) -> imax)[1])) - ((((Ac_dbox) -> imin)[1])) + 1)) : (0)))) * (((0)<((((((Ac_dbox) -> imax)[0])) - ((((Ac_dbox) -> imin)[0])) + 1))) ? ((((((Ac_dbox) -> imax)[0])) - ((((Ac_dbox) -> imin)[0])) + 1)) : (0))); int hypre__sx1 = ((stridef[0]));int hypre__sy1 = ((stridef[1])*(((0)<((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1))) ? ((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1)) : (0)));int hypre__sz1 = ((stridef[2])* (((0)<((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1))) ? ((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1)) : (0))*(((0)<((((((A_dbox) -> imax)[1])) - ((((A_dbox) -> imin)[1])) + 1))) ? ((((((A_dbox) -> imax)[1])) - ((((A_dbox) -> imin)[1])) + 1)) : (0))); int hypre__sx2 = ((stridec[0]));int hypre__sy2 = ((stridec[1])*(((0)<((((((Ac_dbox) -> imax)[0])) - ((((Ac_dbox) -> imin)[0])) + 1))) ? ((((((Ac_dbox) -> imax)[0])) - ((((Ac_dbox) -> imin)[0])) + 1)) : (0)));int hypre__sz2 = ((stridec[2])* (((0)<((((((Ac_dbox) -> imax)[0])) - ((((Ac_dbox) -> imin)[0])) + 1))) ? ((((((Ac_dbox) -> imax)[0])) - ((((Ac_dbox) -> imin)[0])) + 1)) : (0))*(((0)<((((((Ac_dbox) -> imax)[1])) - ((((Ac_dbox) -> imin)[1])) + 1))) ? ((((((Ac_dbox) -> imax)[1])) - ((((Ac_dbox) -> imin)[1])) + 1)) : (0))); int hypre__nx = (loop_size[0]);int hypre__ny = (loop_size[1]);int hypre__nz = (loop_size[2]);int hypre__mx = hypre__nx;int hypre__my = hypre__ny;int hypre__mz = hypre__nz;int hypre__dir, hypre__max;int hypre__div, hypre__mod;int hypre__block, hypre__num_blocks;hypre__dir = 0;hypre__max = hypre__nx;if (hypre__ny > hypre__max){ hypre__dir = 1; hypre__max = hypre__ny;}if (hypre__nz > hypre__max){ hypre__dir = 2; hypre__max = hypre__nz;}hypre__num_blocks = 1;if (hypre__max < hypre__num_blocks){ hypre__num_blocks = hypre__max;}if (hypre__num_blocks > 0){ hypre__div = hypre__max / hypre__num_blocks; hypre__mod = hypre__max % hypre__num_blocks;};;




//# 1 "../struct_mv/hypre_box_smp_forloop.h" 1
//# 17 "../struct_mv/hypre_box_smp_forloop.h"

//# 1 "../struct_mv/../utilities/hypre_smp_forloop.h" 1
//# 18 "../struct_mv/hypre_box_smp_forloop.h" 2
//# 362 "cyclic_reduction.c" 2
            for (hypre__block = 0; hypre__block < hypre__num_blocks; hypre__block++) { loopi = 0;loopj = 0;loopk = 0;hypre__nx = hypre__mx;hypre__ny = hypre__my;hypre__nz = hypre__mz;if (hypre__num_blocks > 1){ if (hypre__dir == 0) { loopi = hypre__block * hypre__div + (((hypre__mod)<(hypre__block)) ? (hypre__mod) : (hypre__block)); hypre__nx = hypre__div + ((hypre__mod > hypre__block) ? 1 : 0); } else if (hypre__dir == 1) { loopj = hypre__block * hypre__div + (((hypre__mod)<(hypre__block)) ? (hypre__mod) : (hypre__block)); hypre__ny = hypre__div + ((hypre__mod > hypre__block) ? 1 : 0); } else if (hypre__dir == 2) { loopk = hypre__block * hypre__div + (((hypre__mod)<(hypre__block)) ? (hypre__mod) : (hypre__block)); hypre__nz = hypre__div + ((hypre__mod > hypre__block) ? 1 : 0); }}; iA = hypre__i1start + loopi*hypre__sx1 + loopj*hypre__sy1 + loopk*hypre__sz1; iAc = hypre__i2start + loopi*hypre__sx2 + loopj*hypre__sy2 + loopk*hypre__sz2; for (loopk = 0; loopk < hypre__nz; loopk++) { for (loopj = 0; loopj < hypre__ny; loopj++) { for (loopi = 0; loopi < hypre__nx; loopi++) {
               {
                  iAm1 = iA - xOffsetA;
                  iAp1 = iA + xOffsetA;

                  ac_cw[iAc] = - a_cw[iA] *a_cw[iAm1] / a_cc[iAm1];

                  ac_cc[iAc] = a_cc[iA]
                     - a_cw[iA] * a_ce[iAm1] / a_cc[iAm1]
                     - a_ce[iA] * a_cw[iAp1] / a_cc[iAp1];
               }
            iA += hypre__sx1; iAc += hypre__sx2; } iA += hypre__sy1 - hypre__nx*hypre__sx1; iAc += hypre__sy2 - hypre__nx*hypre__sx2; } iA += hypre__sz1 - hypre__ny*hypre__sy1; iAc += hypre__sz2 - hypre__ny*hypre__sy2; } }};
         }

      }

   hypre_StructMatrixAssemble(Ac);





   if ((((cgrid) -> periodic)[0]) == 1)
   {
      for (ci = 0; ci < ((cgrid_boxes) -> size); ci++)
      {
         cgrid_box = &((cgrid_boxes) -> boxes[(ci)]);

         cstart = ((cgrid_box) -> imin);

         Ac_dbox = &((((Ac) -> data_space)) -> boxes[(ci)]);
//# 403 "cyclic_reduction.c"
         ( (index[0]) = 0, (index[1]) = 0, (index[2]) = 0 );
         ac_cc = hypre_StructMatrixExtractPointerByIndex(Ac, ci, index);

         ( (index[0]) = -1, (index[1]) = 0, (index[2]) = 0 );
         ac_cw = hypre_StructMatrixExtractPointerByIndex(Ac, ci, index);

         if(!((A) -> symmetric))
         {
            ( (index[0]) = 1, (index[1]) = 0, (index[2]) = 0 );
            ac_ce = hypre_StructMatrixExtractPointerByIndex(Ac, ci, index);
         }






         if(!((A) -> symmetric))
         {
            hypre_BoxGetSize(cgrid_box, loop_size);

            { int hypre__i1start = (((cstart[0]) - ((((Ac_dbox) -> imin)[0]))) + (((cstart[1]) - ((((Ac_dbox) -> imin)[1]))) + (((cstart[2]) - ((((Ac_dbox) -> imin)[2]))) * (((0)<((((((Ac_dbox) -> imax)[1])) - ((((Ac_dbox) -> imin)[1])) + 1))) ? ((((((Ac_dbox) -> imax)[1])) - ((((Ac_dbox) -> imin)[1])) + 1)) : (0)))) * (((0)<((((((Ac_dbox) -> imax)[0])) - ((((Ac_dbox) -> imin)[0])) + 1))) ? ((((((Ac_dbox) -> imax)[0])) - ((((Ac_dbox) -> imin)[0])) + 1)) : (0))); int hypre__sx1 = ((stridec[0]));int hypre__sy1 = ((stridec[1])*(((0)<((((((Ac_dbox) -> imax)[0])) - ((((Ac_dbox) -> imin)[0])) + 1))) ? ((((((Ac_dbox) -> imax)[0])) - ((((Ac_dbox) -> imin)[0])) + 1)) : (0)));int hypre__sz1 = ((stridec[2])* (((0)<((((((Ac_dbox) -> imax)[0])) - ((((Ac_dbox) -> imin)[0])) + 1))) ? ((((((Ac_dbox) -> imax)[0])) - ((((Ac_dbox) -> imin)[0])) + 1)) : (0))*(((0)<((((((Ac_dbox) -> imax)[1])) - ((((Ac_dbox) -> imin)[1])) + 1))) ? ((((((Ac_dbox) -> imax)[1])) - ((((Ac_dbox) -> imin)[1])) + 1)) : (0))); int hypre__nx = (loop_size[0]);int hypre__ny = (loop_size[1]);int hypre__nz = (loop_size[2]);int hypre__mx = hypre__nx;int hypre__my = hypre__ny;int hypre__mz = hypre__nz;int hypre__dir, hypre__max;int hypre__div, hypre__mod;int hypre__block, hypre__num_blocks;hypre__dir = 0;hypre__max = hypre__nx;if (hypre__ny > hypre__max){ hypre__dir = 1; hypre__max = hypre__ny;}if (hypre__nz > hypre__max){ hypre__dir = 2; hypre__max = hypre__nz;}hypre__num_blocks = 1;if (hypre__max < hypre__num_blocks){ hypre__num_blocks = hypre__max;}if (hypre__num_blocks > 0){ hypre__div = hypre__max / hypre__num_blocks; hypre__mod = hypre__max % hypre__num_blocks;};;



//# 1 "../struct_mv/hypre_box_smp_forloop.h" 1
//# 17 "../struct_mv/hypre_box_smp_forloop.h"

//# 1 "../struct_mv/../utilities/hypre_smp_forloop.h" 1
//# 18 "../struct_mv/hypre_box_smp_forloop.h" 2
//# 428 "cyclic_reduction.c" 2
            for (hypre__block = 0; hypre__block < hypre__num_blocks; hypre__block++) { loopi = 0;loopj = 0;loopk = 0;hypre__nx = hypre__mx;hypre__ny = hypre__my;hypre__nz = hypre__mz;if (hypre__num_blocks > 1){ if (hypre__dir == 0) { loopi = hypre__block * hypre__div + (((hypre__mod)<(hypre__block)) ? (hypre__mod) : (hypre__block)); hypre__nx = hypre__div + ((hypre__mod > hypre__block) ? 1 : 0); } else if (hypre__dir == 1) { loopj = hypre__block * hypre__div + (((hypre__mod)<(hypre__block)) ? (hypre__mod) : (hypre__block)); hypre__ny = hypre__div + ((hypre__mod > hypre__block) ? 1 : 0); } else if (hypre__dir == 2) { loopk = hypre__block * hypre__div + (((hypre__mod)<(hypre__block)) ? (hypre__mod) : (hypre__block)); hypre__nz = hypre__div + ((hypre__mod > hypre__block) ? 1 : 0); }}; iAc = hypre__i1start + loopi*hypre__sx1 + loopj*hypre__sy1 + loopk*hypre__sz1; for (loopk = 0; loopk < hypre__nz; loopk++) { for (loopj = 0; loopj < hypre__ny; loopj++) { for (loopi = 0; loopi < hypre__nx; loopi++) {
               {
                  ac_cc[iAc] += (ac_cw[iAc] + ac_ce[iAc]);
                  ac_cw[iAc] = 0.0;
                  ac_ce[iAc] = 0.0;
               }
            iAc += hypre__sx1; } iAc += hypre__sy1 - hypre__nx*hypre__sx1; } iAc += hypre__sz1 - hypre__ny*hypre__sy1; } }};
         }





         else
         {
            hypre_BoxGetSize(cgrid_box, loop_size);

            { int hypre__i1start = (((cstart[0]) - ((((Ac_dbox) -> imin)[0]))) + (((cstart[1]) - ((((Ac_dbox) -> imin)[1]))) + (((cstart[2]) - ((((Ac_dbox) -> imin)[2]))) * (((0)<((((((Ac_dbox) -> imax)[1])) - ((((Ac_dbox) -> imin)[1])) + 1))) ? ((((((Ac_dbox) -> imax)[1])) - ((((Ac_dbox) -> imin)[1])) + 1)) : (0)))) * (((0)<((((((Ac_dbox) -> imax)[0])) - ((((Ac_dbox) -> imin)[0])) + 1))) ? ((((((Ac_dbox) -> imax)[0])) - ((((Ac_dbox) -> imin)[0])) + 1)) : (0))); int hypre__sx1 = ((stridec[0]));int hypre__sy1 = ((stridec[1])*(((0)<((((((Ac_dbox) -> imax)[0])) - ((((Ac_dbox) -> imin)[0])) + 1))) ? ((((((Ac_dbox) -> imax)[0])) - ((((Ac_dbox) -> imin)[0])) + 1)) : (0)));int hypre__sz1 = ((stridec[2])* (((0)<((((((Ac_dbox) -> imax)[0])) - ((((Ac_dbox) -> imin)[0])) + 1))) ? ((((((Ac_dbox) -> imax)[0])) - ((((Ac_dbox) -> imin)[0])) + 1)) : (0))*(((0)<((((((Ac_dbox) -> imax)[1])) - ((((Ac_dbox) -> imin)[1])) + 1))) ? ((((((Ac_dbox) -> imax)[1])) - ((((Ac_dbox) -> imin)[1])) + 1)) : (0))); int hypre__nx = (loop_size[0]);int hypre__ny = (loop_size[1]);int hypre__nz = (loop_size[2]);int hypre__mx = hypre__nx;int hypre__my = hypre__ny;int hypre__mz = hypre__nz;int hypre__dir, hypre__max;int hypre__div, hypre__mod;int hypre__block, hypre__num_blocks;hypre__dir = 0;hypre__max = hypre__nx;if (hypre__ny > hypre__max){ hypre__dir = 1; hypre__max = hypre__ny;}if (hypre__nz > hypre__max){ hypre__dir = 2; hypre__max = hypre__nz;}hypre__num_blocks = 1;if (hypre__max < hypre__num_blocks){ hypre__num_blocks = hypre__max;}if (hypre__num_blocks > 0){ hypre__div = hypre__max / hypre__num_blocks; hypre__mod = hypre__max % hypre__num_blocks;};;



//# 1 "../struct_mv/hypre_box_smp_forloop.h" 1
//# 17 "../struct_mv/hypre_box_smp_forloop.h"

//# 1 "../struct_mv/../utilities/hypre_smp_forloop.h" 1
//# 18 "../struct_mv/hypre_box_smp_forloop.h" 2
//# 449 "cyclic_reduction.c" 2
            for (hypre__block = 0; hypre__block < hypre__num_blocks; hypre__block++) { loopi = 0;loopj = 0;loopk = 0;hypre__nx = hypre__mx;hypre__ny = hypre__my;hypre__nz = hypre__mz;if (hypre__num_blocks > 1){ if (hypre__dir == 0) { loopi = hypre__block * hypre__div + (((hypre__mod)<(hypre__block)) ? (hypre__mod) : (hypre__block)); hypre__nx = hypre__div + ((hypre__mod > hypre__block) ? 1 : 0); } else if (hypre__dir == 1) { loopj = hypre__block * hypre__div + (((hypre__mod)<(hypre__block)) ? (hypre__mod) : (hypre__block)); hypre__ny = hypre__div + ((hypre__mod > hypre__block) ? 1 : 0); } else if (hypre__dir == 2) { loopk = hypre__block * hypre__div + (((hypre__mod)<(hypre__block)) ? (hypre__mod) : (hypre__block)); hypre__nz = hypre__div + ((hypre__mod > hypre__block) ? 1 : 0); }}; iAc = hypre__i1start + loopi*hypre__sx1 + loopj*hypre__sy1 + loopk*hypre__sz1; for (loopk = 0; loopk < hypre__nz; loopk++) { for (loopj = 0; loopj < hypre__ny; loopj++) { for (loopi = 0; loopi < hypre__nx; loopi++) {
               {
                  ac_cc[iAc] += (2.0 * ac_cw[iAc]);
                  ac_cw[iAc] = 0.0;
               }
            iAc += hypre__sx1; } iAc += hypre__sy1 - hypre__nx*hypre__sx1; } iAc += hypre__sz1 - hypre__ny*hypre__sy1; } }};
         }

      }

   }

   hypre_StructMatrixAssemble(Ac);

   return ierr;
}





int
hypre_CyclicReductionSetup( void *cyc_red_vdata,
                            hypre_StructMatrix *A,
                            hypre_StructVector *b,
                            hypre_StructVector *x )
{
   hypre_CyclicReductionData *cyc_red_data = cyc_red_vdata;

   MPI_Comm comm = (cyc_red_data -> comm);
   int cdir = (cyc_red_data -> cdir);
   hypre_IndexRef base_index = (cyc_red_data -> base_index);
   hypre_IndexRef base_stride = (cyc_red_data -> base_stride);

   int num_levels;
   hypre_StructGrid **grid_l;
   hypre_BoxArray *base_points;
   hypre_BoxArray **fine_points_l;
   double *data;
   int data_size = 0;
   hypre_StructMatrix **A_l;
   hypre_StructVector **x_l;
   hypre_ComputePkg **down_compute_pkg_l;
   hypre_ComputePkg **up_compute_pkg_l;

   hypre_Index cindex;
   hypre_Index findex;
   hypre_Index stride;

   hypre_BoxArrayArray *send_boxes;
   hypre_BoxArrayArray *recv_boxes;
   int **send_processes;
   int **recv_processes;
   hypre_BoxArrayArray *indt_boxes;
   hypre_BoxArrayArray *dept_boxes;

   hypre_StructGrid *grid;

   hypre_Box *cbox;

   int l;
   int flop_divisor;

   int x_num_ghost[] = {0, 0, 0, 0, 0, 0};

   int ierr = 0;





   grid = ((A) -> grid);


   cbox = hypre_BoxDuplicate(((grid) -> bounding_box));
   num_levels = hypre_Log2((((0)<((((((cbox) -> imax)[cdir])) - ((((cbox) -> imin)[cdir])) + 1))) ? ((((((cbox) -> imax)[cdir])) - ((((cbox) -> imin)[cdir])) + 1)) : (0))) + 2;

   grid_l = ( (hypre_StructGrid * *)hypre_MAlloc((unsigned int)(sizeof(hypre_StructGrid *) * (num_levels))) );
   hypre_StructGridRef(grid, &grid_l[0]);
   for (l = 0; ; l++)
   {

      { if (l > 0) ( (cindex[0]) = 0, (cindex[1]) = 0, (cindex[2]) = 0 ); else ( (cindex[0]) = (base_index[0]), (cindex[1]) = (base_index[1]), (cindex[2]) = (base_index[2]) ); (cindex[cdir]) += 0;};
      { if (l > 0) ( (stride[0]) = 1, (stride[1]) = 1, (stride[2]) = 1 ); else ( (stride[0]) = (base_stride[0]), (stride[1]) = (base_stride[1]), (stride[2]) = (base_stride[2]) ); (stride[cdir]) *= 2;};


      if ( ((((cbox) -> imin)[cdir])) == ((((cbox) -> imax)[cdir])) )
      {

         break;
      }


      hypre_ProjectBox(cbox, cindex, stride);
      hypre_StructMapFineToCoarse(((cbox) -> imin), cindex, stride,
                                  ((cbox) -> imin));
      hypre_StructMapFineToCoarse(((cbox) -> imax), cindex, stride,
                                  ((cbox) -> imax));


      hypre_StructCoarsen(grid_l[l], cindex, stride, 1, &grid_l[l+1]);
   }
   num_levels = l + 1;


   hypre_BoxDestroy(cbox);

   (cyc_red_data -> num_levels) = num_levels;
   (cyc_red_data -> grid_l) = grid_l;





   base_points = hypre_BoxArrayDuplicate(((grid_l[0]) -> boxes));
   hypre_ProjectBoxArray(base_points, base_index, base_stride);

   (cyc_red_data -> base_points) = base_points;





   fine_points_l = ( (hypre_BoxArray * *)hypre_MAlloc((unsigned int)(sizeof(hypre_BoxArray *) * (num_levels))) );

   for (l = 0; l < (num_levels - 1); l++)
   {
      { if (l > 0) ( (cindex[0]) = 0, (cindex[1]) = 0, (cindex[2]) = 0 ); else ( (cindex[0]) = (base_index[0]), (cindex[1]) = (base_index[1]), (cindex[2]) = (base_index[2]) ); (cindex[cdir]) += 0;};
      { if (l > 0) ( (findex[0]) = 0, (findex[1]) = 0, (findex[2]) = 0 ); else ( (findex[0]) = (base_index[0]), (findex[1]) = (base_index[1]), (findex[2]) = (base_index[2]) ); (findex[cdir]) += 1;};
      { if (l > 0) ( (stride[0]) = 1, (stride[1]) = 1, (stride[2]) = 1 ); else ( (stride[0]) = (base_stride[0]), (stride[1]) = (base_stride[1]), (stride[2]) = (base_stride[2]) ); (stride[cdir]) *= 2;};

      fine_points_l[l] =
         hypre_BoxArrayDuplicate(((grid_l[l]) -> boxes));
      hypre_ProjectBoxArray(fine_points_l[l], findex, stride);
   }

   fine_points_l[l] =
      hypre_BoxArrayDuplicate(((grid_l[l]) -> boxes));
   if (num_levels == 1)
   {
      hypre_ProjectBoxArray(fine_points_l[l], base_index, base_stride);
   }

   (cyc_red_data -> fine_points_l) = fine_points_l;





   A_l = ( (hypre_StructMatrix * *)hypre_MAlloc((unsigned int)(sizeof(hypre_StructMatrix *) * (num_levels))) );
   x_l = ( (hypre_StructVector * *)hypre_MAlloc((unsigned int)(sizeof(hypre_StructVector *) * (num_levels))) );

   A_l[0] = hypre_StructMatrixRef(A);
   x_l[0] = hypre_StructVectorRef(x);

   x_num_ghost[2*cdir] = 1;
   x_num_ghost[2*cdir + 1] = 1;

   for (l = 0; l < (num_levels - 1); l++)
   {
      A_l[l+1] = hypre_CycRedCreateCoarseOp(A_l[l], grid_l[l+1], cdir);
      data_size += ((A_l[l+1]) -> data_size);

      x_l[l+1] = hypre_StructVectorCreate(comm, grid_l[l+1]);
      hypre_StructVectorSetNumGhost(x_l[l+1], x_num_ghost);
      hypre_StructVectorInitializeShell(x_l[l+1]);
      data_size += ((x_l[l+1]) -> data_size);
   }

   data = ( (double *)hypre_CAlloc((unsigned int)((data_size)), (unsigned int)sizeof(double)) );

   (cyc_red_data -> data) = data;

   for (l = 0; l < (num_levels - 1); l++)
   {
      hypre_StructMatrixInitializeData(A_l[l+1], data);
      data += ((A_l[l+1]) -> data_size);
      hypre_StructVectorInitializeData(x_l[l+1], data);
      hypre_StructVectorAssemble(x_l[l+1]);
      data += ((x_l[l+1]) -> data_size);
   }

   (cyc_red_data -> A_l) = A_l;
   (cyc_red_data -> x_l) = x_l;





   for (l = 0; l < (num_levels - 1); l++)
   {
      { if (l > 0) ( (cindex[0]) = 0, (cindex[1]) = 0, (cindex[2]) = 0 ); else ( (cindex[0]) = (base_index[0]), (cindex[1]) = (base_index[1]), (cindex[2]) = (base_index[2]) ); (cindex[cdir]) += 0;};
      { if (l > 0) ( (stride[0]) = 1, (stride[1]) = 1, (stride[2]) = 1 ); else ( (stride[0]) = (base_stride[0]), (stride[1]) = (base_stride[1]), (stride[2]) = (base_stride[2]) ); (stride[cdir]) *= 2;};

      hypre_CycRedSetupCoarseOp(A_l[l], A_l[l+1], cindex, stride);
   }





   down_compute_pkg_l = ( (hypre_ComputePkg * *)hypre_MAlloc((unsigned int)(sizeof(hypre_ComputePkg *) * ((num_levels - 1)))) );
   up_compute_pkg_l = ( (hypre_ComputePkg * *)hypre_MAlloc((unsigned int)(sizeof(hypre_ComputePkg *) * ((num_levels - 1)))) );

   for (l = 0; l < (num_levels - 1); l++)
   {
      { if (l > 0) ( (cindex[0]) = 0, (cindex[1]) = 0, (cindex[2]) = 0 ); else ( (cindex[0]) = (base_index[0]), (cindex[1]) = (base_index[1]), (cindex[2]) = (base_index[2]) ); (cindex[cdir]) += 0;};
      { if (l > 0) ( (findex[0]) = 0, (findex[1]) = 0, (findex[2]) = 0 ); else ( (findex[0]) = (base_index[0]), (findex[1]) = (base_index[1]), (findex[2]) = (base_index[2]) ); (findex[cdir]) += 1;};
      { if (l > 0) ( (stride[0]) = 1, (stride[1]) = 1, (stride[2]) = 1 ); else ( (stride[0]) = (base_stride[0]), (stride[1]) = (base_stride[1]), (stride[2]) = (base_stride[2]) ); (stride[cdir]) *= 2;};

      hypre_CreateComputeInfo(grid_l[l], ((A_l[l]) -> stencil),
                              &send_boxes, &recv_boxes,
                              &send_processes, &recv_processes,
                              &indt_boxes, &dept_boxes);


      hypre_ProjectBoxArrayArray(send_boxes, findex, stride);
      hypre_ProjectBoxArrayArray(recv_boxes, findex, stride);
      hypre_ProjectBoxArrayArray(indt_boxes, cindex, stride);
      hypre_ProjectBoxArrayArray(dept_boxes, cindex, stride);
      hypre_ComputePkgCreate(send_boxes, recv_boxes,
                             stride, stride,
                             send_processes, recv_processes,
                             indt_boxes, dept_boxes,
                             stride, grid_l[l],
                             ((x_l[l]) -> data_space), 1,
                             &down_compute_pkg_l[l]);

      hypre_CreateComputeInfo(grid_l[l], ((A_l[l]) -> stencil),
                              &send_boxes, &recv_boxes,
                              &send_processes, &recv_processes,
                              &indt_boxes, &dept_boxes);


      hypre_ProjectBoxArrayArray(send_boxes, cindex, stride);
      hypre_ProjectBoxArrayArray(recv_boxes, cindex, stride);
      hypre_ProjectBoxArrayArray(indt_boxes, findex, stride);
      hypre_ProjectBoxArrayArray(dept_boxes, findex, stride);
      hypre_ComputePkgCreate(send_boxes, recv_boxes,
                             stride, stride,
                             send_processes, recv_processes,
                             indt_boxes, dept_boxes,
                             stride, grid_l[l],
                             ((x_l[l]) -> data_space), 1,
                             &up_compute_pkg_l[l]);
   }

   (cyc_red_data -> down_compute_pkg_l) = down_compute_pkg_l;
   (cyc_red_data -> up_compute_pkg_l) = up_compute_pkg_l;





   flop_divisor = ((base_stride[0]) *
                   (base_stride[1]) *
                   (base_stride[2]) );
   (cyc_red_data -> solve_flops) =
      ((x_l[0]) -> global_size)/2/flop_divisor;
   (cyc_red_data -> solve_flops) +=
      5*((x_l[0]) -> global_size)/2/flop_divisor;
   for (l = 1; l < (num_levels - 1); l++)
   {
      (cyc_red_data -> solve_flops) +=
         10*((x_l[l]) -> global_size)/2;
   }

   if (num_levels > 1)
   {
      (cyc_red_data -> solve_flops) +=
          ((x_l[l]) -> global_size)/2;
   }
//# 740 "cyclic_reduction.c"
   return ierr;
}
//# 752 "cyclic_reduction.c"
int
hypre_CyclicReduction( void *cyc_red_vdata,
                       hypre_StructMatrix *A,
                       hypre_StructVector *b,
                       hypre_StructVector *x )
{
   hypre_CyclicReductionData *cyc_red_data = cyc_red_vdata;

   int num_levels = (cyc_red_data -> num_levels);
   int cdir = (cyc_red_data -> cdir);
   hypre_IndexRef base_index = (cyc_red_data -> base_index);
   hypre_IndexRef base_stride = (cyc_red_data -> base_stride);
   hypre_BoxArray *base_points = (cyc_red_data -> base_points);
   hypre_BoxArray **fine_points_l = (cyc_red_data -> fine_points_l);
   hypre_StructMatrix **A_l = (cyc_red_data -> A_l);
   hypre_StructVector **x_l = (cyc_red_data -> x_l);
   hypre_ComputePkg **down_compute_pkg_l =
      (cyc_red_data -> down_compute_pkg_l);
   hypre_ComputePkg **up_compute_pkg_l =
      (cyc_red_data -> up_compute_pkg_l);

   hypre_StructGrid *fgrid;
   int *fgrid_ids;
   hypre_StructGrid *cgrid;
   hypre_BoxArray *cgrid_boxes;
   int *cgrid_ids;

   hypre_CommHandle *comm_handle;

   hypre_BoxArrayArray *compute_box_aa;
   hypre_BoxArray *compute_box_a;
   hypre_Box *compute_box;

   hypre_Box *A_dbox;
   hypre_Box *x_dbox;
   hypre_Box *b_dbox;
   hypre_Box *xc_dbox;

   double *Ap, *Awp, *Aep;
   double *xp, *xwp, *xep;
   double *bp;
   double *xcp;

   int Ai;
   int xi;
   int bi;
   int xci;

   hypre_Index cindex;
   hypre_Index stride;

   hypre_Index index;
   hypre_Index loop_size;
   hypre_Index start;
   hypre_Index startc;
   hypre_Index stridec;

   int compute_i, fi, ci, j, l;
   int loopi, loopj, loopk;

   int ierr = 0;

   ;






   ( (stridec[0]) = 1, (stridec[1]) = 1, (stridec[2]) = 1 );

   hypre_StructMatrixDestroy(A_l[0]);
   hypre_StructVectorDestroy(x_l[0]);
   A_l[0] = hypre_StructMatrixRef(A);
   x_l[0] = hypre_StructVectorRef(x);





   compute_box_a = base_points;
   for (fi = 0; fi < ((compute_box_a) -> size); fi++)
      {
         compute_box = &((compute_box_a) -> boxes[(fi)]);

         x_dbox = &((((x) -> data_space)) -> boxes[(fi)]);
         b_dbox = &((((b) -> data_space)) -> boxes[(fi)]);

         xp = (((x) -> data) + ((x) -> data_indices)[fi]);
         bp = (((b) -> data) + ((b) -> data_indices)[fi]);

         ( (start[0]) = (((compute_box) -> imin)[0]), (start[1]) = (((compute_box) -> imin)[1]), (start[2]) = (((compute_box) -> imin)[2]) );
         hypre_BoxGetStrideSize(compute_box, base_stride, loop_size);

         { int hypre__i1start = (((start[0]) - ((((x_dbox) -> imin)[0]))) + (((start[1]) - ((((x_dbox) -> imin)[1]))) + (((start[2]) - ((((x_dbox) -> imin)[2]))) * (((0)<((((((x_dbox) -> imax)[1])) - ((((x_dbox) -> imin)[1])) + 1))) ? ((((((x_dbox) -> imax)[1])) - ((((x_dbox) -> imin)[1])) + 1)) : (0)))) * (((0)<((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1))) ? ((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1)) : (0))); int hypre__i2start = (((start[0]) - ((((b_dbox) -> imin)[0]))) + (((start[1]) - ((((b_dbox) -> imin)[1]))) + (((start[2]) - ((((b_dbox) -> imin)[2]))) * (((0)<((((((b_dbox) -> imax)[1])) - ((((b_dbox) -> imin)[1])) + 1))) ? ((((((b_dbox) -> imax)[1])) - ((((b_dbox) -> imin)[1])) + 1)) : (0)))) * (((0)<((((((b_dbox) -> imax)[0])) - ((((b_dbox) -> imin)[0])) + 1))) ? ((((((b_dbox) -> imax)[0])) - ((((b_dbox) -> imin)[0])) + 1)) : (0))); int hypre__sx1 = ((base_stride[0]));int hypre__sy1 = ((base_stride[1])*(((0)<((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1))) ? ((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1)) : (0)));int hypre__sz1 = ((base_stride[2])* (((0)<((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1))) ? ((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1)) : (0))*(((0)<((((((x_dbox) -> imax)[1])) - ((((x_dbox) -> imin)[1])) + 1))) ? ((((((x_dbox) -> imax)[1])) - ((((x_dbox) -> imin)[1])) + 1)) : (0))); int hypre__sx2 = ((base_stride[0]));int hypre__sy2 = ((base_stride[1])*(((0)<((((((b_dbox) -> imax)[0])) - ((((b_dbox) -> imin)[0])) + 1))) ? ((((((b_dbox) -> imax)[0])) - ((((b_dbox) -> imin)[0])) + 1)) : (0)));int hypre__sz2 = ((base_stride[2])* (((0)<((((((b_dbox) -> imax)[0])) - ((((b_dbox) -> imin)[0])) + 1))) ? ((((((b_dbox) -> imax)[0])) - ((((b_dbox) -> imin)[0])) + 1)) : (0))*(((0)<((((((b_dbox) -> imax)[1])) - ((((b_dbox) -> imin)[1])) + 1))) ? ((((((b_dbox) -> imax)[1])) - ((((b_dbox) -> imin)[1])) + 1)) : (0))); int hypre__nx = (loop_size[0]);int hypre__ny = (loop_size[1]);int hypre__nz = (loop_size[2]);int hypre__mx = hypre__nx;int hypre__my = hypre__ny;int hypre__mz = hypre__nz;int hypre__dir, hypre__max;int hypre__div, hypre__mod;int hypre__block, hypre__num_blocks;hypre__dir = 0;hypre__max = hypre__nx;if (hypre__ny > hypre__max){ hypre__dir = 1; hypre__max = hypre__ny;}if (hypre__nz > hypre__max){ hypre__dir = 2; hypre__max = hypre__nz;}hypre__num_blocks = 1;if (hypre__max < hypre__num_blocks){ hypre__num_blocks = hypre__max;}if (hypre__num_blocks > 0){ hypre__div = hypre__max / hypre__num_blocks; hypre__mod = hypre__max % hypre__num_blocks;};;




//# 1 "../struct_mv/hypre_box_smp_forloop.h" 1
//# 17 "../struct_mv/hypre_box_smp_forloop.h"

//# 1 "../struct_mv/../utilities/hypre_smp_forloop.h" 1
//# 18 "../struct_mv/hypre_box_smp_forloop.h" 2
//# 851 "cyclic_reduction.c" 2
         for (hypre__block = 0; hypre__block < hypre__num_blocks; hypre__block++) { loopi = 0;loopj = 0;loopk = 0;hypre__nx = hypre__mx;hypre__ny = hypre__my;hypre__nz = hypre__mz;if (hypre__num_blocks > 1){ if (hypre__dir == 0) { loopi = hypre__block * hypre__div + (((hypre__mod)<(hypre__block)) ? (hypre__mod) : (hypre__block)); hypre__nx = hypre__div + ((hypre__mod > hypre__block) ? 1 : 0); } else if (hypre__dir == 1) { loopj = hypre__block * hypre__div + (((hypre__mod)<(hypre__block)) ? (hypre__mod) : (hypre__block)); hypre__ny = hypre__div + ((hypre__mod > hypre__block) ? 1 : 0); } else if (hypre__dir == 2) { loopk = hypre__block * hypre__div + (((hypre__mod)<(hypre__block)) ? (hypre__mod) : (hypre__block)); hypre__nz = hypre__div + ((hypre__mod > hypre__block) ? 1 : 0); }}; xi = hypre__i1start + loopi*hypre__sx1 + loopj*hypre__sy1 + loopk*hypre__sz1; bi = hypre__i2start + loopi*hypre__sx2 + loopj*hypre__sy2 + loopk*hypre__sz2; for (loopk = 0; loopk < hypre__nz; loopk++) { for (loopj = 0; loopj < hypre__ny; loopj++) { for (loopi = 0; loopi < hypre__nx; loopi++) {
            {
               xp[xi] = bp[bi];
            }
         xi += hypre__sx1; bi += hypre__sx2; } xi += hypre__sy1 - hypre__nx*hypre__sx1; bi += hypre__sy2 - hypre__nx*hypre__sx2; } xi += hypre__sz1 - hypre__ny*hypre__sy1; bi += hypre__sz2 - hypre__ny*hypre__sy2; } }};
      }
//# 879 "cyclic_reduction.c"
   for (l = 0; ; l++)
   {

      { if (l > 0) ( (cindex[0]) = 0, (cindex[1]) = 0, (cindex[2]) = 0 ); else ( (cindex[0]) = (base_index[0]), (cindex[1]) = (base_index[1]), (cindex[2]) = (base_index[2]) ); (cindex[cdir]) += 0;};
      { if (l > 0) ( (stride[0]) = 1, (stride[1]) = 1, (stride[2]) = 1 ); else ( (stride[0]) = (base_stride[0]), (stride[1]) = (base_stride[1]), (stride[2]) = (base_stride[2]) ); (stride[cdir]) *= 2;};


      compute_box_a = fine_points_l[l];
      for (fi = 0; fi < ((compute_box_a) -> size); fi++)
         {
            compute_box = &((compute_box_a) -> boxes[(fi)]);

            A_dbox =
               &((((A_l[l]) -> data_space)) -> boxes[(fi)]);
            x_dbox =
               &((((x_l[l]) -> data_space)) -> boxes[(fi)]);

            ( (index[0]) = 0, (index[1]) = 0, (index[2]) = 0 );
            Ap = hypre_StructMatrixExtractPointerByIndex(A_l[l], fi, index);
            xp = (((x_l[l]) -> data) + ((x_l[l]) -> data_indices)[fi]);

            ( (start[0]) = (((compute_box) -> imin)[0]), (start[1]) = (((compute_box) -> imin)[1]), (start[2]) = (((compute_box) -> imin)[2]) );
            hypre_BoxGetStrideSize(compute_box, stride, loop_size);

            { int hypre__i1start = (((start[0]) - ((((A_dbox) -> imin)[0]))) + (((start[1]) - ((((A_dbox) -> imin)[1]))) + (((start[2]) - ((((A_dbox) -> imin)[2]))) * (((0)<((((((A_dbox) -> imax)[1])) - ((((A_dbox) -> imin)[1])) + 1))) ? ((((((A_dbox) -> imax)[1])) - ((((A_dbox) -> imin)[1])) + 1)) : (0)))) * (((0)<((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1))) ? ((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1)) : (0))); int hypre__i2start = (((start[0]) - ((((x_dbox) -> imin)[0]))) + (((start[1]) - ((((x_dbox) -> imin)[1]))) + (((start[2]) - ((((x_dbox) -> imin)[2]))) * (((0)<((((((x_dbox) -> imax)[1])) - ((((x_dbox) -> imin)[1])) + 1))) ? ((((((x_dbox) -> imax)[1])) - ((((x_dbox) -> imin)[1])) + 1)) : (0)))) * (((0)<((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1))) ? ((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1)) : (0))); int hypre__sx1 = ((stride[0]));int hypre__sy1 = ((stride[1])*(((0)<((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1))) ? ((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1)) : (0)));int hypre__sz1 = ((stride[2])* (((0)<((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1))) ? ((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1)) : (0))*(((0)<((((((A_dbox) -> imax)[1])) - ((((A_dbox) -> imin)[1])) + 1))) ? ((((((A_dbox) -> imax)[1])) - ((((A_dbox) -> imin)[1])) + 1)) : (0))); int hypre__sx2 = ((stride[0]));int hypre__sy2 = ((stride[1])*(((0)<((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1))) ? ((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1)) : (0)));int hypre__sz2 = ((stride[2])* (((0)<((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1))) ? ((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1)) : (0))*(((0)<((((((x_dbox) -> imax)[1])) - ((((x_dbox) -> imin)[1])) + 1))) ? ((((((x_dbox) -> imax)[1])) - ((((x_dbox) -> imin)[1])) + 1)) : (0))); int hypre__nx = (loop_size[0]);int hypre__ny = (loop_size[1]);int hypre__nz = (loop_size[2]);int hypre__mx = hypre__nx;int hypre__my = hypre__ny;int hypre__mz = hypre__nz;int hypre__dir, hypre__max;int hypre__div, hypre__mod;int hypre__block, hypre__num_blocks;hypre__dir = 0;hypre__max = hypre__nx;if (hypre__ny > hypre__max){ hypre__dir = 1; hypre__max = hypre__ny;}if (hypre__nz > hypre__max){ hypre__dir = 2; hypre__max = hypre__nz;}hypre__num_blocks = 1;if (hypre__max < hypre__num_blocks){ hypre__num_blocks = hypre__max;}if (hypre__num_blocks > 0){ hypre__div = hypre__max / hypre__num_blocks; hypre__mod = hypre__max % hypre__num_blocks;};;




//# 1 "../struct_mv/hypre_box_smp_forloop.h" 1
//# 17 "../struct_mv/hypre_box_smp_forloop.h"

//# 1 "../struct_mv/../utilities/hypre_smp_forloop.h" 1
//# 18 "../struct_mv/hypre_box_smp_forloop.h" 2
//# 908 "cyclic_reduction.c" 2
            for (hypre__block = 0; hypre__block < hypre__num_blocks; hypre__block++) { loopi = 0;loopj = 0;loopk = 0;hypre__nx = hypre__mx;hypre__ny = hypre__my;hypre__nz = hypre__mz;if (hypre__num_blocks > 1){ if (hypre__dir == 0) { loopi = hypre__block * hypre__div + (((hypre__mod)<(hypre__block)) ? (hypre__mod) : (hypre__block)); hypre__nx = hypre__div + ((hypre__mod > hypre__block) ? 1 : 0); } else if (hypre__dir == 1) { loopj = hypre__block * hypre__div + (((hypre__mod)<(hypre__block)) ? (hypre__mod) : (hypre__block)); hypre__ny = hypre__div + ((hypre__mod > hypre__block) ? 1 : 0); } else if (hypre__dir == 2) { loopk = hypre__block * hypre__div + (((hypre__mod)<(hypre__block)) ? (hypre__mod) : (hypre__block)); hypre__nz = hypre__div + ((hypre__mod > hypre__block) ? 1 : 0); }}; Ai = hypre__i1start + loopi*hypre__sx1 + loopj*hypre__sy1 + loopk*hypre__sz1; xi = hypre__i2start + loopi*hypre__sx2 + loopj*hypre__sy2 + loopk*hypre__sz2; for (loopk = 0; loopk < hypre__nz; loopk++) { for (loopj = 0; loopj < hypre__ny; loopj++) { for (loopi = 0; loopi < hypre__nx; loopi++) {
               {
                  xp[xi] /= Ap[Ai];
               }
            Ai += hypre__sx1; xi += hypre__sx2; } Ai += hypre__sy1 - hypre__nx*hypre__sx1; xi += hypre__sy2 - hypre__nx*hypre__sx2; } Ai += hypre__sz1 - hypre__ny*hypre__sy1; xi += hypre__sz2 - hypre__ny*hypre__sy2; } }};
         }

      if (l == (num_levels - 1))
         break;


      fgrid = ((x_l[l]) -> grid);
      fgrid_ids = ((fgrid) -> ids);
      cgrid = ((x_l[l+1]) -> grid);
      cgrid_boxes = ((cgrid) -> boxes);
      cgrid_ids = ((cgrid) -> ids);
      for (compute_i = 0; compute_i < 2; compute_i++)
      {
         switch(compute_i)
         {
            case 0:
            {
               xp = ((x_l[l]) -> data);
               hypre_InitializeIndtComputations(down_compute_pkg_l[l], xp,
                                                &comm_handle);
               compute_box_aa =
                  (down_compute_pkg_l[l] -> indt_boxes);
            }
            break;

            case 1:
            {
               hypre_FinalizeIndtComputations(comm_handle);
               compute_box_aa =
                  (down_compute_pkg_l[l] -> dept_boxes);
            }
            break;
         }

         fi = 0;
         for (ci = 0; ci < ((cgrid_boxes) -> size); ci++)
            {
               while (fgrid_ids[fi] != cgrid_ids[ci])
               {
                  fi++;
               }

               compute_box_a =
                  ((compute_box_aa) -> box_arrays[(fi)]);

               A_dbox =
                  &((((A_l[l]) -> data_space)) -> boxes[(fi)]);
               x_dbox =
                  &((((x_l[l]) -> data_space)) -> boxes[(fi)]);
               xc_dbox =
                  &((((x_l[l+1]) -> data_space)) -> boxes[(ci)]);

               xp = (((x_l[l]) -> data) + ((x_l[l]) -> data_indices)[fi]);
               xcp = (((x_l[l+1]) -> data) + ((x_l[l+1]) -> data_indices)[ci]);

               ( (index[0]) = -1, (index[1]) = 0, (index[2]) = 0 );
               Awp =
                  hypre_StructMatrixExtractPointerByIndex(A_l[l], fi, index);
               xwp = (((x_l[l]) -> data) + ((x_l[l]) -> data_indices)[fi]) +
                  ((index[0]) + ((index[1]) + ((index[2]) * (((0)<((((((x_dbox) -> imax)[1])) - ((((x_dbox) -> imin)[1])) + 1))) ? ((((((x_dbox) -> imax)[1])) - ((((x_dbox) -> imin)[1])) + 1)) : (0)))) * (((0)<((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1))) ? ((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1)) : (0)));

               ( (index[0]) = 1, (index[1]) = 0, (index[2]) = 0 );
               Aep =
                  hypre_StructMatrixExtractPointerByIndex(A_l[l], fi, index);
               xep = (((x_l[l]) -> data) + ((x_l[l]) -> data_indices)[fi]) +
                  ((index[0]) + ((index[1]) + ((index[2]) * (((0)<((((((x_dbox) -> imax)[1])) - ((((x_dbox) -> imin)[1])) + 1))) ? ((((((x_dbox) -> imax)[1])) - ((((x_dbox) -> imin)[1])) + 1)) : (0)))) * (((0)<((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1))) ? ((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1)) : (0)));

               for (j = 0; j < ((compute_box_a) -> size); j++)
                  {
                     compute_box = &((compute_box_a) -> boxes[(j)]);

                     ( (start[0]) = (((compute_box) -> imin)[0]), (start[1]) = (((compute_box) -> imin)[1]), (start[2]) = (((compute_box) -> imin)[2]) );
                     hypre_StructMapFineToCoarse(start, cindex, stride,
                                                 startc);

                     hypre_BoxGetStrideSize(compute_box, stride, loop_size);

                     { int hypre__i1start = (((start[0]) - ((((A_dbox) -> imin)[0]))) + (((start[1]) - ((((A_dbox) -> imin)[1]))) + (((start[2]) - ((((A_dbox) -> imin)[2]))) * (((0)<((((((A_dbox) -> imax)[1])) - ((((A_dbox) -> imin)[1])) + 1))) ? ((((((A_dbox) -> imax)[1])) - ((((A_dbox) -> imin)[1])) + 1)) : (0)))) * (((0)<((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1))) ? ((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1)) : (0))); int hypre__i2start = (((start[0]) - ((((x_dbox) -> imin)[0]))) + (((start[1]) - ((((x_dbox) -> imin)[1]))) + (((start[2]) - ((((x_dbox) -> imin)[2]))) * (((0)<((((((x_dbox) -> imax)[1])) - ((((x_dbox) -> imin)[1])) + 1))) ? ((((((x_dbox) -> imax)[1])) - ((((x_dbox) -> imin)[1])) + 1)) : (0)))) * (((0)<((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1))) ? ((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1)) : (0))); int hypre__i3start = (((startc[0]) - ((((xc_dbox) -> imin)[0]))) + (((startc[1]) - ((((xc_dbox) -> imin)[1]))) + (((startc[2]) - ((((xc_dbox) -> imin)[2]))) * (((0)<((((((xc_dbox) -> imax)[1])) - ((((xc_dbox) -> imin)[1])) + 1))) ? ((((((xc_dbox) -> imax)[1])) - ((((xc_dbox) -> imin)[1])) + 1)) : (0)))) * (((0)<((((((xc_dbox) -> imax)[0])) - ((((xc_dbox) -> imin)[0])) + 1))) ? ((((((xc_dbox) -> imax)[0])) - ((((xc_dbox) -> imin)[0])) + 1)) : (0))); int hypre__sx1 = ((stride[0]));int hypre__sy1 = ((stride[1])*(((0)<((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1))) ? ((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1)) : (0)));int hypre__sz1 = ((stride[2])* (((0)<((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1))) ? ((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1)) : (0))*(((0)<((((((A_dbox) -> imax)[1])) - ((((A_dbox) -> imin)[1])) + 1))) ? ((((((A_dbox) -> imax)[1])) - ((((A_dbox) -> imin)[1])) + 1)) : (0))); int hypre__sx2 = ((stride[0]));int hypre__sy2 = ((stride[1])*(((0)<((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1))) ? ((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1)) : (0)));int hypre__sz2 = ((stride[2])* (((0)<((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1))) ? ((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1)) : (0))*(((0)<((((((x_dbox) -> imax)[1])) - ((((x_dbox) -> imin)[1])) + 1))) ? ((((((x_dbox) -> imax)[1])) - ((((x_dbox) -> imin)[1])) + 1)) : (0))); int hypre__sx3 = ((stridec[0]));int hypre__sy3 = ((stridec[1])*(((0)<((((((xc_dbox) -> imax)[0])) - ((((xc_dbox) -> imin)[0])) + 1))) ? ((((((xc_dbox) -> imax)[0])) - ((((xc_dbox) -> imin)[0])) + 1)) : (0)));int hypre__sz3 = ((stridec[2])* (((0)<((((((xc_dbox) -> imax)[0])) - ((((xc_dbox) -> imin)[0])) + 1))) ? ((((((xc_dbox) -> imax)[0])) - ((((xc_dbox) -> imin)[0])) + 1)) : (0))*(((0)<((((((xc_dbox) -> imax)[1])) - ((((xc_dbox) -> imin)[1])) + 1))) ? ((((((xc_dbox) -> imax)[1])) - ((((xc_dbox) -> imin)[1])) + 1)) : (0))); int hypre__nx = (loop_size[0]);int hypre__ny = (loop_size[1]);int hypre__nz = (loop_size[2]);int hypre__mx = hypre__nx;int hypre__my = hypre__ny;int hypre__mz = hypre__nz;int hypre__dir, hypre__max;int hypre__div, hypre__mod;int hypre__block, hypre__num_blocks;hypre__dir = 0;hypre__max = hypre__nx;if (hypre__ny > hypre__max){ hypre__dir = 1; hypre__max = hypre__ny;}if (hypre__nz > hypre__max){ hypre__dir = 2; hypre__max = hypre__nz;}hypre__num_blocks = 1;if (hypre__max < hypre__num_blocks){ hypre__num_blocks = hypre__max;}if (hypre__num_blocks > 0){ hypre__div = hypre__max / hypre__num_blocks; hypre__mod = hypre__max % hypre__num_blocks;};;

/*



//# 1 "../struct_mv/hypre_box_smp_forloop.h" 1
//# 17 "../struct_mv/hypre_box_smp_forloop.h"

//# 1 "../struct_mv/../utilities/hypre_smp_forloop.h" 1
//# 18 "../struct_mv/hypre_box_smp_forloop.h" 2
//# 996 "cyclic_reduction.c" 2
*/
                     for (hypre__block = 0; hypre__block < hypre__num_blocks; hypre__block++) {
                         loopi = 0;
                         loopj = 0;
                         loopk = 0;
                         hypre__nx = hypre__mx;
                         hypre__ny = hypre__my;
                         hypre__nz = hypre__mz;
                         if (hypre__num_blocks > 1){
                             if (hypre__dir == 0) {
                                 loopi = hypre__block * hypre__div + (((hypre__mod)<(hypre__block)) ? (hypre__mod) : (hypre__block));
                                 hypre__nx = hypre__div + ((hypre__mod > hypre__block) ? 1 : 0);
                             } else if (hypre__dir == 1) {
                                 loopj = hypre__block * hypre__div + (((hypre__mod)<(hypre__block)) ? (hypre__mod) : (hypre__block));
                                 hypre__ny = hypre__div + ((hypre__mod > hypre__block) ? 1 : 0);
                             } else if (hypre__dir == 2) {
                                 loopk = hypre__block * hypre__div + (((hypre__mod)<(hypre__block)) ? (hypre__mod) : (hypre__block));
                                 hypre__nz = hypre__div + ((hypre__mod > hypre__block) ? 1 : 0);
                             }
                         };
                         Ai = hypre__i1start + loopi*hypre__sx1 + loopj*hypre__sy1 + loopk*hypre__sz1;
                         xi = hypre__i2start + loopi*hypre__sx2 + loopj*hypre__sy2 + loopk*hypre__sz2;
                         xci = hypre__i3start + loopi*hypre__sx3 + loopj*hypre__sy3 + loopk*hypre__sz3;
                         for (loopk = 0; loopk < hypre__nz; loopk++) {
                             for (loopj = 0; loopj < hypre__ny; loopj++) {
                                 for (loopi = 0; loopi < hypre__nx; loopi++) {
                                     {
                                         xcp[xci] = xp[xi] -
                                             Awp[Ai]*xwp[xi] -
                                             Aep[Ai]*xep[xi];
                                     }
                                     Ai += hypre__sx1; xi += hypre__sx2; xci += hypre__sx3;
                                 }
                                 Ai += hypre__sy1 - hypre__nx*hypre__sx1;
                                 xi += hypre__sy2 - hypre__nx*hypre__sx2;
                                 xci += hypre__sy3 - hypre__nx*hypre__sx3;
                             }
                             Ai += hypre__sz1 - hypre__ny*hypre__sy1;
                             xi += hypre__sz2 - hypre__ny*hypre__sy2;
                             xci += hypre__sz3 - hypre__ny*hypre__sy3;
                         }
                     }
                     };
                  }
            }
      }
   }
//# 1019 "cyclic_reduction.c"
   for (l = (num_levels - 2); l >= 0; l--)
   {

      { if (l > 0) ( (cindex[0]) = 0, (cindex[1]) = 0, (cindex[2]) = 0 ); else ( (cindex[0]) = (base_index[0]), (cindex[1]) = (base_index[1]), (cindex[2]) = (base_index[2]) ); (cindex[cdir]) += 0;};
      { if (l > 0) ( (stride[0]) = 1, (stride[1]) = 1, (stride[2]) = 1 ); else ( (stride[0]) = (base_stride[0]), (stride[1]) = (base_stride[1]), (stride[2]) = (base_stride[2]) ); (stride[cdir]) *= 2;};


      fgrid = ((x_l[l]) -> grid);
      fgrid_ids = ((fgrid) -> ids);
      cgrid = ((x_l[l+1]) -> grid);
      cgrid_boxes = ((cgrid) -> boxes);
      cgrid_ids = ((cgrid) -> ids);
      fi = 0;
      for (ci = 0; ci < ((cgrid_boxes) -> size); ci++)
         {
            while (fgrid_ids[fi] != cgrid_ids[ci])
            {
               fi++;
            }

            compute_box = &((cgrid_boxes) -> boxes[(ci)]);

            ( (startc[0]) = (((compute_box) -> imin)[0]), (startc[1]) = (((compute_box) -> imin)[1]), (startc[2]) = (((compute_box) -> imin)[2]) );
            hypre_StructMapCoarseToFine(startc, cindex, stride, start);

            x_dbox =
               &((((x_l[l]) -> data_space)) -> boxes[(fi)]);
            xc_dbox =
               &((((x_l[l+1]) -> data_space)) -> boxes[(ci)]);

            xp = (((x_l[l]) -> data) + ((x_l[l]) -> data_indices)[fi]);
            xcp = (((x_l[l+1]) -> data) + ((x_l[l+1]) -> data_indices)[ci]);

            hypre_BoxGetSize(compute_box, loop_size);

            { int hypre__i1start = (((start[0]) - ((((x_dbox) -> imin)[0]))) + (((start[1]) - ((((x_dbox) -> imin)[1]))) + (((start[2]) - ((((x_dbox) -> imin)[2]))) * (((0)<((((((x_dbox) -> imax)[1])) - ((((x_dbox) -> imin)[1])) + 1))) ? ((((((x_dbox) -> imax)[1])) - ((((x_dbox) -> imin)[1])) + 1)) : (0)))) * (((0)<((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1))) ? ((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1)) : (0))); int hypre__i2start = (((startc[0]) - ((((xc_dbox) -> imin)[0]))) + (((startc[1]) - ((((xc_dbox) -> imin)[1]))) + (((startc[2]) - ((((xc_dbox) -> imin)[2]))) * (((0)<((((((xc_dbox) -> imax)[1])) - ((((xc_dbox) -> imin)[1])) + 1))) ? ((((((xc_dbox) -> imax)[1])) - ((((xc_dbox) -> imin)[1])) + 1)) : (0)))) * (((0)<((((((xc_dbox) -> imax)[0])) - ((((xc_dbox) -> imin)[0])) + 1))) ? ((((((xc_dbox) -> imax)[0])) - ((((xc_dbox) -> imin)[0])) + 1)) : (0))); int hypre__sx1 = ((stride[0]));int hypre__sy1 = ((stride[1])*(((0)<((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1))) ? ((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1)) : (0)));int hypre__sz1 = ((stride[2])* (((0)<((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1))) ? ((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1)) : (0))*(((0)<((((((x_dbox) -> imax)[1])) - ((((x_dbox) -> imin)[1])) + 1))) ? ((((((x_dbox) -> imax)[1])) - ((((x_dbox) -> imin)[1])) + 1)) : (0))); int hypre__sx2 = ((stridec[0]));int hypre__sy2 = ((stridec[1])*(((0)<((((((xc_dbox) -> imax)[0])) - ((((xc_dbox) -> imin)[0])) + 1))) ? ((((((xc_dbox) -> imax)[0])) - ((((xc_dbox) -> imin)[0])) + 1)) : (0)));int hypre__sz2 = ((stridec[2])* (((0)<((((((xc_dbox) -> imax)[0])) - ((((xc_dbox) -> imin)[0])) + 1))) ? ((((((xc_dbox) -> imax)[0])) - ((((xc_dbox) -> imin)[0])) + 1)) : (0))*(((0)<((((((xc_dbox) -> imax)[1])) - ((((xc_dbox) -> imin)[1])) + 1))) ? ((((((xc_dbox) -> imax)[1])) - ((((xc_dbox) -> imin)[1])) + 1)) : (0))); int hypre__nx = (loop_size[0]);int hypre__ny = (loop_size[1]);int hypre__nz = (loop_size[2]);int hypre__mx = hypre__nx;int hypre__my = hypre__ny;int hypre__mz = hypre__nz;int hypre__dir, hypre__max;int hypre__div, hypre__mod;int hypre__block, hypre__num_blocks;hypre__dir = 0;hypre__max = hypre__nx;if (hypre__ny > hypre__max){ hypre__dir = 1; hypre__max = hypre__ny;}if (hypre__nz > hypre__max){ hypre__dir = 2; hypre__max = hypre__nz;}hypre__num_blocks = 1;if (hypre__max < hypre__num_blocks){ hypre__num_blocks = hypre__max;}if (hypre__num_blocks > 0){ hypre__div = hypre__max / hypre__num_blocks; hypre__mod = hypre__max % hypre__num_blocks;};;




//# 1 "../struct_mv/hypre_box_smp_forloop.h" 1
//# 17 "../struct_mv/hypre_box_smp_forloop.h"

//# 1 "../struct_mv/../utilities/hypre_smp_forloop.h" 1
//# 18 "../struct_mv/hypre_box_smp_forloop.h" 2
//# 1059 "cyclic_reduction.c" 2
            for (hypre__block = 0; hypre__block < hypre__num_blocks; hypre__block++) { loopi = 0;loopj = 0;loopk = 0;hypre__nx = hypre__mx;hypre__ny = hypre__my;hypre__nz = hypre__mz;if (hypre__num_blocks > 1){ if (hypre__dir == 0) { loopi = hypre__block * hypre__div + (((hypre__mod)<(hypre__block)) ? (hypre__mod) : (hypre__block)); hypre__nx = hypre__div + ((hypre__mod > hypre__block) ? 1 : 0); } else if (hypre__dir == 1) { loopj = hypre__block * hypre__div + (((hypre__mod)<(hypre__block)) ? (hypre__mod) : (hypre__block)); hypre__ny = hypre__div + ((hypre__mod > hypre__block) ? 1 : 0); } else if (hypre__dir == 2) { loopk = hypre__block * hypre__div + (((hypre__mod)<(hypre__block)) ? (hypre__mod) : (hypre__block)); hypre__nz = hypre__div + ((hypre__mod > hypre__block) ? 1 : 0); }}; xi = hypre__i1start + loopi*hypre__sx1 + loopj*hypre__sy1 + loopk*hypre__sz1; xci = hypre__i2start + loopi*hypre__sx2 + loopj*hypre__sy2 + loopk*hypre__sz2; for (loopk = 0; loopk < hypre__nz; loopk++) { for (loopj = 0; loopj < hypre__ny; loopj++) { for (loopi = 0; loopi < hypre__nx; loopi++) {
               {
                  xp[xi] = xcp[xci];
               }
            xi += hypre__sx1; xci += hypre__sx2; } xi += hypre__sy1 - hypre__nx*hypre__sx1; xci += hypre__sy2 - hypre__nx*hypre__sx2; } xi += hypre__sz1 - hypre__ny*hypre__sy1; xci += hypre__sz2 - hypre__ny*hypre__sy2; } }};
         }


      for (compute_i = 0; compute_i < 2; compute_i++)
      {
         switch(compute_i)
         {
            case 0:
            {
               xp = ((x_l[l]) -> data);
               hypre_InitializeIndtComputations(up_compute_pkg_l[l], xp,
                                                &comm_handle);
               compute_box_aa =
                  (up_compute_pkg_l[l] -> indt_boxes);
            }
            break;

            case 1:
            {
               hypre_FinalizeIndtComputations(comm_handle);
               compute_box_aa =
                  (up_compute_pkg_l[l] -> dept_boxes);
            }
            break;
         }

         for (fi = 0; fi < ((compute_box_aa) -> size); fi++)
            {
               compute_box_a =
                  ((compute_box_aa) -> box_arrays[(fi)]);

               A_dbox =
                  &((((A_l[l]) -> data_space)) -> boxes[(fi)]);
               x_dbox =
                  &((((x_l[l]) -> data_space)) -> boxes[(fi)]);

               ( (index[0]) = 0, (index[1]) = 0, (index[2]) = 0 );
               Ap = hypre_StructMatrixExtractPointerByIndex(A_l[l], fi, index);
               xp = (((x_l[l]) -> data) + ((x_l[l]) -> data_indices)[fi]);

               ( (index[0]) = -1, (index[1]) = 0, (index[2]) = 0 );
               Awp =
                  hypre_StructMatrixExtractPointerByIndex(A_l[l], fi, index);
               xwp = (((x_l[l]) -> data) + ((x_l[l]) -> data_indices)[fi]) +
                  ((index[0]) + ((index[1]) + ((index[2]) * (((0)<((((((x_dbox) -> imax)[1])) - ((((x_dbox) -> imin)[1])) + 1))) ? ((((((x_dbox) -> imax)[1])) - ((((x_dbox) -> imin)[1])) + 1)) : (0)))) * (((0)<((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1))) ? ((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1)) : (0)));

               ( (index[0]) = 1, (index[1]) = 0, (index[2]) = 0 );
               Aep =
                  hypre_StructMatrixExtractPointerByIndex(A_l[l], fi, index);
               xep = (((x_l[l]) -> data) + ((x_l[l]) -> data_indices)[fi]) +
                  ((index[0]) + ((index[1]) + ((index[2]) * (((0)<((((((x_dbox) -> imax)[1])) - ((((x_dbox) -> imin)[1])) + 1))) ? ((((((x_dbox) -> imax)[1])) - ((((x_dbox) -> imin)[1])) + 1)) : (0)))) * (((0)<((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1))) ? ((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1)) : (0)));

               for (j = 0; j < ((compute_box_a) -> size); j++)
                  {
                     compute_box = &((compute_box_a) -> boxes[(j)]);

                     ( (start[0]) = (((compute_box) -> imin)[0]), (start[1]) = (((compute_box) -> imin)[1]), (start[2]) = (((compute_box) -> imin)[2]) );
                     hypre_BoxGetStrideSize(compute_box, stride, loop_size);

                     { int hypre__i1start = (((start[0]) - ((((A_dbox) -> imin)[0]))) + (((start[1]) - ((((A_dbox) -> imin)[1]))) + (((start[2]) - ((((A_dbox) -> imin)[2]))) * (((0)<((((((A_dbox) -> imax)[1])) - ((((A_dbox) -> imin)[1])) + 1))) ? ((((((A_dbox) -> imax)[1])) - ((((A_dbox) -> imin)[1])) + 1)) : (0)))) * (((0)<((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1))) ? ((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1)) : (0))); int hypre__i2start = (((start[0]) - ((((x_dbox) -> imin)[0]))) + (((start[1]) - ((((x_dbox) -> imin)[1]))) + (((start[2]) - ((((x_dbox) -> imin)[2]))) * (((0)<((((((x_dbox) -> imax)[1])) - ((((x_dbox) -> imin)[1])) + 1))) ? ((((((x_dbox) -> imax)[1])) - ((((x_dbox) -> imin)[1])) + 1)) : (0)))) * (((0)<((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1))) ? ((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1)) : (0))); int hypre__sx1 = ((stride[0]));int hypre__sy1 = ((stride[1])*(((0)<((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1))) ? ((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1)) : (0)));int hypre__sz1 = ((stride[2])* (((0)<((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1))) ? ((((((A_dbox) -> imax)[0])) - ((((A_dbox) -> imin)[0])) + 1)) : (0))*(((0)<((((((A_dbox) -> imax)[1])) - ((((A_dbox) -> imin)[1])) + 1))) ? ((((((A_dbox) -> imax)[1])) - ((((A_dbox) -> imin)[1])) + 1)) : (0))); int hypre__sx2 = ((stride[0]));int hypre__sy2 = ((stride[1])*(((0)<((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1))) ? ((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1)) : (0)));int hypre__sz2 = ((stride[2])* (((0)<((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1))) ? ((((((x_dbox) -> imax)[0])) - ((((x_dbox) -> imin)[0])) + 1)) : (0))*(((0)<((((((x_dbox) -> imax)[1])) - ((((x_dbox) -> imin)[1])) + 1))) ? ((((((x_dbox) -> imax)[1])) - ((((x_dbox) -> imin)[1])) + 1)) : (0))); int hypre__nx = (loop_size[0]);int hypre__ny = (loop_size[1]);int hypre__nz = (loop_size[2]);int hypre__mx = hypre__nx;int hypre__my = hypre__ny;int hypre__mz = hypre__nz;int hypre__dir, hypre__max;int hypre__div, hypre__mod;int hypre__block, hypre__num_blocks;hypre__dir = 0;hypre__max = hypre__nx;if (hypre__ny > hypre__max){ hypre__dir = 1; hypre__max = hypre__ny;}if (hypre__nz > hypre__max){ hypre__dir = 2; hypre__max = hypre__nz;}hypre__num_blocks = 1;if (hypre__max < hypre__num_blocks){ hypre__num_blocks = hypre__max;}if (hypre__num_blocks > 0){ hypre__div = hypre__max / hypre__num_blocks; hypre__mod = hypre__max % hypre__num_blocks;};;




//# 1 "../struct_mv/hypre_box_smp_forloop.h" 1
//# 17 "../struct_mv/hypre_box_smp_forloop.h"

//# 1 "../struct_mv/../utilities/hypre_smp_forloop.h" 1
//# 18 "../struct_mv/hypre_box_smp_forloop.h" 2
//#  1128 "cyclic_reduction.c" 2
                     for (hypre__block = 0; hypre__block < hypre__num_blocks; hypre__block++) {
                         loopi = 0;
                         loopj = 0;
                         loopk = 0;
                         hypre__nx = hypre__mx;
                         hypre__ny = hypre__my;
                         hypre__nz = hypre__mz;
                         if (hypre__num_blocks > 1){
                             if (hypre__dir == 0) {
                                 loopi = hypre__block * hypre__div + (((hypre__mod)<(hypre__block)) ? (hypre__mod) : (hypre__block));
                                 hypre__nx = hypre__div + ((hypre__mod > hypre__block) ? 1 : 0);
                             } else if (hypre__dir == 1) {
                                 loopj = hypre__block * hypre__div + (((hypre__mod)<(hypre__block)) ? (hypre__mod) : (hypre__block));
                                 hypre__ny = hypre__div + ((hypre__mod > hypre__block) ? 1 : 0);
                             } else if (hypre__dir == 2) {
                                 loopk = hypre__block * hypre__div + (((hypre__mod)<(hypre__block)) ? (hypre__mod) : (hypre__block));
                                 hypre__nz = hypre__div + ((hypre__mod > hypre__block) ? 1 : 0);
                             }
                         }
                         Ai = hypre__i1start + loopi*hypre__sx1 + loopj*hypre__sy1 + loopk*hypre__sz1;
                         xi = hypre__i2start + loopi*hypre__sx2 + loopj*hypre__sy2 + loopk*hypre__sz2;
                         for (loopk = 0; loopk < hypre__nz; loopk++) {
                             for (loopj = 0; loopj < hypre__ny; loopj++) {
                                 for (loopi = 0; loopi < hypre__nx; loopi++) {
                                     {
                                         xp[xi] -= (Awp[Ai]*xwp[xi] +
                                                    Aep[Ai]*xep[xi] ) / Ap[Ai];
                                     }
                                     Ai += hypre__sx1; xi += hypre__sx2;
                                 }
                                 Ai += hypre__sy1 - hypre__nx*hypre__sx1;
                                 xi += hypre__sy2 - hypre__nx*hypre__sx2;
                             }
                             Ai += hypre__sz1 - hypre__ny*hypre__sy1;
                             xi += hypre__sz2 - hypre__ny*hypre__sy2;
                         }
                     }
                     };
                  }
            }
      }
   }

   return ierr;
}





int
hypre_CyclicReductionSetBase( void *cyc_red_vdata,
                              hypre_Index base_index,
                              hypre_Index base_stride )
{
   hypre_CyclicReductionData *cyc_red_data = cyc_red_vdata;
   int d;
   int ierr = 0;

   for (d = 0; d < 3; d++)
   {
      ((cyc_red_data -> base_index)[d]) =
         (base_index[d]);
      ((cyc_red_data -> base_stride)[d]) =
         (base_stride[d]);
   }

   return ierr;
}





int
hypre_CyclicReductionDestroy( void *cyc_red_vdata )
{
   hypre_CyclicReductionData *cyc_red_data = cyc_red_vdata;

   int l;
   int ierr = 0;

   if (cyc_red_data)
   {
      hypre_BoxArrayDestroy(cyc_red_data -> base_points);
      hypre_StructGridDestroy(cyc_red_data -> grid_l[0]);
      hypre_StructMatrixDestroy(cyc_red_data -> A_l[0]);
      hypre_StructVectorDestroy(cyc_red_data -> x_l[0]);
      for (l = 0; l < ((cyc_red_data -> num_levels) - 1); l++)
      {
         hypre_StructGridDestroy(cyc_red_data -> grid_l[l+1]);
         hypre_BoxArrayDestroy(cyc_red_data -> fine_points_l[l]);
         hypre_StructMatrixDestroy(cyc_red_data -> A_l[l+1]);
         hypre_StructVectorDestroy(cyc_red_data -> x_l[l+1]);
         hypre_ComputePkgDestroy(cyc_red_data -> down_compute_pkg_l[l]);
         hypre_ComputePkgDestroy(cyc_red_data -> up_compute_pkg_l[l]);
      }
      hypre_BoxArrayDestroy(cyc_red_data -> fine_points_l[l]);
      ( hypre_Free((char *)cyc_red_data -> data), cyc_red_data -> data = ((void *)0) );
      ( hypre_Free((char *)cyc_red_data -> grid_l), cyc_red_data -> grid_l = ((void *)0) );
      ( hypre_Free((char *)cyc_red_data -> fine_points_l), cyc_red_data -> fine_points_l = ((void *)0) );
      ( hypre_Free((char *)cyc_red_data -> A_l), cyc_red_data -> A_l = ((void *)0) );
      ( hypre_Free((char *)cyc_red_data -> x_l), cyc_red_data -> x_l = ((void *)0) );
      ( hypre_Free((char *)cyc_red_data -> down_compute_pkg_l), cyc_red_data -> down_compute_pkg_l = ((void *)0) );
      ( hypre_Free((char *)cyc_red_data -> up_compute_pkg_l), cyc_red_data -> up_compute_pkg_l = ((void *)0) );

      ;
      ( hypre_Free((char *)cyc_red_data), cyc_red_data = ((void *)0) );
   }

   return ierr;
}
