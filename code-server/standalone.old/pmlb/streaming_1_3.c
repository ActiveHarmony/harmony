//#include "mpi.h"

#define en 19

typedef struct {int xs; int xe; int ys; int ye; int zs; int ze;} Internal_Coords;

void streaming_1_3(double *fi,int nx_local, int ny_local, int nz_local, Internal_Coords *i_c)
{
    int i,j,k,kk,ii;


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
    int myid=0;

    int n_ip1jm1,jj;

    register int ny_local_plus;
    register int nz_local_plus;

    ny_local_plus=ny_local+2;
    nz_local_plus=nz_local+2;

    imax=i_c->xe;
    jmax=i_c->ye;
    kmax=i_c->ze;

    myid=10000;
    
    /* chill friendly */
	for (i=1; i<=imax;i++)
	{ 
        for( j=1; j<=jmax; j++)
        {
            for(k=1; k<=kmax; k++)
            {
                n_i=i*(ny_local_plus);
                n_ip1=n_i+(ny_local_plus);
                n_ij=(n_i+j)*(nz_local_plus);
                n_ijp1=n_ij+(nz_local_plus);
                n_ip1j=(n_ip1+j)*(nz_local_plus);
                n_ip1jp1=n_ip1j+(nz_local_plus);
                n_ijk=(n_ij+k)*en;
               
                fi[ 6+n_ijk]=fi[ 6+ (k+1+ n_ij   )*en];
                fi[ 4+n_ijk]=fi[ 4+ (k  + n_ijp1 )*en];
                fi[18+n_ijk]=fi[18+ (k+1+ n_ijp1 )*en];
                fi[ 2+n_ijk]=fi[ 2+ (k+   n_ip1j )*en];
                fi[14+n_ijk]=fi[14+ (k+1+ n_ip1j )*en];
                fi[10+n_ijk]=fi[10+ (k+ +n_ip1jp1)*en];

                //3rd
                // have to map k
                //n_i=i*(ny_local_plus);
                //n_ip1=n_i+(ny_local_plus);
                //n_ij=(n_i+j)*(nz_local_plus);
                //n_ijp1=n_ij+(nz_local_plus);
                //n_ip1j=(n_ip1+j)*(nz_local_plus);
                kk=kmax+1-k;
                n_ijk=(n_ij+kk)*en;

                fi[12+n_ijk]=fi[12+(k-1+n_ip1j)*en];
                fi[16+n_ijk]=fi[16+(k-1+n_ijp1)*en];
            }
        }
	}
}

