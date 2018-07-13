#include <../../nrnconf.h>
#include <stdio.h>
#include <assert.h>
#include "grids.h"
#include "rxd.h"
#include <pthread.h>
#if 0 && defined(__APPLE__)
#include <Python/Python.h>
#else
#include <Python.h>
#endif

/*Tortuous diffusion coefficients*/
#define DcX(x,y,z) (g->dc_x/LAMBDA(x,y,z))
#define DcY(x,y,z) (g->dc_y/LAMBDA(x,y,z))
#define DcZ(x,y,z) (g->dc_z/LAMBDA(x,y,z))

/*Flux in the x,y,z directions*/
#define Fxx(x1,x2) (ALPHA(x1,y,z)*ALPHA(x2,y,z)*g->dc_x*(g->states[IDX(x1,y,z)] - g->states[IDX(x2,y,z)])/((ALPHA(x1,y,z)+ALPHA(x2,y,z))*LAMBDA(x1,y,z)))
#define Fxy(y1,y1d,y2) (ALPHA(x,y1,z)*ALPHA(x,y2,z)*g->dc_y*(g->states[IDX(x,y1,z)] - g->states[IDX(x,y2,z)])/((ALPHA(x,y1,z)+ALPHA(x,y2,z))*LAMBDA(x,y1d,z)))
#define Fxz(z1,z1d,z2) (ALPHA(x,y,z1)*ALPHA(x,y,z2)*g->dc_z*(g->states[IDX(x,y,z1)] - g->states[IDX(x,y,z2)])/((ALPHA(x,y,z1)+ALPHA(x,y,z2))*LAMBDA(x,y,z1d)))
#define Fyy(y1,y2) (ALPHA(x,y1,z)*ALPHA(x,y2,z)*g->dc_y*(g->states[IDX(x,y1,z)] - g->states[IDX(x,y2,z)])/((ALPHA(x,y1,z)+ALPHA(x,y2,z))*LAMBDA(x,y1,z)))
#define Fzz(z1,z2) (ALPHA(x,y,z1)*ALPHA(x,y,z2)*g->dc_z*(g->states[IDX(x,y,z1)] - g->states[IDX(x,y,z2)])/((ALPHA(x,y,z1)+ALPHA(x,y,z2))*LAMBDA(x,y,z1)))

#define FLUXX(a,b,pa,pb) (ALPHA(a,j,k)*ALPHA(b,j,k)*g->dc_x*(pa - pb)/(0.5*(ALPHA(a,j,k)+ALPHA(b,j,k))))
#define FLUXY(a,b,pa,pb) (ALPHA(i,a,k)*ALPHA(i,b,k)*g->dc_y*(pa - pb)/(0.5*(ALPHA(i,a,k)+ALPHA(i,b,k))))
#define FLUXZ(a,b,pa,pb) (ALPHA(i,j,a)*ALPHA(i,j,b)*g->dc_z*(pa - pb)/(0.5*(ALPHA(i,j,a)+ALPHA(i,j,b))))


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
 * scratch  - scratchpad array of doubles, length g.size_x - 1
 * like dg_adi_x except the grid has a variable volume fraction
 * g.alpha and my have variable tortuosity g.lambda
 */
