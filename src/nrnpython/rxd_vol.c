#include <stdio.h>
#include <assert.h>
#include "grids.h"
#include "rxd.h"
#include <pthread.h>
#ifdef __APPLE__
#include <Python/Python.h>
#else
#include <Python.h>
#endif

/*Tortuous diffusion coefficients*/
#define DcX(x,y,z) (g.dc_x/LAMBDA(x,y,z))
#define DcY(x,y,z) (g.dc_y/LAMBDA(x,y,z))
#define DcZ(x,y,z) (g.dc_z/LAMBDA(x,y,z))

/*Flux in the x,y,z directions*/
#define Fxx(x1,x2) (ALPHA(x1,y,z)*g.dc_x*(g.states[IDX(x1,y,z)] - g.states[IDX(x2,y,z)])/LAMBDA(x1,y,z))
#define Fxy(y1,y1d,y2) (ALPHA(x,y1d,z)*g.dc_y*(g.states[IDX(x,y1,z)] - g.states[IDX(x,y2,z)])/LAMBDA(x,y1d,z))
#define Fxz(z1,z1d,z2) (ALPHA(x,y,z1d)*g.dc_z*(g.states[IDX(x,y,z1)] - g.states[IDX(x,y,z2)])/LAMBDA(x,y,z1d))
#define Fyy(y1,y2) (ALPHA(x,y1,z)*g.dc_y*(g.states[IDX(x,y1,z)] - g.states[IDX(x,y2,z)])/LAMBDA(x,y1,z))
#define Fzz(z1,z2) (ALPHA(x,y,z1)*g.dc_z*(g.states[IDX(x,y,z1)] - g.states[IDX(x,y,z2)])/LAMBDA(x,y,z1))


extern int NUM_THREADS;
/* solve Ax=b where A is a diagonally dominant tridiagonal matrix
 * N	-	length of the matrix
 * l_diag	-	pointer to the lower diagonal (length N-1)
 * diag	-	pointer to  diagonal	(length N)
 * u_diag	-	pointer to the upper diagonal (length N-1)
 * B	-	pointer to the RHS	(length N)
 * The solution (x) will be stored in B.
 * l_diag, diag and u_diag are not changed.
 * c	-	scratch pad, preallocated memory for (N-1) doubles
 */
static int solve_dd_tridiag(int N, const double* l_diag, const double* diag, 
	const double* u_diag, double* b, double *c)
{
	int i;
	
	c[0] = u_diag[0]/diag[0];
	b[0] = b[0]/diag[0];

	for(i=1;i<N-1;i++)
	{
		c[i] = u_diag[i]/(diag[i]-l_diag[i-1]*c[i-1]);
		b[i] = (b[i]-l_diag[i-1]*b[i-1])/(diag[i]-l_diag[i-1]*c[i-1]);
	}
	b[N-1] = (b[N-1]-l_diag[N-2]*b[N-2])/(diag[N-1]-l_diag[N-2]*c[N-2]);


	/*back substitution*/
	for(i=N-2;i>=0;i--)
	{
		b[i]=b[i]-c[i]*b[i+1];	
	}
	return 0;
}


/* dg_adi_vol_x performs the first of 3 steps in DG-ADI
 * g    -   the parameters and state of the grid
 * dt   -   the time step
 * y    -   the index for the y plane
 * z    -   the index for the z plane
 * state    -   where the output of this step is stored
 * scratch  - scratchpad array of doubles, length 2*g.size_x - 1
 * like dg_adi_x except the grid has a variable volume fraction
 * g.alpha and my have variable tortuosity g.lambda
 */
