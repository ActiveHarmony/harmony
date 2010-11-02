#define en 19

typedef struct {int xs; int xe; int ys; int ye; int zs; int ze;} Internal_Coords;



void streaming_4(double *fi,int nx_local, int ny_local, int nz_local, Internal_Coords *i_c)
{
    int i,j,k,ii,jj;

    
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

    /*original*/
    /*
    for(i=i_c->xe;i>=i_c->xs;i--)
	{
	   int n_i=i*(ny_local+2);
	   int n_im1=n_i-(ny_local+2);	   
	   for( j=i_c->ys; j<=i_c->ye; j++)
	   {
 	      int n_ij=(n_i+j)*(nz_local+2);
		  int n_im1j=(n_im1+j)*(nz_local+2);
		  int n_im1jp1=n_im1j+(nz_local+2);
	      for(k=i_c->zs; k<=i_c->ze; k++)
		  {
			  int n_ijk=(n_ij+k)*en;
		      fi[ 9+n_ijk]=fi[ 9+(k+n_im1jp1)*en];		      
		      fi[13+n_ijk]=fi[13+(k+1+n_im1j)*en];
		  }
	   }
	}
    */
    /* chill friendly */
    
    for(ii=1;ii<=imax;ii++)
	{
        for( j=1; j<=jmax; j++)
        {
            for(k=1; k<=kmax; k++)
            {
                i=imax+1-ii;
                
                n_i=i*(ny_local_plus);
                n_im1=n_i-(ny_local_plus);
                
                n_ij=(n_i+j)*(nz_local_plus);
                n_im1j=(n_im1+j)*(nz_local_plus);
                n_im1jp1=n_im1j+(nz_local_plus);
                n_ijk=(n_ij+k)*en;
                //printf("%d -> %d, %d -> %d \n", 9+n_ijk,
                //9+(k+n_im1jp1)*en,13+n_ijk,13+(k+1+n_im1j)*en);
                //if(k==10) 
                //{
                //    printf("4th____ nip1j: %d->%d\n", 9+n_ijk,9+(k+n_im1jp1)*en);
                //    printf("4th____ nip1j: %d->%d\n",13+n_ijk, 13+(k+1+n_im1j)*en);
                // }
                fi[ 9+n_ijk]=fi[ 9+(k+n_im1jp1)*en];
                fi[13+n_ijk]=fi[13+(k+1+n_im1j)*en];
            }
        }
	}
    
}

