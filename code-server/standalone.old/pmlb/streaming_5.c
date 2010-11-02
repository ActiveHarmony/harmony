#define en 19

typedef struct {int xs; int xe; int ys; int ye; int zs; int ze;} Internal_Coords;



void streaming_5(double *fi,int nx_local, int ny_local, int nz_local, Internal_Coords *i_c)
{
    int i,j,k,jj;


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


    int n_ip1jm1,ii;

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

    for (i=1; i<=imax;i++)
	{
        for(jj=1;jj<=jmax;jj++)
        {
            for(k=1; k<=kmax; k++)
            {
                n_i=i*(ny_local_plus);
                n_ip1=n_i+(ny_local_plus);
                j=jmax+1-jj;
                n_ij=(n_i+j)*(nz_local_plus);
                n_ip1jm1=(n_ip1+j-1)*(nz_local_plus);
                n_ijm1=(n_i+j-1)*(nz_local_plus);
                n_ijk=(n_ij+k)*en;
                //printf("5th____ nip1j: %d->%d\n",8+n_ijk, 8+(k+n_ip1jm1)*en);
                //printf("5th____ nip1j: %d->%d\n", 17+n_ijk,17+(k+1+n_ijm1)*en);
                fi[ 8+n_ijk]=fi[ 8+(k+n_ip1jm1)*en];
                fi[17+n_ijk]=fi[17+(k+1+n_ijm1)*en];

            }
		}
	}

}

