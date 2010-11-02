



#define en 19

typedef struct {int xs; int xe; int ys; int ye; int zs; int ze;} Internal_Coords;



void streaming_2(double *fi,int nx_local, int ny_local, int nz_local, Internal_Coords *i_c)
{
    int i,j,k,kk,jj,ii,jjj,kkk;


    int imin, imax, jmin, jmax, kmin, kmax;

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

    register int ny_local_plus;
    register int nz_local_plus;

    ny_local_plus=ny_local+2;
    nz_local_plus=nz_local+2;

    imin=i_c->xs;
    imax=i_c->xe;

    jmin=i_c->ys;
    jmax=i_c->ye;

    kmin=i_c->zs;
    kmax=i_c->ze;


    /* original */


    for(ii=1;ii<=imax;ii++)
    {
        for(jj=1;jj<=jmax;jj++)
        {
            for(kk=1;kk<=kmax; kk++)
            {
                i=imax+1-ii;
                n_i=i*(ny_local_plus);
                n_im1=n_i-(ny_local_plus);
                j=jmax+1-jj;
                n_ij=(n_i+j)*(nz_local_plus);
                n_ijm1=n_ij-(nz_local_plus);
                n_im1j=(n_im1+j)*(nz_local_plus);
                n_im1jm1=n_im1j-(nz_local_plus);
                k=kmax+1-kk;
                n_ijk=(n_ij+k)*en;
 
                fi[ 7+n_ijk]=fi[ 7+(k  +n_im1jm1)*en];
                fi[11+n_ijk]=fi[11+(k-1+ n_im1j )*en];
                fi[ 1+n_ijk]=fi[ 1+(k  + n_im1j )*en];
                fi[15+n_ijk]=fi[15+(k-1+ n_ijm1 )*en];
                fi[ 3+n_ijk]=fi[ 3+(k  + n_ijm1 )*en];
                fi[ 5+n_ijk]=fi[ 5+(k-1+ n_ij   )*en];
            }
        }
    }
}

 