static AdiLineData dg_adi_vol_x(Grid_node g, const double dt, const int y, const int z, double const * const state, double* const scratch)
{
	int yp,ypd,ym,ymd,zp,zpd,zm,zmd,div_y,div_z;
	int i,x,N=2*g.size_x-1;
	double frac;
	unsigned char* fp = malloc(N*sizeof(unsigned char));
		/*an array indicating if a point is an additional point (TRUE) that should not be stored in state
		 *or and original point (FALSE) that should be stored in state*/
	double *RHS;
	double *diag = malloc(N*sizeof(double));
	double *l_diag = malloc((N-1)*sizeof(double));
	double *u_diag = malloc((N-1)*sizeof(double));

	double scale_lower=1., add_diag=0;
	
	AdiLineData result;
    result.copyFrom = diag;
    result.copyTo = IDX(0, y, z);


	for(x=0,i=0; x<g.size_x-1; x++)
	{
		frac = ALPHA(x+1,y,z)/ALPHA(x,y,z);
		if(frac<=1.)
		{
			if(x==0)
			{
				diag[0] = 1. + 0.5*dt*DcX(1,y,z)/SQ(g.dx);
				u_diag[0] = -0.5*dt*DcX(1,y,z)/SQ(g.dx);
			}
			else
			{
				l_diag[i-1] = -dt*DcX(x,y,z)*scale_lower/(2.*SQ(g.dx));
				diag[i] = 1. + dt*(DcX(x,y,z) + DcX(x+1,y,z) + add_diag)/(2.*SQ(g.dx));
				u_diag[i] = -dt*DcX(x+1,y,z)/(2.*SQ(g.dx));
			}
			fp[i++]=FALSE;
		}

		if(ALPHA(x,y,z) == ALPHA(x+1,y,z))
		{
			/*An additional point is not needed - so N is reduced*/
			N--;
			add_diag = 0.;
			scale_lower = 1.;
		}
		else if(frac<1.)
		{
			/*additional grid point to match the flux*/
			l_diag[i-1] = 1. - frac;
			diag[i] = -1.;
			u_diag[i] = frac;
			fp[i++] = TRUE;
			
			/*rescale the point before following the additional point*/
			add_diag = DcX(x+1,y,z)*frac/(1.-frac);
			scale_lower = 1./(1.-frac);
		}
		else
		{
			/*rescale the point before the additional point*/
			if(x==0)
			{
				diag[0] = 1. + dt*(DcX(1,y,z) + DcX(1,y,z)/(frac-1.))/(2.*SQ(g.dx));
				u_diag[0] = -dt*DcX(1,y,z)*(frac/(frac-1.))/(2.*SQ(g.dx));
			}
			else
			{
				l_diag[i-1] = -dt*DcX(x,y,z)*scale_lower/(2.*SQ(g.dx));
				diag[i] = 1. + dt*(DcX(x,y,z) + DcX(x+1,y,z) + add_diag + DcX(x+1,y,z)/(frac-1.))/(2.*SQ(g.dx));
				u_diag[i] = -dt*DcX(x+1,y,z)*(frac/(frac-1.))/(2.*SQ(g.dx));
			}
			fp[i++] = FALSE;

			/*additional grid point to match the flux*/
			l_diag[i-1] = 1./frac;
			diag[i] = -1.;
			u_diag[i] = (frac-1.)/frac;
			fp[i++]=TRUE;

			add_diag = 0.;
			scale_lower = 1.;
		}
	}

	
	/*zero flux boundary condition*/
	yp = (y==g.size_y-1)?y-1:y+1;
	ypd = (y==g.size_y-1)?y:y+1;
	ym = (y==0)?y+1:y-1;
	ymd = (y==0)?1:y;
	zp = (z==g.size_z-1)?z-1:z+1;
	zpd = (z==g.size_z-1)?z:z+1;
	zm = (z==0)?z+1:z-1;
	zmd = (z==0)?1:z;


	div_y = (y==0||y==g.size_y-1)?2.*SQ(g.dy):SQ(g.dy);
	div_z = (z==0||z==g.size_z-1)?2.*SQ(g.dz):SQ(g.dz);

	diag[N-1] = 1. + 0.5*dt*(add_diag+DcX(g.size_x-1,y,z))/SQ(g.dx);
	l_diag[N-2] = -0.5*dt*scale_lower*DcX(g.size_x-1,y,z)/SQ(g.dx);
	fp[N-1]=FALSE;

	RHS = malloc(N*sizeof(double));
	x=0;
	RHS[0] = g.states[IDX(0,y,z)] + (dt/ALPHA(0,y,z))*
					((Fxx(1,x)/(2.*SQ(g.dx))) 
				+    (Fxy(yp,ypd,y) - Fxy(y,ymd,ym))/div_y 
				+ 	 (Fxz(zp,zpd,z) - Fxz(z,zmd,zm))/div_z);
	for(i=1,x=1;i<N-1;i++)
	{
		if(fp[i])
		{
			RHS[i] = 0;
		}
		else
		{
			RHS[i] =  g.states[IDX(x,y,z)] + (dt/ALPHA(x,y,z))*
					((Fxx(x+1,x) - Fxx(x,x-1))/(2.*SQ(g.dx)) 
				+    (Fxy(yp,ypd,y) - Fxy(y,ymd,ym))/div_y 
				+ 	 (Fxz(zp,zpd,z) - Fxz(z,zmd,zm))/div_z);
			x++;
		}
	}
	RHS[N-1]  = g.states[IDX(x,y,z)] + (dt/ALPHA(x,y,z))*
				((-Fxx(x,x-1))/(2.*SQ(g.dx)) 
			+    (Fxy(yp,ypd,y) - Fxy(y,ymd,ym))/div_y 
			+ 	 (Fxz(zp,zpd,z) - Fxz(z,zmd,zm))/div_z);
	
	solve_dd_tridiag(N, l_diag, diag, u_diag, RHS, scratch);
	
	for(i=0,x=0;i<N;i++)
	{
		if(!fp[i])
		{
			diag[x++]=RHS[i];
		}
	}
	
	free(RHS);
	free(l_diag);
	free(u_diag);
	free(fp);

	return result;
}

