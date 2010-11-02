#define en 19

typedef struct {int xs; int xe; int ys; int ye; int zs; int ze;} Internal_Coords;



void streaming_3(double *fi,int nx_local, int ny_local, int nz_local, Internal_Coords *i_c)
{
    int i,j,k,kk;

    
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

    /*chill friendly*/
    
    for (i=1; i<=imax;i++)
	{
        for( j=1; j<=jmax; j++)
        {
            for(kk=1;kk<=kmax; kk++)
            {
                n_i=i*(ny_local_plus);
                n_ip1=n_i+(ny_local_plus);
                n_ij=(n_i+j)*(nz_local_plus);
                n_ijp1=n_ij+(nz_local_plus);
                n_ip1j=(n_ip1+j)*(nz_local_plus);
                k=kmax+1-kk;
                n_ijk=(n_ij+k)*en;
                fi[12+n_ijk]=fi[12+(k-1+n_ip1j)*en];
                fi[16+n_ijk]=fi[16+(k-1+n_ijp1)*en];
            }
        }
	}
    
    
}