static void dg_adi_vol_x(Grid_node* g, const double dt, const int y, const int z, double const * const state, double* const RHS, double* const scratch)
{
    int yp,ypd,ym,ymd,zp,zpd,zm,zmd,div_y,div_z;
	int x;
	double *diag;
	double *l_diag;
	double *u_diag;
	double prev, next;

    /*TODO: Get rid of this by not calling dg_adi when on the boundary for DIRICHLET conditions*/
    if(g->bc->type == DIRICHLET && (y == 0 || z == 0 || y == g->size_y-1 || z == g->size_z-1))
    {
        for(x=0; x<g->size_x; x++)
            RHS[x] = g->bc->value;
        return;
    }
    /*TODO: move allocation out of loop*/
	diag = malloc(g->size_x*sizeof(double));
	l_diag = malloc((g->size_x-1)*sizeof(double));
	u_diag = malloc((g->size_x-1)*sizeof(double));

	for(x=1;x<g->size_x-1;x++)
	{
		prev = ALPHA(x-1,y,z)*DcX(x,y,z)/(ALPHA(x-1,y,z) + ALPHA(x,y,z));
		next = ALPHA(x+1,y,z)*DcX(x+1,y,z)/(ALPHA(x+1,y,z) + ALPHA(x,y,z));

		l_diag[x-1] = -dt*prev/SQ(g->dx);
		diag[x] = 1. + dt*(prev + next)/SQ(g->dx);
		u_diag[x] = -dt*next/SQ(g->dx);
	}


	yp = (y==g->size_y-1)?y-1:y+1;
	ypd = (y==g->size_y-1)?y:y+1;
	ym = (y==0)?y+1:y-1;
	ymd = (y==0)?1:y;
	zp = (z==g->size_z-1)?z-1:z+1;
	zpd = (z==g->size_z-1)?z:z+1;
	zm = (z==0)?z+1:z-1;
	zmd = (z==0)?1:z;

	div_y = SQ(g->dy)*((y==0||y==g->size_y-1)?1.0:0.5);
	div_z = SQ(g->dz)*((z==0||z==g->size_z-1)?1.0:0.5);

    if(g->bc->type == NEUMANN)
    {
        /*zero flux boundary condition*/
	    next = ALPHA(1,y,z)*DcX(1,y,z)/(ALPHA(1,y,z) + ALPHA(0,y,z));
	    diag[0] = 1. + dt*next/SQ(g->dx);
	    u_diag[0] = -dt*next/SQ(g->dx);

	    prev = ALPHA(g->size_x-2,y,z)*DcX(g->size_x-1,y,z)/
			(ALPHA(g->size_x-1,y,z) + ALPHA(g->size_x-2,y,z));
	    diag[g->size_x-1] = 1. + dt*prev/SQ(g->dx);
	    l_diag[g->size_x-2] = -dt*prev/SQ(g->dx);

	    x=0;
	    RHS[x] = g->states[IDX(x,y,z)] + (dt/ALPHA(x,y,z))*
					((Fxx(x+1,x)/SQ(g->dx)) 
				+    (Fxy(yp,ypd,y) - Fxy(y,ymd,ym))/div_y 
				+ 	 (Fxz(zp,zpd,z) - Fxz(z,zmd,zm))/div_z)
                + dt*g->states_cur[IDX(x,y,z)];;

        x = g->size_x-1;
        RHS[x]  = g->states[IDX(x,y,z)] + (dt/ALPHA(x,y,z))*
				((Fxx(x-1,x))/SQ(g->dx) 
			+    (Fxy(yp,ypd,y) - Fxy(y,ymd,ym))/div_y 
			+ 	 (Fxz(zp,zpd,z) - Fxz(z,zmd,zm))/div_z)
            + dt*g->states_cur[IDX(x,y,z)];
    }
    else
    {
        diag[0] = 1.0;
        u_diag[0] = 0;
        diag[g->size_x-1] = 1.0;
        l_diag[g->size_x-2] = 0;
        RHS[0] = g->bc->value;
        RHS[g->size_x-1] = g->bc->value;
    }

	
    
    //fprintf(stderr,"\t\t %1.20e %1.20e == %1.20e\n", diag[0], u_diag[0], RHS[0]);
	for(x=1;x<g->size_x-1;x++)
	{
        __builtin_prefetch(&(g->states[IDX(x+PREFETCH,y,z)]), 0, 1);
        __builtin_prefetch(&(g->states[IDX(x+PREFETCH,yp,z)]), 0, 0);
        __builtin_prefetch(&(g->states[IDX(x+PREFETCH,ym,z)]), 0, 0);
        __builtin_prefetch(&(g->states[IDX(x+PREFETCH,ypd,z)]), 0, 0);
        __builtin_prefetch(&(g->states[IDX(x+PREFETCH,ymd,z)]), 0, 0);
		RHS[x] =  g->states[IDX(x,y,z)] + (dt/ALPHA(x,y,z))*
				((Fxx(x+1,x) - Fxx(x,x-1))/SQ(g->dx) 
			+    (Fxy(yp,ypd,y) - Fxy(y,ymd,ym))/div_y 
			+ 	 (Fxz(zp,zpd,z) - Fxz(z,zmd,zm))/div_z)
            + dt*g->states_cur[IDX(x,y,z)];
        //fprintf(stderr,"%1.20e %1.20e %1.20e == %1.20e\n", l_diag[x-1], diag[x], u_diag[x], RHS[x]);
	}
    //fprintf(stderr,"%1.20e %1.20e\t\t == %1.20e\n", l_diag[x-1], diag[x], RHS[x]);

	
	solve_dd_tridiag(g->size_x, l_diag, diag, u_diag, RHS, scratch);
	
	free(diag);
	free(l_diag);
	free(u_diag);
}

/* dg_adi_vol_y performs the second of 3 steps in DG-ADI
 * g    -   the parameters and state of the grid
 * dt   -   the time step
 * y    -   the index for the y plane
 * z    -   the index for the z plane
 * state    -   where the output of this step is stored
 * scratch  - scratchpad array of doubles, length g->size_y 
 * like dg_adi_y except the grid has a variable volume fraction
 * g->alpha and my have variable tortuosity g->lambda
 */