/* dg_adi_vol_y performs the second of 3 steps in DG-ADI
 * g    -   the parameters and state of the grid
 * dt   -   the time step
 * y    -   the index for the y plane
 * z    -   the index for the z plane
 * state    -   where the output of this step is stored
 * scratch  - scratchpad array of doubles, length 2*g.size_y - 1
 * like dg_adi_y except the grid has a variable volume fraction
 * g.alpha and my have variable tortuosity g.lambda
 */
static AdiLineData dg_adi_vol_y(Grid_node g, double const dt, int const x, int const z, double const * const state, double* const scratch)
{
	int i,y,N=2*g.size_y-1;
	double frac;
	unsigned char* fp = malloc(N*sizeof(unsigned char));
	double *RHS;
	double *diag = malloc(N*sizeof(double));
	double *l_diag = malloc((N-1)*sizeof(double));
	double *u_diag = malloc((N-1)*sizeof(double));
	double scale_lower=1., add_diag=0;

    AdiLineData result;
    result.copyFrom = diag;
    result.copyTo = IDX(x, 0, z);

	for(y=0,i=0; y<g.size_y-1; y++)
	{
		frac = ALPHA(x,y+1,z)/ALPHA(x,y,z);
		if(frac<=1.)
		{
			if(y==0)
			{
				diag[0] = 1. + 0.5*dt*DcY(x,1,z)/SQ(g.dy);
				u_diag[0] = -0.5*dt*DcY(x,1,z)/SQ(g.dy);
			}
			else
			{
				l_diag[i-1] = -dt*DcY(x,y,z)*scale_lower/(2.*SQ(g.dy));
				diag[i] = 1. + dt*(DcY(x,y,z)+DcY(x,y+1,z) + add_diag)/(2.*SQ(g.dy));
				u_diag[i] = -dt*DcY(x,y+1,z)/(2.*SQ(g.dy));
			}
			fp[i++]=FALSE;
		}

		if(ALPHA(x,y,z) == ALPHA(x,y+1,z))
		{
			/*An additional point is not needed - so N is reduced*/
			N--;
			add_diag = 0.;
			scale_lower = 1.;
		}
		else if(frac<1.)
		{
			/*additional grid point to match the flux*/
			l_diag[i-1] = 1. - frac;
			diag[i] = -1.;
			u_diag[i] = frac;
			fp[i++] = TRUE;
			
			/*rescale the point before following the additional point*/
			add_diag = DcY(x,y+1,z)*frac/(1.-frac);
			scale_lower = 1./(1.-frac);
		}
		else
		{
			/*rescale the point before the additional point*/
			if(y==0)
			{
				diag[0] = 1. + dt*(DcY(x,1,z) + DcY(x,1,z)/(frac-1.))/(2.*SQ(g.dy));
				u_diag[0] = -dt*DcY(x,1,z)*(frac/(frac-1.))/(2.*SQ(g.dy));

			}
			else
			{
				l_diag[i-1] = -dt*DcY(x,y,z)*scale_lower/(2.*SQ(g.dy));
				diag[i] = 1. + dt*(DcY(x,y,z) + DcY(x,y+1,z) + add_diag + DcY(x,y+1,z)/(frac-1.))/(2.*SQ(g.dy));
				u_diag[i] = -dt*DcY(x,y+1,z)*(frac/(frac-1.))/(2.*SQ(g.dy));
			}
			fp[i++] = FALSE;

			/*additional grid point to match the flux*/
			l_diag[i-1] = 1./frac;
			diag[i] = -1.;
			u_diag[i] = (frac-1.)/frac;
			fp[i++]=TRUE;

			add_diag = 0.;
			scale_lower = 1.;
		}
	}


	/*zero flux boundary condition*/
	diag[N-1] = 1. + 0.5*dt*(add_diag+DcY(x,g.size_y-1,z))/SQ(g.dy);
	l_diag[N-2] = -0.5*dt*scale_lower*DcY(x,g.size_y-1,z)/SQ(g.dy);
	fp[N-1]=FALSE;


	RHS = malloc(N*sizeof(double));
	RHS[0] =  state[IDX(x,0,z)] + dt*Fyy(1,0)/(2.*SQ(g.dy)*ALPHA(x,0,z));
	for(i=1,y=1;i<N-1;i++)
	{
		if(fp[i])
		{
			RHS[i]=0;
		}
		else
		{
			RHS[i] =  state[IDX(x,y,z)] + (dt/ALPHA(x,y,z))*(Fyy(y+1,y) - Fyy(y,y-1))/(2.*SQ(g.dy));
			y++;
		}
	}
	RHS[N-1] =  state[IDX(x,y,z)] - (dt/ALPHA(x,y,z))*Fyy(y,y-1)/(2.*SQ(g.dy));

	solve_dd_tridiag(N, l_diag, diag, u_diag, RHS, scratch);

	for(i=0,y=0;i<N;i++)
	{
		if(!fp[i])
		{
			diag[y++]=RHS[i];
		}
	}
	
	free(RHS);
	free(l_diag);
	free(u_diag);
	free(fp);

	return result;
}

