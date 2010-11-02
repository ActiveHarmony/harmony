/*
 * This file was created automatically from SUIF
 *   on Thu Oct 21 08:39:20 2010.
 *
 * Created by:
 * s2c 1.3.0.1 (plus local changes) compiled Tue Oct 12 14:23:42 EDT 2010 by tiwari on spoon
 *     Based on SUIF distribution 1.3.0.1
 *     Linked with:
 *   libsuif1 1.3.0.1 (plus local changes) compiled Tue Oct 12 14:23:10 EDT 2010 by tiwari on spoon
 *   libuseful 1.3.0.1 (plus local changes) compiled Tue Oct 12 14:23:17 EDT 2010 by tiwari on spoon
 */


#define __suif_min(x,y) ((x)<(y)?(x):(y))
#define __suif_max(x,y) ((x)>(y)?(x):(y))

extern void rmatmult_3_at(void **, double *, double *);

extern void rmatmult_3_at(void **rmatmult3_args, double *x, double *b)
  {
    int i;
    int ii;
    int jj;
    int kk;
    int imin;
    int imax;
    int jmin;
    int jmax;
    int kmin;
    int kmax;
    int jp;
    int kp;
    const double *dbl;
    const double *dbc;
    const double *dbr;
    const double *dcl;
    const double *dcc;
    const double *dcr;
    const double *dfl;
    const double *dfc;
    const double *dfr;
    const double *cbl;
    const double *cbc;
    const double *cbr;
    const double *ccl;
    const double *ccc;
    const double *ccr;
    const double *cfl;
    const double *cfc;
    const double *cfr;
    const double *ubl;
    const double *ubc;
    const double *ubr;
    const double *ucl;
    const double *ucc;
    const double *ucr;
    const double *ufl;
    const double *ufc;
    const double *ufr;
    double *xdbl;
    double *xdbc;
    double *xdbr;
    double *xdcl;
    double *xdcc;
    double *xdcr;
    double *xdfl;
    double *xdfc;
    double *xdfr;
    double *xcbl;
    double *xcbc;
    double *xcbr;
    double *xccl;
    double *xccc;
    double *xccr;
    double *xcfl;
    double *xcfc;
    double *xcfr;
    double *xubl;
    double *xubc;
    double *xubr;
    double *xucl;
    double *xucc;
    double *xucr;
    double *xufl;
    double *xufc;
    double *xufr;
    int over1;
    int _t9;
    int _t10;
    int _t11;
    int _t12;
    int _t13;
    int _t14;
    int _t15;
    int _t16;
    int _t17;
    int _t18;
    int _t19;
    int _t20;
    int _t21;
    int _t22;
    int _t23;
    int _t24;
    int _t25;
    int _t26;
    int _t27;
    int _t28;
    int _t29;
    int _t32;
    int over2;
    int _t36;
    int _t37;
    int _t38;
    int _t39;
    int _t40;
    int _t41;
    int _t42;
    int _t43;
    int _t44;
    int _t45;
    int _t46;
    int _t47;
    int _t48;
    int _t49;
    int _t50;
    int _t51;
    int _t52;
    int _t53;
    int _t54;
    int _t55;
    int _t56;
    int _t57;
    int _t58;
    int _t59;
    int t2;
    int t4;
    int t6;
    int t8;
    int t10;
    int t12;

    imin = *(int *)*rmatmult3_args;
    imax = *(int *)rmatmult3_args[1];
    jmin = *(int *)rmatmult3_args[2];
    jmax = *(int *)rmatmult3_args[3];
    kmin = *(int *)rmatmult3_args[4];
    kmax = *(int *)rmatmult3_args[5];
    jp = *(int *)rmatmult3_args[6];
    kp = *(int *)rmatmult3_args[7];
    dbl = *(const double **)rmatmult3_args[8];
    dbc = *(const double **)rmatmult3_args[9];
    dbr = *(const double **)rmatmult3_args[10];
    dcl = *(const double **)rmatmult3_args[11];
    dcc = *(const double **)rmatmult3_args[12];
    dcr = *(const double **)rmatmult3_args[13];
    dfl = *(const double **)rmatmult3_args[14];
    dfc = *(const double **)rmatmult3_args[15];
    dfr = *(const double **)rmatmult3_args[16];
    cbl = *(const double **)rmatmult3_args[17];
    cbc = *(const double **)rmatmult3_args[18];
    cbr = *(const double **)rmatmult3_args[19];
    ccl = *(const double **)rmatmult3_args[20];
    ccc = *(const double **)rmatmult3_args[21];
    ccr = *(const double **)rmatmult3_args[22];
    cfl = *(const double **)rmatmult3_args[23];
    cfc = *(const double **)rmatmult3_args[24];
    cfr = *(const double **)rmatmult3_args[25];
    ubl = *(const double **)rmatmult3_args[26];
    ubc = *(const double **)rmatmult3_args[27];
    ubr = *(const double **)rmatmult3_args[28];
    ucl = *(const double **)rmatmult3_args[29];
    ucc = *(const double **)rmatmult3_args[30];
    ucr = *(const double **)rmatmult3_args[31];
    ufl = *(const double **)rmatmult3_args[32];
    ufc = *(const double **)rmatmult3_args[33];
    ufr = *(const double **)rmatmult3_args[34];
    xdbl = (double *)((char *)x - 8 * kp - 8 * jp) - 1;
    xdbc = (double *)((char *)x - 8 * kp - 8 * jp);
    xdbr = (double *)((char *)x - 8 * kp - 8 * jp) + 1;
    xdcl = (double *)((char *)x - 8 * kp) - 1;
    xdcc = (double *)((char *)x - 8 * kp);
    xdcr = (double *)((char *)x - 8 * kp) + 1;
    xdfl = (double *)((char *)x - 8 * kp + 8 * jp) - 1;
    xdfc = (double *)((char *)x - 8 * kp + 8 * jp);
    xdfr = (double *)((char *)x - 8 * kp + 8 * jp) + 1;
    xcbl = (double *)((char *)x - 8 * jp) - 1;
    xcbc = (double *)((char *)x - 8 * jp);
    xcbr = (double *)((char *)x - 8 * jp) + 1;
    xccl = x - 1;
    xccc = x;
    xccr = x + 1;
    xcfl = (double *)((char *)x + 8 * jp) - 1;
    xcfc = (double *)((char *)x + 8 * jp);
    xcfr = (double *)((char *)x + 8 * jp) + 1;
    xubl = (double *)((char *)x + 8 * kp - 8 * jp) - 1;
    xubc = (double *)((char *)x + 8 * kp - 8 * jp);
    xubr = (double *)((char *)x + 8 * kp - 8 * jp) + 1;
    xucl = (double *)((char *)x + 8 * kp) - 1;
    xucc = (double *)((char *)x + 8 * kp);
    xucr = (double *)((char *)x + 8 * kp) + 1;
    xufl = (double *)((char *)x + 8 * kp + 8 * jp) - 1;
    xufc = (double *)((char *)x + 8 * kp + 8 * jp);
    xufr = (double *)((char *)x + 8 * kp + 8 * jp) + 1;
    over1 = 0;
    over2 = 0;
    for (t2 = 2; t2 <= kmax - 1; t2 += 25)
      {
        for (t4 = 2; t4 <= jmax - 1; t4 += 24)
          {
            for (t6 = 2; t6 <= imax - 1; t6 += 23)
              {
                for (t8 = t2; t8 <= __suif_min(t2 + 24, kmax - 1); t8++)
                  {
                    over1 = (1 + jmax) % 3;
                    for (t10 = t4; t10 <= __suif_min(t4 + 21, -over1 + jmax - 1); t10 += 3)
                      {
                        over2 = (t6 + imax) % 2;
                        for (t12 = t6; t12 <= __suif_min(-over2 + imax - 1, t6 + 20); t12 += 2)
                          {
                            i = t12 + t10 * jp + t8 * kp;
                            b[i] = dbl[i] * xdbl[i] + dbc[i] * xdbc[i] + dbr[i] * xdbr[i] + dcl[i] * xdcl[i] + dcc[i] * xdcc[i] + dcr[i] * xdcr[i] + dfl[i] * xdfl[i] + dfc[i] * xdfc[i] + dfr[i] * xdfr[i] + cbl[i] * xcbl[i] + cbc[i] * xcbc[i] + cbr[i] * xcbr[i] + ccl[i] * xccl[i] + ccc[i] * xccc[i] + ccr[i] * xccr[i] + cfl[i] * xcfl[i] + cfc[i] * xcfc[i] + cfr[i] * xcfr[i] + ubl[i] * xubl[i] + ubc[i] * xubc[i] + ubr[i] * xubr[i] + ucl[i] * xucl[i] + ucc[i] * xucc[i] + ucr[i] * xucr[i] + ufl[i] * xufl[i] + ufc[i] * xufc[i] + ufr[i] * xufr[i];
                            i = t12 + 1 + t10 * jp + t8 * kp;
                            b[i] = dbl[i] * xdbl[i] + dbc[i] * xdbc[i] + dbr[i] * xdbr[i] + dcl[i] * xdcl[i] + dcc[i] * xdcc[i] + dcr[i] * xdcr[i] + dfl[i] * xdfl[i] + dfc[i] * xdfc[i] + dfr[i] * xdfr[i] + cbl[i] * xcbl[i] + cbc[i] * xcbc[i] + cbr[i] * xcbr[i] + ccl[i] * xccl[i] + ccc[i] * xccc[i] + ccr[i] * xccr[i] + cfl[i] * xcfl[i] + cfc[i] * xcfc[i] + cfr[i] * xcfr[i] + ubl[i] * xubl[i] + ubc[i] * xubc[i] + ubr[i] * xubr[i] + ucl[i] * xucl[i] + ucc[i] * xucc[i] + ucr[i] * xucr[i] + ufl[i] * xufl[i] + ufc[i] * xufc[i] + ufr[i] * xufr[i];
                          }
                        if (t12 + 1 <= imax)
                          {
                            i = t6 + 22 + t10 * jp + t8 * kp;
                            b[i] = dbl[i] * xdbl[i] + dbc[i] * xdbc[i] + dbr[i] * xdbr[i] + dcl[i] * xdcl[i] + dcc[i] * xdcc[i] + dcr[i] * xdcr[i] + dfl[i] * xdfl[i] + dfc[i] * xdfc[i] + dfr[i] * xdfr[i] + cbl[i] * xcbl[i] + cbc[i] * xcbc[i] + cbr[i] * xcbr[i] + ccl[i] * xccl[i] + ccc[i] * xccc[i] + ccr[i] * xccr[i] + cfl[i] * xcfl[i] + cfc[i] * xcfc[i] + cfr[i] * xcfr[i] + ubl[i] * xubl[i] + ubc[i] * xubc[i] + ubr[i] * xubr[i] + ucl[i] * xucl[i] + ucc[i] * xucc[i] + ucr[i] * xucr[i] + ufl[i] * xufl[i] + ufc[i] * xufc[i] + ufr[i] * xufr[i];
                          }
                        if (!(!(imax <= t6 + 22) || !(1 <= over2)))
                          {
                            i = imax - 1 + t10 * jp + t8 * kp;
                            b[i] = dbl[i] * xdbl[i] + dbc[i] * xdbc[i] + dbr[i] * xdbr[i] + dcl[i] * xdcl[i] + dcc[i] * xdcc[i] + dcr[i] * xdcr[i] + dfl[i] * xdfl[i] + dfc[i] * xdfc[i] + dfr[i] * xdfr[i] + cbl[i] * xcbl[i] + cbc[i] * xcbc[i] + cbr[i] * xcbr[i] + ccl[i] * xccl[i] + ccc[i] * xccc[i] + ccr[i] * xccr[i] + cfl[i] * xcfl[i] + cfc[i] * xcfc[i] + cfr[i] * xcfr[i] + ubl[i] * xubl[i] + ubc[i] * xubc[i] + ubr[i] * xubr[i] + ucl[i] * xucl[i] + ucc[i] * xucc[i] + ucr[i] * xucr[i] + ufl[i] * xufl[i] + ufc[i] * xufc[i] + ufr[i] * xufr[i];
                          }
                        for (t12 = t6; t12 <= __suif_min(imax - 1, t6 + 22); t12++)
                          {
                            i = t12 + (t10 + 1) * jp + t8 * kp;
                            b[i] = dbl[i] * xdbl[i] + dbc[i] * xdbc[i] + dbr[i] * xdbr[i] + dcl[i] * xdcl[i] + dcc[i] * xdcc[i] + dcr[i] * xdcr[i] + dfl[i] * xdfl[i] + dfc[i] * xdfc[i] + dfr[i] * xdfr[i] + cbl[i] * xcbl[i] + cbc[i] * xcbc[i] + cbr[i] * xcbr[i] + ccl[i] * xccl[i] + ccc[i] * xccc[i] + ccr[i] * xccr[i] + cfl[i] * xcfl[i] + cfc[i] * xcfc[i] + cfr[i] * xcfr[i] + ubl[i] * xubl[i] + ubc[i] * xubc[i] + ubr[i] * xubr[i] + ucl[i] * xucl[i] + ucc[i] * xucc[i] + ucr[i] * xucr[i] + ufl[i] * xufl[i] + ufc[i] * xufc[i] + ufr[i] * xufr[i];
                          }
                        for (t12 = t6; t12 <= __suif_min(imax - 1, t6 + 22); t12++)
                          {
                            i = t12 + (t10 + 2) * jp + t8 * kp;
                            b[i] = dbl[i] * xdbl[i] + dbc[i] * xdbc[i] + dbr[i] * xdbr[i] + dcl[i] * xdcl[i] + dcc[i] * xdcc[i] + dcr[i] * xdcr[i] + dfl[i] * xdfl[i] + dfc[i] * xdfc[i] + dfr[i] * xdfr[i] + cbl[i] * xcbl[i] + cbc[i] * xcbc[i] + cbr[i] * xcbr[i] + ccl[i] * xccl[i] + ccc[i] * xccc[i] + ccr[i] * xccr[i] + cfl[i] * xcfl[i] + cfc[i] * xcfc[i] + cfr[i] * xcfr[i] + ubl[i] * xubl[i] + ubc[i] * xubc[i] + ubr[i] * xubr[i] + ucl[i] * xucl[i] + ucc[i] * xucc[i] + ucr[i] * xucr[i] + ufl[i] * xufl[i] + ufc[i] * xufc[i] + ufr[i] * xufr[i];
                          }
                      }
                    for (t10 = __suif_max(t4, jmax - over1); t10 <= __suif_min(t4 + 23, jmax - 1); t10++)
                      {
                        for (t12 = t6; t12 <= __suif_min(imax - 1, t6 + 22); t12++)
                          {
                            i = t12 + t10 * jp + t8 * kp;
                            b[i] = dbl[i] * xdbl[i] + dbc[i] * xdbc[i] + dbr[i] * xdbr[i] + dcl[i] * xdcl[i] + dcc[i] * xdcc[i] + dcr[i] * xdcr[i] + dfl[i] * xdfl[i] + dfc[i] * xdfc[i] + dfr[i] * xdfr[i] + cbl[i] * xcbl[i] + cbc[i] * xcbc[i] + cbr[i] * xcbr[i] + ccl[i] * xccl[i] + ccc[i] * xccc[i] + ccr[i] * xccr[i] + cfl[i] * xcfl[i] + cfc[i] * xcfc[i] + cfr[i] * xcfr[i] + ubl[i] * xubl[i] + ubc[i] * xubc[i] + ubr[i] * xubr[i] + ucl[i] * xucl[i] + ucc[i] * xucc[i] + ucr[i] * xucr[i] + ufl[i] * xufl[i] + ufc[i] * xufc[i] + ufr[i] * xufr[i];
                          }
                      }
                  }
              }
          }
      }
    return;
  }