static void dg_adi_vol_y(Grid_node* g, double const dt, int const x, int const z, double const * const state, double* const RHS, double* const scratch)
{
	int y;
	double *diag;
	double *l_diag;
	double *u_diag;
	double prev, next;
    
    /*TODO: Get rid of this by not calling dg_adi when on the boundary for DIRICHLET conditions*/
    if(g->bc->type == DIRICHLET && (x == 0 || z == 0 || x == g->size_x-1 || z == g->size_z-1))
    {
        for(y=0; y<g->size_y; y++)
            RHS[y] = g->bc->value;
        return;
    }

	diag = malloc(g->size_y*sizeof(double));
	l_diag = malloc((g->size_y-1)*sizeof(double));
	u_diag = malloc((g->size_y-1)*sizeof(double));

	for(y=1;y<g->size_y-1;y++)
	{
		prev = ALPHA(x,y-1,z)*DcY(x,y,z)/(ALPHA(x,y-1,z) + ALPHA(x,y,z));
		next = ALPHA(x,y+1,z)*DcY(x,y+1,z)/(ALPHA(x,y+1,z) + ALPHA(x,y,z));

		l_diag[y-1] = -dt*prev/SQ(g->dy);
		diag[y] = 1. + dt*(prev + next)/SQ(g->dy);
		u_diag[y] = -dt*next/SQ(g->dy);
	}


    if(g->bc->type == NEUMANN)
    {
	    /*zero flux boundary condition*/
	    next = ALPHA(x,1,z)*DcY(x,1,z)/(ALPHA(x,1,z) + ALPHA(x,0,z));
	    diag[0] = 1. + dt*next/SQ(g->dy);
	    u_diag[0] = -dt*next/SQ(g->dy);

	    prev = ALPHA(x,g->size_y-2,z)*DcY(x,g->size_y-1,z)/
			(ALPHA(x,g->size_y-1,z) + ALPHA(x,g->size_y-2,z));

	    diag[g->size_y-1] = 1. + dt*prev/SQ(g->dy);
	    l_diag[g->size_y-2] = -dt*prev/SQ(g->dy);

	    RHS[0] =  state[x + z*g->size_x] - dt*Fyy(1,0)/(SQ(g->dy)*ALPHA(x,0,z));
        y = g->size_y-1;
        RHS[y] =  state[x + (z + y*g->size_z)*g->size_x] + (dt/ALPHA(x,y,z))*Fyy(y,y-1)/SQ(g->dy);
    }
    else
    {
        diag[0] = 1.0;
        u_diag[0] = 0;
        diag[g->size_y-1] = 1.0;
        l_diag[g->size_y-2] = 0;
        RHS[0] = g->bc->value;
        RHS[g->size_y-1] = g->bc->value;

    }
	for(y=1;y<g->size_y-1;y++)
	{
        __builtin_prefetch(&state[x + (z + (y+PREFETCH)*g->size_z)*g->size_x], 0, 0);
        __builtin_prefetch(&(g->states[IDX(x,y+PREFETCH,z)]), 0, 1);
		RHS[y] =  state[x + (z + y*g->size_z)*g->size_x]
            - (dt/ALPHA(x,y,z))*(Fyy(y+1,y) - Fyy(y,y-1))/SQ(g->dy);
	}

	solve_dd_tridiag(g->size_y, l_diag, diag, u_diag, RHS, scratch);

	free(diag);
	free(l_diag);
	free(u_diag);

}

/* dg_adi_vol_z performs the third of 3 steps in DG-ADI
 * g    -   the parameters and state of the grid
 * dt   -   the time step
 * y    -   the index for the y plane
 * z    -   the index for the z plane
 * state    -   where the output of this step is stored
 * scratch  - scratchpad array of doubles, length g->size_y
 * like dg_adi_z except the grid has a variable volume fraction
 * g->alpha and my have variable tortuosity g->lambda
 */