/* dg_adi_vol_z performs the third of 3 steps in DG-ADI
 * g    -   the parameters and state of the grid
 * dt   -   the time step
 * y    -   the index for the y plane
 * z    -   the index for the z plane
 * state    -   where the output of this step is stored
 * scratch  - scratchpad array of doubles, length 2*g.size_y - 1
 * like dg_adi_z except the grid has a variable volume fraction
 * g.alpha and my have variable tortuosity g.lambda
 */
static AdiLineData dg_adi_vol_z(Grid_node g, double const dt, int const x, int const y, double const * const state, double* const scratch)
{
	int i,z,N=2*g.size_z-1;
	double frac;
	unsigned char* fp = malloc(N*sizeof(unsigned char));
	double *RHS;
	double *diag = malloc(N*sizeof(double));
	double *l_diag = malloc((N-1)*sizeof(double));
	double *u_diag = malloc((N-1)*sizeof(double));
	double scale_lower=1., add_diag=0;

	AdiLineData result;
    result.copyFrom = diag;
    result.copyTo = IDX(x, y, 0);


	for(z=0,i=0; z<g.size_z-1; z++)
	{
		frac = ALPHA(x,y,z+1)/ALPHA(x,y,z);
		if(frac<=1.)
		{
			if(z==0)
			{
				diag[0] = 1. + 0.5*dt*DcZ(x,y,1)/SQ(g.dz);
				u_diag[0] = -0.5*dt*DcZ(x,y,1)/SQ(g.dz);
			}
			else
			{
				l_diag[i-1] = -dt*DcZ(x,y,z)*scale_lower/(2.*SQ(g.dz));
				diag[i] = 1. + dt*(DcZ(x,y,z)+DcZ(x,y,z+1) + add_diag)/(2.*SQ(g.dz));
				u_diag[i] = -dt*DcZ(x,y,z+1)/(2.*SQ(g.dz));
			}
			fp[i++]=FALSE;
		}

		if(ALPHA(x,y,z) == ALPHA(x,y,z+1))
		{
			/*An additional point is not needed - so N is reduced*/
			N--;
			add_diag = 0.;
			scale_lower = 1.;
		}
		else if(frac<1.)
		{
			/*additional grid point to match the flux*/
			l_diag[i-1] = 1. - frac;
			diag[i] = -1.;
			u_diag[i] = frac;
			fp[i++] = TRUE;
			
			/*rescale the point before following the additional point*/
			add_diag = DcZ(x,y,z+1)*frac/(1.-frac);
			scale_lower = 1./(1.-frac);
		}
		else
		{
			/*rescale the point before the additional point*/
			if(z==0)
			{
				diag[0] = 1. + dt*(DcZ(x,y,1) + DcZ(x,y,1)/(frac-1.))/(2.*SQ(g.dz));
				u_diag[0] = -dt*DcZ(x,y,1)*(frac/(frac-1.))/(2.*SQ(g.dz));

			}
			else
			{
				l_diag[i-1] = -dt*DcZ(x,y,z)*scale_lower/(2.*SQ(g.dz));
				diag[i] = 1. + dt*(DcZ(x,y,z) + DcZ(x,y,z+1) + add_diag + DcZ(x,y,z+1)/(frac-1.))/(2.*SQ(g.dz));
				u_diag[i] = -dt*DcZ(x,y,z+1)*(frac/(frac-1.))/(2.*SQ(g.dz));
			}
			fp[i++] = FALSE;

			/*additional grid point to match the flux*/
			l_diag[i-1] = 1./frac;
			diag[i] = -1.;
			u_diag[i] = (frac-1.)/frac;
			fp[i++]=TRUE;

			add_diag = 0.;
			scale_lower = 1.;
		}
	}


	/*zero flux boundary condition*/
	diag[N-1] = 1. + 0.5*dt*(add_diag+DcZ(x,y,g.size_z-1))/SQ(g.dz);
	l_diag[N-2] = -0.5*dt*scale_lower*DcZ(x,y,g.size_z-1)/SQ(g.dz);
	fp[N-1]=FALSE;

	RHS = malloc(N*sizeof(double));
	
	RHS[0] = state[IDX(x,y,0)] + dt*Fzz(1,0)/(2.*SQ(g.dz)*ALPHA(x,y,0));
	for(i=1,z=1;i<N-1;i++)
	{
		if(fp[i])
		{
			RHS[i]=0;
		}
		else
		{
			RHS[i] =  state[IDX(x,y,z)] + (dt/ALPHA(x,y,z))*(Fzz(z+1,z) - Fzz(z,z-1))/(2.*SQ(g.dz));
			z++;
		}
	}
	RHS[N-1] =  state[IDX(x,y,z)] - (dt/ALPHA(x,y,z))*Fzz(z,z-1)/(2.*SQ(g.dz));

	solve_dd_tridiag(N, l_diag, diag, u_diag, RHS, scratch);

	for(i=0,z=0;i<N;i++)
	{
		if(!fp[i])
		{
			diag[z++] = RHS[i];
		}
	}

	free(RHS);
	free(l_diag);
	free(u_diag);
	free(fp);

	return result;
}

