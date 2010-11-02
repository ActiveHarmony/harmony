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

extern void streaming_2(double *, int, int, int, struct __tmp_struct1 *);

extern void streaming_2(double *fi, int nx_local, int ny_local, int nz_local, struct __tmp_struct1 *i_c)
  {
    int i;
    int j;
    int k;
    int kk;
    int jj;
    int ii;
    int jjj;
    int kkk;
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
    int n_ip1jm1;
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

    ny_local_plus = ny_local + 2;
    nz_local_plus = nz_local + 2;
    imin = i_c->xs;
    imax = i_c->xe;
    jmin = i_c->ys;
    jmax = i_c->ye;
    kmin = i_c->zs;
    kmax = i_c->ze;
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
                        i = imax + 1 - t6;
                        n_i = i * ny_local_plus;
                        n_im1 = n_i - ny_local_plus;
                        j = jmax + 1 - t8;
                        n_ij = (n_i + j) * nz_local_plus;
                        n_ijm1 = n_ij - nz_local_plus;
                        n_im1j = (n_im1 + j) * nz_local_plus;
                        n_im1jm1 = n_im1j - nz_local_plus;
                        k = kmax + 1 - t10;
                        n_ijk = (n_ij + k) * 19;
                        fi[7 + n_ijk] = fi[7 + (k + n_im1jm1) * 19];
                        fi[11 + n_ijk] = fi[11 + (k - 1 + n_im1j) * 19];
                        fi[1 + n_ijk] = fi[1 + (k + n_im1j) * 19];
                        fi[15 + n_ijk] = fi[15 + (k - 1 + n_ijm1) * 19];
                        fi[3 + n_ijk] = fi[3 + (k + n_ijm1) * 19];
                        fi[5 + n_ijk] = fi[5 + (k - 1 + n_ij) * 19];
                        i = imax + 1 - t6;
                        n_i = i * ny_local_plus;
                        n_im1 = n_i - ny_local_plus;
                        j = jmax + 1 - t8;
                        n_ij = (n_i + j) * nz_local_plus;
                        n_ijm1 = n_ij - nz_local_plus;
                        n_im1j = (n_im1 + j) * nz_local_plus;
                        n_im1jm1 = n_im1j - nz_local_plus;
                        k = kmax + 1 - (t10 + 1);
                        n_ijk = (n_ij + k) * 19;
                        fi[7 + n_ijk] = fi[7 + (k + n_im1jm1) * 19];
                        fi[11 + n_ijk] = fi[11 + (k - 1 + n_im1j) * 19];
                        fi[1 + n_ijk] = fi[1 + (k + n_im1j) * 19];
                        fi[15 + n_ijk] = fi[15 + (k - 1 + n_ijm1) * 19];
                        fi[3 + n_ijk] = fi[3 + (k + n_ijm1) * 19];
                        fi[5 + n_ijk] = fi[5 + (k - 1 + n_ij) * 19];
                      }
                    if (1 <= over1)
                      {
                        i = imax + 1 - t6;
                        n_i = i * ny_local_plus;
                        n_im1 = n_i - ny_local_plus;
                        j = jmax + 1 - t8;
                        n_ij = (n_i + j) * nz_local_plus;
                        n_ijm1 = n_ij - nz_local_plus;
                        n_im1j = (n_im1 + j) * nz_local_plus;
                        n_im1jm1 = n_im1j - nz_local_plus;
                        k = kmax + 1 - kmax;
                        n_ijk = (n_ij + k) * 19;
                        fi[7 + n_ijk] = fi[7 + (k + n_im1jm1) * 19];
                        fi[11 + n_ijk] = fi[11 + (k - 1 + n_im1j) * 19];
                        fi[1 + n_ijk] = fi[1 + (k + n_im1j) * 19];
                        fi[15 + n_ijk] = fi[15 + (k - 1 + n_ijm1) * 19];
                        fi[3 + n_ijk] = fi[3 + (k + n_ijm1) * 19];
                        fi[5 + n_ijk] = fi[5 + (k - 1 + n_ij) * 19];
                      }
                  }
              }
          }
      }
    return;
  }
