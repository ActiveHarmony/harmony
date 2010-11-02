/*
 * This file was created automatically from SUIF
 *   on Thu Sep 30 23:41:29 2010.
 *
 * Created by:
 * s2c 1.3.0.1 (plus local changes) compiled Wed May 26 12:14:38 EDT 2010 by tiwari on spoon
 *     Based on SUIF distribution 1.3.0.1
 *     Linked with:
 *   libsuif1 1.3.0.1 (plus local changes) compiled Wed May 26 12:14:06 EDT 2010 by tiwari on spoon
 *   libuseful 1.3.0.1 (plus local changes) compiled Wed May 26 12:14:14 EDT 2010 by tiwari on spoon
 */


#define __suif_min(x,y) ((x)<(y)?(x):(y))

struct __tmp_struct1 { int xs;
                       int xe;
                       int ys;
                       int ye;
                       int zs;
                       int ze; };

extern void streaming_1_3(double *, int, int, int, struct __tmp_struct1 *);

extern void streaming_1_3(double *fi, int nx_local, int ny_local, int nz_local, struct __tmp_struct1 *i_c)
  {
    int i;
    int j;
    int k;
    int kk;
    int ii;
    int imin;
    int imax;
    int jmin;
    int jmax;
    int kmin;
    int kmax;
    int n_i;
    int n_ip1;
    int n_ij;
    int n_ijp1;
    int n_ip1j;
    int n_ip1jp1;
    int n_ijk;
    int n_im1;
    int n_ijm1;
    int n_im1j;
    int n_im1jm1;
    int n_im1jp1;
    int myid;
    int n_ip1jm1;
    int jj;
    int ny_local_plus;
    int nz_local_plus;
    int over1;
    int _t8;
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
    int t2;
    int t4;
    int t6;
    int t8;
    int t10;

    myid = 0;
    ny_local_plus = ny_local + 2;
    nz_local_plus = nz_local + 2;
    imax = i_c->xe;
    jmax = i_c->ye;
    kmax = i_c->ze;
    myid = 10000;
    over1 = 0;
    for (t2 = 1; t2 <= jmax; t2 += 48)
      {
        for (t4 = 1; t4 <= imax; t4 += 48)
          {
            for (t6 = t4; t6 <= __suif_min(t4 + 47, imax); t6++)
              {
                for (t8 = t2; t8 <= __suif_min(jmax, t2 + 47); t8++)
                  {
                    over1 = kmax % 2;
                    for (t10 = 1; t10 <= -over1 + kmax; t10 += 2)
                      {
                        n_i = t6 * ny_local_plus;
                        n_ip1 = n_i + ny_local_plus;
                        n_ij = (n_i + t8) * nz_local_plus;
                        n_ijp1 = n_ij + nz_local_plus;
                        n_ip1j = (n_ip1 + t8) * nz_local_plus;
                        n_ip1jp1 = n_ip1j + nz_local_plus;
                        n_ijk = (n_ij + t10) * 19;
                        fi[6 + n_ijk] = fi[6 + (t10 + 1 + n_ij) * 19];
                        fi[4 + n_ijk] = fi[4 + (t10 + n_ijp1) * 19];
                        fi[18 + n_ijk] = fi[18 + (t10 + 1 + n_ijp1) * 19];
                        fi[2 + n_ijk] = fi[2 + (t10 + n_ip1j) * 19];
                        fi[14 + n_ijk] = fi[14 + (t10 + 1 + n_ip1j) * 19];
                        fi[10 + n_ijk] = fi[10 + (t10 + n_ip1jp1) * 19];
                        kk = kmax + 1 - t10;
                        n_ijk = (n_ij + kk) * 19;
                        fi[12 + n_ijk] = fi[12 + (t10 - 1 + n_ip1j) * 19];
                        fi[16 + n_ijk] = fi[16 + (t10 - 1 + n_ijp1) * 19];
                        n_i = t6 * ny_local_plus;
                        n_ip1 = n_i + ny_local_plus;
                        n_ij = (n_i + t8) * nz_local_plus;
                        n_ijp1 = n_ij + nz_local_plus;
                        n_ip1j = (n_ip1 + t8) * nz_local_plus;
                        n_ip1jp1 = n_ip1j + nz_local_plus;
                        n_ijk = (n_ij + t10 + 1) * 19;
                        fi[6 + n_ijk] = fi[6 + (t10 + 1 + 1 + n_ij) * 19];
                        fi[4 + n_ijk] = fi[4 + (t10 + 1 + n_ijp1) * 19];
                        fi[18 + n_ijk] = fi[18 + (t10 + 1 + 1 + n_ijp1) * 19];
                        fi[2 + n_ijk] = fi[2 + (t10 + 1 + n_ip1j) * 19];
                        fi[14 + n_ijk] = fi[14 + (t10 + 1 + 1 + n_ip1j) * 19];
                        fi[10 + n_ijk] = fi[10 + (t10 + 1 + n_ip1jp1) * 19];
                        kk = kmax + 1 - (t10 + 1);
                        n_ijk = (n_ij + kk) * 19;
                        fi[12 + n_ijk] = fi[12 + (t10 + 1 - 1 + n_ip1j) * 19];
                        fi[16 + n_ijk] = fi[16 + (t10 + 1 - 1 + n_ijp1) * 19];
                      }
                    if (1 <= over1)
                      {
                        n_i = t6 * ny_local_plus;
                        n_ip1 = n_i + ny_local_plus;
                        n_ij = (n_i + t8) * nz_local_plus;
                        n_ijp1 = n_ij + nz_local_plus;
                        n_ip1j = (n_ip1 + t8) * nz_local_plus;
                        n_ip1jp1 = n_ip1j + nz_local_plus;
                        n_ijk = (n_ij + kmax) * 19;
                        fi[6 + n_ijk] = fi[6 + (kmax + 1 + n_ij) * 19];
                        fi[4 + n_ijk] = fi[4 + (kmax + n_ijp1) * 19];
                        fi[18 + n_ijk] = fi[18 + (kmax + 1 + n_ijp1) * 19];
                        fi[2 + n_ijk] = fi[2 + (kmax + n_ip1j) * 19];
                        fi[14 + n_ijk] = fi[14 + (kmax + 1 + n_ip1j) * 19];
                        fi[10 + n_ijk] = fi[10 + (kmax + n_ip1jp1) * 19];
                        kk = kmax + 1 - kmax;
                        n_ijk = (n_ij + kk) * 19;
                        fi[12 + n_ijk] = fi[12 + (kmax - 1 + n_ip1j) * 19];
                        fi[16 + n_ijk] = fi[16 + (kmax - 1 + n_ijp1) * 19];
                      }
                  }
              }
          }
      }
    return;
  }