/*DG-ADI implementation the 3 step process to diffusion species in grid g by time step *dt_ptr
 * g    -   the state and parameters
 * like dg_adi execpt the Grid_node g has variable volume fraction g.alpha and may have
 * variable tortuosity g.lambda
 */
int dg_adi_vol(Grid_node g)
{
	double* state = malloc(sizeof(double) * g.size_x * g.size_y * g.size_z);
    AdiLineData* vals = malloc(sizeof(AdiLineData) * g.size_y * g.size_z);
    pthread_t* thread = malloc(sizeof(pthread_t) * NUM_THREADS);
    AdiGridData* tasks = malloc(sizeof(AdiGridData) * NUM_THREADS);

    /* first step: advance the x direction */
    run_threaded_dg_adi(tasks, thread, g.size_y, g.size_z, g, state, vals, dg_adi_vol_x, g.size_x);

    /* transfer data */
    dg_transfer_data(vals, state, g.size_x, g.size_y * g.size_z, g.size_y * g.size_z);

    /* Adjust memory */
    free(vals);
    vals = malloc(sizeof(AdiLineData) * g.size_x * g.size_z);

    /* second step: advance the y direction */
    run_threaded_dg_adi(tasks, thread, g.size_x, g.size_z, g, state, vals, dg_adi_vol_y, g.size_y);

    /* transfer data */
    dg_transfer_data(vals, state, g.size_y, g.size_x * g.size_z, g.size_z);

    /* Adjust memory */
    free(vals);
    vals = malloc(sizeof(AdiLineData) * g.size_x * g.size_y);

    /* third step: advance the z direction */
    run_threaded_dg_adi(tasks, thread, g.size_x, g.size_y, g, state, vals, dg_adi_vol_z, g.size_z);

    /* transfer data directly into Grid_node values (g.states) */
    dg_transfer_data(vals, g.states, g.size_z, g.size_x * g.size_y, 1);

    free(state);
    free(vals);
    free(thread);
    free(tasks);

    return 0;
}