static void dg_adi_vol_z(Grid_node* g, double const dt, int const x, int const y, double const * const state, double* const RHS, double* const scratch)
{
	int z;
	double *diag;
	double *l_diag;
	double *u_diag;
	double prev, next;

    /*TODO: Get rid of this by not calling dg_adi when on the boundary for DIRICHLET conditions*/
    if(g->bc->type == DIRICHLET && (x == 0 || y == 0 || x == g->size_x-1 || y == g->size_y-1))
    {
        for(z=0; z<g->size_z; z++)
            RHS[z] = g->bc->value;
        return;
    }

	diag = malloc(g->size_z*sizeof(double));
	l_diag = malloc((g->size_z-1)*sizeof(double));
	u_diag = malloc((g->size_z-1)*sizeof(double));

	for(z=1;z<g->size_z-1;z++)
	{
		prev = ALPHA(x,y,z-1)*DcZ(x,y,z)/(ALPHA(x,y,z-1) + ALPHA(x,y,z));
		next = ALPHA(x,y,z+1)*DcZ(x,y,z+1)/(ALPHA(x,y,z+1) + ALPHA(x,y,z));

		l_diag[z-1] = -dt*prev/SQ(g->dz);
		diag[z] = 1. + dt*(prev + next)/SQ(g->dz);
		u_diag[z] = -dt*next/SQ(g->dz);
	}

    if(g->bc->type == NEUMANN)
    { 
	    /*zero flux boundary condition*/
    	next = ALPHA(x,y,1)*DcZ(x,y,1)/(ALPHA(x,y,1) + ALPHA(x,y,0));
    	diag[0] = 1. + dt*next/SQ(g->dz);
    	u_diag[0] = -dt*next/SQ(g->dz);

    	prev = ALPHA(x,y,g->size_z-2)*DcZ(x,y,g->size_z-1)/
			(ALPHA(x,y,g->size_z-1) + ALPHA(x,y,g->size_z-2));

    	diag[g->size_z-1] = 1. + dt*prev/SQ(g->dz);
    	l_diag[g->size_z-2] = -dt*prev/SQ(g->dz);

        RHS[0] = state[y + g->size_y*(x*g->size_z)] - dt*Fzz(1,0)/(SQ(g->dz)*ALPHA(x,y,0));
        z = g->size_z-1;
        RHS[z] =  state[y + g->size_y*(x*g->size_z + z)] + (dt/ALPHA(x,y,z))*Fzz(z,z-1)/SQ(g->dz);
    }
    else
    {
        diag[0] = 1.0;
        u_diag[0] = 0;
        diag[g->size_z-1] = 1.0;
        l_diag[g->size_z-2] = 0;
        RHS[0] = g->bc->value;
        RHS[g->size_z-1] = g->bc->value;

    }
	for(z=1;z<g->size_z-1;z++)
	{
		RHS[z] =  state[y + g->size_y*(x*g->size_z + z)]
                - (dt/ALPHA(x,y,z))*(Fzz(z+1,z) - Fzz(z,z-1))/SQ(g->dz);
	}

	solve_dd_tridiag(g->size_z, l_diag, diag, u_diag, RHS, scratch);

	free(diag);
	free(l_diag);
	free(u_diag);

}

/*DG-ADI implementation the 3 step process to diffusion species in grid g by time step *dt_ptr
 * g    -   the state and parameters
 * like dg_adi execpt the Grid_node g has variable volume fraction g.alpha and may have
 * variable tortuosity g.lambda
 */