/* dg_adi_tort_x performs the first of 3 steps in DG-ADI
 * g    -   the parameters and state of the grid
 * dt   -   the time step
 * y    -   the index for the y plane
 * z    -   the index for the z plane
 * state    -   where the output of this step is stored
 * scratch  - scratchpad array of doubles, length g.size_x - 1
 * like dg_adi_x except the grid has a variable tortuosity
 * g.lambda (but it still has fixed volume fraction)
 */
static AdiLineData dg_adi_tort_x(Grid_node g, const double dt, const int y, const int z, double const * const state, double* const scratch)
{
	int yp,ypd,ym,ymd,zp,zpd,zm,zmd,div_y,div_z;
	int x;
	double *RHS = malloc(g.size_x*sizeof(double));
	double *diag = malloc(g.size_x*sizeof(double));
	double *l_diag = malloc((g.size_x-1)*sizeof(double));
	double *u_diag = malloc((g.size_x-1)*sizeof(double));

	AdiLineData result;
    result.copyFrom = RHS;
    result.copyTo = IDX(0, y, z);


	for(x=1;x<g.size_x-1;x++)
	{
		l_diag[x-1] = -dt*DcX(x,y,z)/(2.*SQ(g.dx));
		diag[x] = 1. + dt*(DcX(x,y,z) + DcX(x+1,y,z))/(2.*SQ(g.dx));
		u_diag[x] = -dt*DcX(x+1,y,z)/(2.*SQ(g.dx));
	}

	/*zero flux boundary condition*/
	yp = (y==g.size_y-1)?y-1:y+1;
	ypd = (y==g.size_y-1)?y:y+1;
	ym = (y==0)?y+1:y-1;
	ymd = (y==0)?1:y;
	zp = (z==g.size_z-1)?z-1:z+1;
	zpd = (z==g.size_z-1)?z:z+1;
	zm = (z==0)?z+1:z-1;
	zmd = (z==0)?1:z;

	div_y = (y==0||y==g.size_y-1)?2.:1.;
	div_z = (z==0||z==g.size_z-1)?2.:1.;

	diag[0] = 1. + 0.5*dt*DcX(1,y,z)/SQ(g.dx);
	u_diag[0] = -0.5*dt*DcX(1,y,z)/SQ(g.dx);
	diag[g.size_x-1] = 1. + 0.5*dt*DcX(g.size_x-1,y,z)/SQ(g.dx);
	l_diag[g.size_x-2] = -0.5*dt*DcX(g.size_x-1,y,z)/SQ(g.dx);



	RHS[0] =  g.states[IDX(0,y,z)]
			+ dt*((DcX(1,y,z)*g.states[IDX(1,y,z)] - DcX(1,y,z)*g.states[IDX(0,y,z)])/(2.*SQ(g.dx))
			+	  (DcY(0,ypd,z)*g.states[IDX(0,yp,z)] - (DcY(0,ypd,z)+DcY(0,ymd,z))*g.states[IDX(0,y,z)] + DcY(0,ymd,z)*g.states[IDX(0,ym,z)])/(div_y*SQ(g.dy))
			+	  (DcZ(0,y,zpd)*g.states[IDX(0,y,zp)] - (DcZ(0,y,zpd)+DcZ(0,y,zmd))*g.states[IDX(0,y,z)] + DcZ(0,y,z)*g.states[IDX(0,y,zm)])/(div_z*SQ(g.dz)));

	for(x=1;x<g.size_x-1;x++)
	{
		RHS[x] =  g.states[IDX(x,y,z)]
			+ dt*((DcX(x+1,y,z)*g.states[IDX(x+1,y,z)] - (DcX(x+1,y,z)+DcX(x,y,z))*g.states[IDX(x,y,z)] + DcX(x,y,z)*g.states[IDX(x-1,y,z)])/(2.*SQ(g.dx))
			+	  (DcY(x,ypd,z)*g.states[IDX(x,yp,z)] - (DcY(x,ypd,z)+DcY(x,ymd,z))*g.states[IDX(x,y,z)] + DcY(x,ymd,z)*g.states[IDX(x,ym,z)])/(div_y*SQ(g.dy))
			+	  (DcZ(x,y,zpd)*g.states[IDX(x,y,zp)] - (DcZ(x,y,zpd)+DcZ(x,y,zmd))*g.states[IDX(x,y,z)] + DcZ(x,y,zmd)*g.states[IDX(x,y,zm)])/(div_z*SQ(g.dz)));
	}
	RHS[x] =  g.states[IDX(x,y,z)]
				+ dt*((-DcX(x,y,z)*g.states[IDX(x,y,z)] + DcX(x,y,z)*g.states[IDX(x-1,y,z)])/(2.*SQ(g.dx))
				+	  (DcY(x,ypd,z)*g.states[IDX(x,yp,z)] - (DcY(x,ypd,z)+DcY(x,ymd,z))*g.states[IDX(x,y,z)] + DcY(x,ymd,z)*g.states[IDX(x,ym,z)])/(div_y*SQ(g.dy))
				+	  (DcZ(x,y,zpd)*g.states[IDX(x,y,zp)] - (DcZ(x,y,zpd)+DcZ(x,y,zmd))*g.states[IDX(x,y,z)] + DcZ(x,y,zmd)*g.states[IDX(x,y,zm)])/(div_z*SQ(g.dz)));

	solve_dd_tridiag(g.size_x, l_diag, diag, u_diag, RHS, scratch);
	
	free(diag);
	free(l_diag);
	free(u_diag);

	return result;
}

/* dg_adi_tort_y performs the second of 3 steps in DG-ADI
 * g    -   the parameters and state of the grid
 * dt   -   the time step
 * y    -   the index for the y plane
 * z    -   the index for the z plane
 * state    -   where the output of this step is stored
 * scratch  - scratchpad array of doubles, length g.size_y - 1
 * like dg_adi_y except the grid has a variable tortuosity
 * g.lambda (but it still has fixed volume fraction)
 */
static AdiLineData dg_adi_tort_y(Grid_node g, double const dt, int const x, int const z, double const * const state, double* const scratch)
{
	int y;
	double *RHS = malloc(g.size_y*sizeof(double));
	double *diag = malloc(g.size_y*sizeof(double));
	double *l_diag = malloc((g.size_y-1)*sizeof(double));
	double *u_diag = malloc((g.size_y-1)*sizeof(double));

    AdiLineData result;
    result.copyFrom = RHS;
    result.copyTo = IDX(x, 0, z);

	for(y=1;y<g.size_y-1;y++)
	{
		l_diag[y-1] = -dt*DcY(x,y,z)/(2.*SQ(g.dy));
		diag[y] = 1. + dt*(DcY(x,y,z)+DcY(x,y+1,z))/(2.*SQ(g.dy));
		u_diag[y] = -dt*DcY(x,y+1,z)/(2.*SQ(g.dy));
	}


	/*zero flux boundary condition*/
	diag[0] = 1. + 0.5*dt*DcY(x,1,z)/SQ(g.dy);
	u_diag[0] = -0.5*dt*DcY(x,1,z)/SQ(g.dy);
	diag[g.size_y-1] = 1. + 0.5*dt*DcY(x,g.size_y-1,z)/SQ(g.dy);
	l_diag[g.size_y-2] = -0.5*dt*DcY(x,g.size_y-1,z)/SQ(g.dy);



	RHS[0] =  state[IDX(x,0,z)]+ dt*((DcY(1,y,z)*g.states[IDX(x,1,z)] - DcY(1,y,z)*g.states[IDX(x,0,z)])/(2.*SQ(g.dy)));
	for(y=1;y<g.size_y-1;y++)
	{
		RHS[y] =  state[IDX(x,y,z)]
			+	  dt*(DcY(x,y+1,z)*g.states[IDX(x,y+1,z)] - (DcY(x,y+1,z)+DcY(x,y,z))*g.states[IDX(x,y,z)] + DcY(x,y,z)*g.states[IDX(x,y-1,z)])/(2.*SQ(g.dy));

	}
	RHS[y] =  state[IDX(x,y,z)]
		+	  dt*(-DcY(x,y,z)*g.states[IDX(x,y,z)] + DcY(x,y,z)*g.states[IDX(x,y-1,z)])/(2.*SQ(g.dy));

	
	solve_dd_tridiag(g.size_y, l_diag, diag, u_diag, RHS, scratch);

	free(diag);
	free(l_diag);
	free(u_diag);

	return result;
}

/* dg_adi_tort_z performs the second of 3 steps in DG-ADI
 * g    -   the parameters and state of the grid
 * dt   -   the time step
 * y    -   the index for the y plane
 * z    -   the index for the z plane
 * state    -   where the output of this step is stored
 * scratch  - scratchpad array of doubles, length g.size_z - 1
 * like dg_adi_z except the grid has a variable tortuosity
 * g.lambda (but it still has fixed volume fraction)
 */