void set_adi_vol(Grid_node *g)
{
    g->adi_dir_x->dg_adi_dir = dg_adi_vol_x;
    g->adi_dir_y->dg_adi_dir = dg_adi_vol_y;
    g->adi_dir_z->dg_adi_dir = dg_adi_vol_z;    
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
static void dg_adi_tort_x(Grid_node* g, const double dt, const int y, const int z, double const * const state, double* const RHS, double* const scratch)
{
	int yp,ypd,ym,ymd,zp,zpd,zm,zmd,div_y,div_z;
	int x;
	double *diag;
	double *l_diag;
	double *u_diag;

    if(g->bc->type == DIRICHLET && (y == 0 || z == 0 || y == g->size_y-1 || z == g->size_z-1))
    {
        for(x=0; x<g->size_x; x++)
            RHS[x] = g->bc->value;
        return;
    }

	diag = malloc(g->size_x*sizeof(double));
	l_diag = malloc((g->size_x-1)*sizeof(double));
	u_diag = malloc((g->size_x-1)*sizeof(double));

	for(x=1;x<g->size_x-1;x++)
	{
		l_diag[x-1] = -dt*DcX(x,y,z)/(2.*SQ(g->dx));
		diag[x] = 1. + dt*(DcX(x,y,z) + DcX(x+1,y,z))/(2.*SQ(g->dx));
		u_diag[x] = -dt*DcX(x+1,y,z)/(2.*SQ(g->dx));
	}

	/*zero flux boundary condition*/
	yp = (y==g->size_y-1)?y-1:y+1;
	ypd = (y==g->size_y-1)?y:y+1;
	ym = (y==0)?y+1:y-1;
	ymd = (y==0)?1:y;
	zp = (z==g->size_z-1)?z-1:z+1;
	zpd = (z==g->size_z-1)?z:z+1;
	zm = (z==0)?z+1:z-1;
	zmd = (z==0)?1:z;

	div_y = (y==0||y==g->size_y-1)?2.:1.;
	div_z = (z==0||z==g->size_z-1)?2.:1.;


    if(g->bc->type == NEUMANN)
    { 
	    diag[0] = 1. + 0.5*dt*DcX(1,y,z)/SQ(g->dx);
	    u_diag[0] = -0.5*dt*DcX(1,y,z)/SQ(g->dx);
	    diag[g->size_x-1] = 1. + 0.5*dt*DcX(g->size_x-1,y,z)/SQ(g->dx);
	    l_diag[g->size_x-2] = -0.5*dt*DcX(g->size_x-1,y,z)/SQ(g->dx);
        

	    RHS[0] =  g->states[IDX(0,y,z)]
			    + dt*((DcX(1,y,z)*g->states[IDX(1,y,z)] - DcX(1,y,z)*g->states[IDX(0,y,z)])/(2.*SQ(g->dx))
			    +	  (DcY(0,ypd,z)*g->states[IDX(0,yp,z)] - (DcY(0,ypd,z)+DcY(0,ymd,z))*g->states[IDX(0,y,z)] + DcY(0,ymd,z)*g->states[IDX(0,ym,z)])/(div_y*SQ(g->dy))
			    +	  (DcZ(0,y,zpd)*g->states[IDX(0,y,zp)] - (DcZ(0,y,zpd)+DcZ(0,y,zmd))*g->states[IDX(0,y,z)] + DcZ(0,y,zmd)*g->states[IDX(0,y,zm)])/(div_z*SQ(g->dz)))
                + dt*g->states_cur[IDX(0,y,z)];
        x = g->size_x-1;
        RHS[x] =  g->states[IDX(x,y,z)]
				+ dt*((DcX(x,y,z)*g->states[IDX(x-1,y,z)] - DcX(x,y,z)*g->states[IDX(x,y,z)])/(2.*SQ(g->dx))
				+	  (DcY(x,ypd,z)*g->states[IDX(x,yp,z)] - (DcY(x,ypd,z)+DcY(x,ymd,z))*g->states[IDX(x,y,z)] + DcY(x,ymd,z)*g->states[IDX(x,ym,z)])/(div_y*SQ(g->dy))
				+	  (DcZ(x,y,zpd)*g->states[IDX(x,y,zp)] - (DcZ(x,y,zpd)+DcZ(x,y,zmd))*g->states[IDX(x,y,z)] + DcZ(x,y,zmd)*g->states[IDX(x,y,zm)])/(div_z*SQ(g->dz)))
                + dt*g->states_cur[IDX(x,y,z)];

    }
    else
    {
        diag[0] = 1.0;
	    u_diag[0] = 0.0;
	    diag[g->size_x-1] = 1.0;
	    l_diag[g->size_x-2] = 0.0;        
        RHS[0] = g->bc->value;
        RHS[g->size_x-1] = g->bc->value;
    }

	for(x=1;x<g->size_x-1;x++)
	{
        __builtin_prefetch(&(g->states[IDX(x+PREFETCH,y,z)]), 0, 1);
        __builtin_prefetch(&(g->states[IDX(x+PREFETCH,yp,z)]), 0, 0);
        __builtin_prefetch(&(g->states[IDX(x+PREFETCH,ym,z)]), 0, 0);
		RHS[x] =  g->states[IDX(x,y,z)]
			+ dt*((DcX(x+1,y,z)*g->states[IDX(x+1,y,z)] - (DcX(x+1,y,z)+DcX(x,y,z))*g->states[IDX(x,y,z)] + DcX(x,y,z)*g->states[IDX(x-1,y,z)])/(2.*SQ(g->dx))
			+	  (DcY(x,ypd,z)*g->states[IDX(x,yp,z)] - (DcY(x,ypd,z)+DcY(x,ymd,z))*g->states[IDX(x,y,z)] + DcY(x,ymd,z)*g->states[IDX(x,ym,z)])/(div_y*SQ(g->dy))
			+	  (DcZ(x,y,zpd)*g->states[IDX(x,y,zp)] - (DcZ(x,y,zpd)+DcZ(x,y,zmd))*g->states[IDX(x,y,z)] + DcZ(x,y,zmd)*g->states[IDX(x,y,zm)])/(div_z*SQ(g->dz)))
            + dt*g->states_cur[IDX(x,y,z)];
	}
	
	solve_dd_tridiag(g->size_x, l_diag, diag, u_diag, RHS, scratch);
	
	free(diag);
	free(l_diag);
	free(u_diag);
}

/* dg_adi_tort_y performs the second of 3 steps in DG-ADI
 * g    -   the parameters and state of the grid
 * dt   -   the time step
 * y    -   the index for the y plane
 * z    -   the index for the z plane
 * state    -   where the output of this step is stored
 * scratch  - scratchpad array of doubles, length g->size_y - 1
 * like dg_adi_y except the grid has a variable tortuosity
 * g->lambda (but it still has fixed volume fraction)
 */
static void dg_adi_tort_y(Grid_node* g, double const dt, int const x, int const z, double const * const state, double* const RHS, double* const scratch)
{
	int y;
	double *diag;
	double *l_diag;
	double *u_diag;

    /*TODO: Get rid of this by not calling dg_adi when on the boundary for DIRICHLET conditions*/
    if(g->bc->type == DIRICHLET && (x == 0 || z == 0 || x == g->size_x-1 || z == g->size_z-1))
    {
        for(y=0; y<g->size_y; y++)
            RHS[y] = g->bc->value;
        return;
    }

	diag = malloc(g->size_y*sizeof(double));
	l_diag = malloc((g->size_y-1)*sizeof(double));
	u_diag = malloc((g->size_y-1)*sizeof(double));

	for(y=1;y<g->size_y-1;y++)
	{
		l_diag[y-1] = -dt*DcY(x,y,z)/(2.*SQ(g->dy));
		diag[y] = 1. + dt*(DcY(x,y,z)+DcY(x,y+1,z))/(2.*SQ(g->dy));
		u_diag[y] = -dt*DcY(x,y+1,z)/(2.*SQ(g->dy));
	}

    if(g->bc->type == NEUMANN)
    {   
	    /*zero flux boundary condition*/
	    diag[0] = 1. + 0.5*dt*DcY(x,1,z)/SQ(g->dy);
	    u_diag[0] = -0.5*dt*DcY(x,1,z)/SQ(g->dy);
	    diag[g->size_y-1] = 1. + 0.5*dt*DcY(x,g->size_y-1,z)/SQ(g->dy);
	    l_diag[g->size_y-2] = -0.5*dt*DcY(x,g->size_y-1,z)/SQ(g->dy);



	    RHS[0] =  state[x + z*g->size_x] - dt*((DcY(x,1,z)*g->states[IDX(x,1,z)] -   DcY(x,1,z)*g->states[IDX(x,0,z)])/(2.*SQ(g->dy)));
        y = g->size_y-1;
        RHS[y] =  state[x + (z + y*g->size_z)*g->size_x]	
		-	  dt*(DcY(x,y,z)*g->states[IDX(x,y-1,z)] - DcY(x,y,z)*g->states[IDX(x,y,z)])/(2.*SQ(g->dy));
    }
    else
    {
        diag[0] = 1.0;
	    u_diag[0] = 0.0;
	    diag[g->size_y-1] = 1.0;
	    l_diag[g->size_y-2] = 0.0;        
        RHS[0] = g->bc->value;
        RHS[g->size_y-1] = g->bc->value;
    }


	for(y=1;y<g->size_y-1;y++)
	{
        __builtin_prefetch(&state[x + (z + (y+PREFETCH)*g->size_z)*g->size_x], 0, 0);
        __builtin_prefetch(&(g->states[IDX(x,y+PREFETCH,z)]), 0, 1);
        RHS[y] =  state[x + (z + y*g->size_z)*g->size_x]			-	  dt*(DcY(x,y+1,z)*g->states[IDX(x,y+1,z)] - (DcY(x,y+1,z)+DcY(x,y,z))*g->states[IDX(x,y,z)] + DcY(x,y,z)*g->states[IDX(x,y-1,z)])/(2.*SQ(g->dy));

	}

	solve_dd_tridiag(g->size_y, l_diag, diag, u_diag, RHS, scratch);

	free(diag);
	free(l_diag);
	free(u_diag);

}

/* dg_adi_tort_z performs the second of 3 steps in DG-ADI
 * g    -   the parameters and state of the grid
 * dt   -   the time step
 * y    -   the index for the y plane
 * z    -   the index for the z plane
 * state    -   where the output of this step is stored
 * scratch  - scratchpad array of doubles, length g->size_z - 1
 * like dg_adi_z except the grid has a variable tortuosity
 * g->lambda (but it still has fixed volume fraction)
 */
static void dg_adi_tort_z(Grid_node* g, double const dt, int const x, int const y, double const * const state, double* const RHS, double* const scratch)
{
	int z;
	double *diag;
	double *l_diag;
	double *u_diag;

    /*TODO: Get rid of this by not calling dg_adi when on the boundary for DIRICHLET conditions*/
    if(g->bc->type == DIRICHLET && (x == 0 || y == 0 || x == g->size_x-1 || y == g->size_y-1))
    {
        for(z=0; z<g->size_z; z++)
            RHS[z] = g->bc->value;
        return;
    }

	diag = malloc(g->size_z*sizeof(double));
	l_diag = malloc((g->size_z-1)*sizeof(double));
	u_diag = malloc((g->size_z-1)*sizeof(double));


	for(z=1;z<g->size_z-1;z++)
	{
		l_diag[z-1] = -dt*DcZ(x,y,z)/(2.*SQ(g->dz));
		diag[z] = 1. + dt*(DcZ(x,y,z)+DcZ(x,y,z+1))/(2.*SQ(g->dz));
		u_diag[z] = -dt*DcZ(x,y,z+1)/(2.*SQ(g->dz));
	}

    if(g->bc->type == NEUMANN)
    { 
	/*zero flux boundary condition*/
	    diag[0] = 1. + 0.5*dt*DcZ(x,y,1)/SQ(g->dz);
	    u_diag[0] = -0.5*dt*DcZ(x,y,1)/SQ(g->dz);
	    diag[g->size_z-1] = 1. + 0.5*dt*DcZ(x,y,g->size_z-1)/SQ(g->dz);
	    l_diag[g->size_z-2] = -0.5*dt*DcZ(x,y,g->size_z-1)/SQ(g->dz);

	    RHS[0] =  state[y + g->size_y*(x*g->size_z)] - dt*((DcZ(x,y,1)*g->states[IDX(x,y,1)] - DcZ(x,y,1)*g->states[IDX(x,y,0)])/(2.*SQ(g->dz)));
        z = g->size_z-1;
        RHS[z] =  state[y + g->size_y*(x*g->size_z + z)]
				-	  dt*(DcZ(x,y,z)*g->states[IDX(x,y,z-1)] - DcZ(x,y,z)*g->states[IDX(x,y,z)])/(2.*SQ(g->dz));
    }
    else
    {
        diag[0] = 1.0;
	    u_diag[0] = 0.0;
	    diag[g->size_z-1] = 1.0;
	    l_diag[g->size_z-2] = 0.0; 
        RHS[0] = g->bc->value;
        RHS[g->size_z-1] = g->bc->value;
    }



	for(z=1;z<g->size_z-1;z++)
	{
		RHS[z] =  state[y + g->size_y*(x*g->size_z + z)]
			-	  dt*(DcZ(x,y,z+1)*g->states[IDX(x,y,z+1)] - (DcZ(x,y,z+1)+DcZ(x,y,z))*g->states[IDX(x,y,z)] + DcZ(x,y,z)*g->states[IDX(x,y,z-1)])/(2.*SQ(g->dz));

	}

	solve_dd_tridiag(g->size_z, l_diag, diag, u_diag, RHS, scratch);

	free(diag);
	free(l_diag);
	free(u_diag);
}

/*DG-ADI implementation the 3 step process to diffusion species in grid g by time step *dt_ptr
 * g    -   the state and parameters
 * like dg_adi except the grid node g has variable tortuosity g.lambda (but fixed volume
 * fraction)
 */
void set_adi_tort(Grid_node *g)
{
    g->adi_dir_x->dg_adi_dir = dg_adi_tort_x;
    g->adi_dir_y->dg_adi_dir = dg_adi_tort_y;
    g->adi_dir_z->dg_adi_dir = dg_adi_tort_z;    
}


/*****************************************************************************
*
* Begin variable step code
*
*****************************************************************************/

void _rhs_variable_step_helper_tort(Grid_node* g, double const * const states, double* ydot) {
    int num_states_x = g->size_x, num_states_y = g->size_y, num_states_z = g->size_z;
    double dx = g->dx, dy = g->dy, dz = g->dz;
    int i, j, k, stop_i, stop_j, stop_k;
    double rate_x = 1. / (dx * dx);
    double rate_y = 1. / (dy * dy);
    double rate_z = 1. / (dz * dz);

	/*indices*/
	int index, prev_i, prev_j, prev_k, next_i ,next_j, next_k;
	int xp, xpd, xm, xmd, yp, ypd, ym, ymd, zp, zpd, zm, zmd;
	double div_x, div_y, div_z;

	/* Euler advance x, y, z (all points) */
    stop_i = num_states_x - 1;
    stop_j = num_states_y - 1;
    stop_k = num_states_z - 1;

    for (i = 0; i <= stop_i; i++) {
			/*Zero flux boundary conditions*/
			xp = (i==stop_i)?i-1:i+1;
			xpd = (i==stop_i)?i:i+1;
			xm = (i==0)?i+1:i-1;
			xmd = (i==0)?1:i;
			div_x = (i==0||i==stop_i)?2.:1.;

        for(j = 0; j <= stop_j; j++) {
			yp = (j==stop_j)?j-1:j+1;
			ypd = (j==stop_j)?j:j+1;
			ym = (j==0)?j+1:j-1;
			ymd = (j==0)?1:j;
			div_y = (j==0||j==stop_j)?2.:1.;
            
			for(k = 0; k <= stop_k; k++) {
				zp = (k==stop_k)?k-1:k+1;
				zpd = (k==stop_k)?k:k+1;
				zm = (k==0)?k+1:k-1;
				zmd = (k==0)?1:k;
				div_z = (k==0||k==stop_k)?2.:1.;

                index = find(i, j, k, num_states_y, num_states_z);
                prev_i = find(xm, j, k, num_states_y, num_states_z);
                prev_j = find(i, ym, k, num_states_y, num_states_z);
                prev_k = find(i, j, zm, num_states_y, num_states_z);
                next_i = find(xp, j, k, num_states_y, num_states_z);
                next_j = find(i, yp, k, num_states_y, num_states_z);
                next_k = find(i, j, zp, num_states_y, num_states_z);
			
                ydot[index] += rate_x * (DcX(xmd,j,k)*states[prev_i] -  
                    (DcX(xmd,j,k)+DcX(xpd,j,k)) * states[index] + 
					DcX(xpd,j,k)*states[next_i])/div_x;

                ydot[index] += rate_y * (DcY(i,ymd,k)*states[prev_j] - 
                    (DcY(i,ymd,k)+DcY(i,ypd,k)) * states[index] +
					DcY(i,ypd,k)*states[next_j])/div_y;

                ydot[index] += rate_z * (DcZ(i,j,zmd)*states[prev_k] - 
                    (DcZ(i,j,zmd)+DcZ(i,j,zpd)) * states[index] + 
					DcZ(i,j,zpd)*states[next_k])/div_z;

            }   
        }
    }
}


void _rhs_variable_step_helper_vol(Grid_node* g, double const * const states, double* ydot) {
    int num_states_x = g->size_x, num_states_y = g->size_y, num_states_z = g->size_z;
    double dx = g->dx, dy = g->dy, dz = g->dz;
    int i, j, k, stop_i, stop_j, stop_k;
    double rate_x = g->dc_x / (dx * dx);
    double rate_y = g->dc_y / (dy * dy);
    double rate_z = g->dc_z / (dz * dz);


	/*indices*/
	int index, prev_i, prev_j, prev_k, next_i ,next_j, next_k;

	/*zero flux boundary conditions*/
	int xp, xpd, xm, xmd, yp, ypd, ym, ymd, zp, zpd, zm, zmd;
	double div_x, div_y, div_z;
	
	/* Euler advance x, y, z (all points) */
    stop_i = num_states_x - 1;
    stop_j = num_states_y - 1;
    stop_k = num_states_z - 1;
    for (i = 0; i <= stop_i; i++) {
			/*Zero flux boundary conditions*/
			xp = (i==stop_i)?stop_i-1:i+1;
			xpd = (i==stop_i)?i:i+1;
			xm = (i==0)?i+1:i-1;
			xmd = (i==0)?1:i;
			div_x = (i==0||i==stop_i)?2.:1.;

        for(j = 0; j <= stop_j; j++) {
			yp = (j==stop_j)?j-1:j+1;
			ypd = (j==stop_j)?j:j+1;
			ym = (j==0)?j+1:j-1;
			ymd = (j==0)?1:j;
			div_y = (j==0||j==stop_j)?2.:1.;
            
			for(k = 0; k <= stop_k; k++) {
				zp = (k==stop_k)?k-1:k+1;
				zpd = (k==stop_k)?k:k+1;
				zm = (k==0)?k+1:k-1;
				zmd = (k==0)?1:k;
				div_z = (k==0||k==stop_k)?2.:1.;
				
                index = find(i, j, k, num_states_y, num_states_z);
                prev_i = find(xm, j, k, num_states_y, num_states_z);
                prev_j = find(i, ym, k, num_states_y, num_states_z);
                prev_k = find(i, j, zm, num_states_y, num_states_z);
                next_i = find(xp, j, k, num_states_y, num_states_z);
                next_j = find(i, yp, k, num_states_y, num_states_z);
                next_k = find(i, j, zp, num_states_y, num_states_z);
				
				/*x-direction*/
				ydot[index] += rate_x * (FLUXX(xp,i,states[next_i],states[index])/LAMBDA(xpd,j,k) - 
					FLUXX(xm,i,states[index],states[prev_i])/LAMBDA(xmd,j,k))/(ALPHA(i,j,k)*div_x);

				/*y-direction*/
				ydot[index] += rate_y * (FLUXY(yp,j,states[next_j],states[index])/LAMBDA(i,ypd,k) - 
					FLUXY(ym,j,states[index],states[prev_j])/LAMBDA(i,ymd,k))/(ALPHA(i,j,k)*div_y);

				/*z-direction*/
				ydot[index] += rate_z * (FLUXZ(zp,k,states[next_k],states[index])/LAMBDA(i,j,zpd) - 
					FLUXZ(zm,k,states[index],states[prev_k])/LAMBDA(i,j,zmd))/(ALPHA(i,j,k)*div_z);
				
            }   
        }
    }
}