static AdiLineData dg_adi_tort_z(Grid_node g, double const dt, int const x, int const y, double const * const state, double* const scratch)
{
	int z;
	double *RHS = malloc(g.size_z*sizeof(double));
	double *diag = malloc(g.size_z*sizeof(double));
	double *l_diag = malloc((g.size_z-1)*sizeof(double));
	double *u_diag = malloc((g.size_z-1)*sizeof(double));

	AdiLineData result;
    result.copyFrom = RHS;
    result.copyTo = IDX(x, y, 0);


	for(z=1;z<g.size_z-1;z++)
	{
		l_diag[z-1] = -dt*DcZ(x,y,z)/(2.*SQ(g.dz));
		diag[z] = 1. + dt*(DcZ(x,y,z)+DcZ(x,y,z+1))/(2.*SQ(g.dz));
		u_diag[z] = -dt*DcZ(x,y,z+1)/(2.*SQ(g.dz));
	}

	/*zero flux boundary condition*/
	diag[0] = 1. + 0.5*dt*DcZ(x,y,1)/SQ(g.dz);
	u_diag[0] = -0.5*dt*DcZ(x,y,1)/SQ(g.dz);
	diag[g.size_z-1] = 1. + 0.5*dt*DcZ(x,y,g.size_z-1)/SQ(g.dz);
	l_diag[g.size_z-2] = -0.5*dt*DcZ(x,y,g.size_z-1)/SQ(g.dz);

	RHS[0] =  state[IDX(x,y,0)] + dt*((DcZ(x,y,1)*g.states[IDX(x,y,1)] - DcZ(x,y,1)*g.states[IDX(x,y,0)])/(2.*SQ(g.dz)));

	for(z=1;z<g.size_z-1;z++)
	{
		RHS[z] =  state[IDX(x,y,z)]
			+	  dt*(DcZ(x,y,z+1)*g.states[IDX(x,y,z+1)] - (DcZ(x,y,z+1)+DcZ(x,y,z))*g.states[IDX(x,y,z)] + DcZ(x,y,z)*g.states[IDX(x,y,z-1)])/(2.*SQ(g.dz));

	}
	RHS[z] =  state[IDX(x,y,z)]
				+	  dt*(-DcZ(x,y,z)*g.states[IDX(x,y,z)] + DcZ(x,y,z)*g.states[IDX(x,y,z-1)])/(2.*SQ(g.dz));

	solve_dd_tridiag(g.size_z, l_diag, diag, u_diag, RHS, scratch);

	free(diag);
	free(l_diag);
	free(u_diag);

	return result;
}


/*DG-ADI implementation the 3 step process to diffusion species in grid g by time step *dt_ptr
 * g    -   the state and parameters
 * like dg_adi except the grid node g has variable tortuosity g.lambda (but fixed volume
 * fraction)
 */
int dg_adi_tort(Grid_node g)
{
	double* state = malloc(sizeof(double) * g.size_x * g.size_y * g.size_z);
    AdiLineData* vals = malloc(sizeof(AdiLineData) * g.size_y * g.size_z);
    pthread_t* thread = malloc(sizeof(pthread_t) * NUM_THREADS);
    AdiGridData* tasks = malloc(sizeof(AdiGridData) * NUM_THREADS);

    /* first step: advance the x direction */
    run_threaded_dg_adi(tasks, thread, g.size_y, g.size_z, g, state, vals, dg_adi_tort_x, g.size_x);

    /* transfer data */
    dg_transfer_data(vals, state, g.size_x, g.size_y * g.size_z, g.size_y * g.size_z);

    /* Adjust memory */
    free(vals);
    vals = malloc(sizeof(AdiLineData) * g.size_x * g.size_z);

    /* second step: advance the y direction */
    run_threaded_dg_adi(tasks, thread, g.size_x, g.size_z, g, state, vals, dg_adi_tort_y, g.size_y);

    /* transfer data */
    dg_transfer_data(vals, state, g.size_y, g.size_x * g.size_z, g.size_z);

    /* Adjust memory */
    free(vals);
    vals = malloc(sizeof(AdiLineData) * g.size_x * g.size_y);

    /* third step: advance the z direction */
    run_threaded_dg_adi(tasks, thread, g.size_x, g.size_y, g, state, vals, dg_adi_tort_z, g.size_z);

    /* transfer data directly into Grid_node values (g.states) */
    dg_transfer_data(vals, g.states, g.size_z, g.size_x * g.size_y, 1);

    free(state);
    free(vals);
    free(thread);
    free(tasks);

    return 0;
}


