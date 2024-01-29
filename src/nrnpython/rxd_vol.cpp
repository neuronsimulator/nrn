#include <../../nrnconf.h>
#include <stdio.h>
#include <assert.h>
#include "grids.h"
#include "rxd.h"
#include <nrnwrap_Python.h>

/*Tortuous diffusion coefficients*/
#define DcX(x, y, z) (g->dc_x * PERM(x, y, z))
#define DcY(x, y, z) (g->dc_y * PERM(x, y, z))
#define DcZ(x, y, z) (g->dc_z * PERM(x, y, z))

/*Flux in the x,y,z directions*/
// TODO: Refactor to avoid calculating indices
#define Fxx(x1, x2)                                          \
    (ALPHA(x1, y, z) * ALPHA(x2, y, z) * DcX(x1, y, z) *     \
     (g->states[IDX(x1, y, z)] - g->states[IDX(x2, y, z)]) / \
     ((ALPHA(x1, y, z) + ALPHA(x2, y, z))))
#define Fxy(y1, y1d, y2)                                     \
    (ALPHA(x, y1, z) * ALPHA(x, y2, z) * DcY(x, y1d, z) *    \
     (g->states[IDX(x, y1, z)] - g->states[IDX(x, y2, z)]) / \
     ((ALPHA(x, y1, z) + ALPHA(x, y2, z))))
#define Fxz(z1, z1d, z2)                                     \
    (ALPHA(x, y, z1) * ALPHA(x, y, z2) * DcZ(x, y, z1d) *    \
     (g->states[IDX(x, y, z1)] - g->states[IDX(x, y, z2)]) / \
     ((ALPHA(x, y, z1) + ALPHA(x, y, z2))))
#define Fyy(y1, y2)                                          \
    (ALPHA(x, y1, z) * ALPHA(x, y2, z) * DcY(x, y1, z) *     \
     (g->states[IDX(x, y1, z)] - g->states[IDX(x, y2, z)]) / \
     ((ALPHA(x, y1, z) + ALPHA(x, y2, z))))
#define Fzz(z1, z2)                                          \
    (ALPHA(x, y, z1) * ALPHA(x, y, z2) * DcZ(x, y, z1) *     \
     (g->states[IDX(x, y, z1)] - g->states[IDX(x, y, z2)]) / \
     ((ALPHA(x, y, z1) + ALPHA(x, y, z2))))

/*Flux for used by variable step inhomogeneous volume fraction*/
#define FLUX(pidx, idx)                                             \
    (VOLFRAC(pidx) * VOLFRAC(idx) * (states[pidx] - states[idx])) / \
        (0.5 * (VOLFRAC(pidx) + VOLFRAC(idx)))

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
static int solve_dd_tridiag(int N,
                            const double* l_diag,
                            const double* diag,
                            const double* u_diag,
                            double* b,
                            double* c) {
    int i;

    c[0] = u_diag[0] / diag[0];
    b[0] = b[0] / diag[0];

    for (i = 1; i < N - 1; i++) {
        c[i] = u_diag[i] / (diag[i] - l_diag[i - 1] * c[i - 1]);
        b[i] = (b[i] - l_diag[i - 1] * b[i - 1]) / (diag[i] - l_diag[i - 1] * c[i - 1]);
    }
    b[N - 1] = (b[N - 1] - l_diag[N - 2] * b[N - 2]) / (diag[N - 1] - l_diag[N - 2] * c[N - 2]);


    /*back substitution*/
    for (i = N - 2; i >= 0; i--) {
        b[i] = b[i] - c[i] * b[i + 1];
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
static void ecs_dg_adi_vol_x(ECS_Grid_node* g,
                             const double dt,
                             const int y,
                             const int z,
                             double const* const state,
                             double* const RHS,
                             double* const scratch) {
    int yp, ypd, ym, ymd, zp, zpd, zm, zmd;
    int x;
    double* diag;
    double* l_diag;
    double* u_diag;
    double prev, next, div_y, div_z;

    /*TODO: Get rid of this by not calling dg_adi when on the boundary for DIRICHLET conditions*/
    if (g->bc->type == DIRICHLET &&
        (y == 0 || z == 0 || y == g->size_y - 1 || z == g->size_z - 1)) {
        for (x = 0; x < g->size_x; x++)
            RHS[x] = g->bc->value;
        return;
    }

    /*zero flux boundary condition*/
    div_y = ((y == 0 || y == g->size_y - 1) ? 1.0 : 0.5) * SQ(g->dy);
    div_z = ((z == 0 || z == g->size_z - 1) ? 1.0 : 0.5) * SQ(g->dz);
    if (g->size_y == 1) {
        yp = 0;
        ypd = 0;
        ym = 0;
        ymd = 0;
    } else {
        yp = (y == g->size_y - 1) ? y - 1 : y + 1;
        ypd = (y == g->size_y - 1) ? y : y + 1;
        ym = (y == 0) ? y + 1 : y - 1;
        ymd = (y == 0) ? 1 : y;
    }
    if (g->size_z == 1) {
        zp = 0;
        zpd = 0;
        zm = 0;
        zmd = 0;
    } else {
        zp = (z == g->size_z - 1) ? z - 1 : z + 1;
        zpd = (z == g->size_z - 1) ? z : z + 1;
        zm = (z == 0) ? z + 1 : z - 1;
        zmd = (z == 0) ? 1 : z;
    }

    if (g->size_x == 1) {
        if (g->bc->type == DIRICHLET) {
            RHS[0] = g->bc->value;
        } else {
            x = 0;
            RHS[0] = 0;
            if (g->size_y > 1)
                RHS[0] += (Fxy(yp, ypd, y) - Fxy(y, ymd, ym)) / div_y;
            if (g->size_z > 1)
                RHS[0] += (Fxz(zp, zpd, z) - Fxz(z, zmd, zm)) / div_z;
            RHS[0] *= (dt / ALPHA(0, y, z));
            RHS[0] += state[IDX(0, y, z)] + g->states_cur[IDX(0, y, z)];
        }
        return;
    }

    /*TODO: move allocation out of loop*/
    diag = (double*) malloc(g->size_x * sizeof(double));
    l_diag = (double*) malloc((g->size_x - 1) * sizeof(double));
    u_diag = (double*) malloc((g->size_x - 1) * sizeof(double));

    for (x = 1; x < g->size_x - 1; x++) {
        prev = ALPHA(x - 1, y, z) * DcX(x, y, z) / (ALPHA(x - 1, y, z) + ALPHA(x, y, z));
        next = ALPHA(x + 1, y, z) * DcX(x + 1, y, z) / (ALPHA(x + 1, y, z) + ALPHA(x, y, z));

        l_diag[x - 1] = -dt * prev / SQ(g->dx);
        diag[x] = 1. + dt * (prev + next) / SQ(g->dx);
        u_diag[x] = -dt * next / SQ(g->dx);
    }


    if (g->bc->type == NEUMANN) {
        /*zero flux boundary condition*/
        next = ALPHA(1, y, z) * DcX(1, y, z) / (ALPHA(1, y, z) + ALPHA(0, y, z));
        diag[0] = 1. + dt * next / SQ(g->dx);
        u_diag[0] = -dt * next / SQ(g->dx);

        prev = ALPHA(g->size_x - 2, y, z) * DcX(g->size_x - 1, y, z) /
               (ALPHA(g->size_x - 1, y, z) + ALPHA(g->size_x - 2, y, z));
        diag[g->size_x - 1] = 1. + dt * prev / SQ(g->dx);
        l_diag[g->size_x - 2] = -dt * prev / SQ(g->dx);

        x = 0;
        RHS[x] = state[IDX(x, y, z)] +
                 (dt / ALPHA(x, y, z)) *
                     ((Fxx(x + 1, x) / SQ(g->dx)) + (Fxy(yp, ypd, y) - Fxy(y, ymd, ym)) / div_y +
                      (Fxz(zp, zpd, z) - Fxz(z, zmd, zm)) / div_z) +
                 g->states_cur[IDX(x, y, z)];

        x = g->size_x - 1;
        RHS[x] = state[IDX(x, y, z)] +
                 (dt / ALPHA(x, y, z)) *
                     ((Fxx(x - 1, x)) / SQ(g->dx) + (Fxy(yp, ypd, y) - Fxy(y, ymd, ym)) / div_y +
                      (Fxz(zp, zpd, z) - Fxz(z, zmd, zm)) / div_z) +
                 g->states_cur[IDX(x, y, z)];
    } else {
        diag[0] = 1.0;
        u_diag[0] = 0;
        diag[g->size_x - 1] = 1.0;
        l_diag[g->size_x - 2] = 0;
        RHS[0] = g->bc->value;
        RHS[g->size_x - 1] = g->bc->value;
    }

    for (x = 1; x < g->size_x - 1; x++) {
#ifndef __PGI
        __builtin_prefetch(&(state[IDX(x + PREFETCH, y, z)]), 0, 1);
        __builtin_prefetch(&(state[IDX(x + PREFETCH, yp, z)]), 0, 0);
        __builtin_prefetch(&(state[IDX(x + PREFETCH, ym, z)]), 0, 0);
        __builtin_prefetch(&(state[IDX(x + PREFETCH, ypd, z)]), 0, 0);
        __builtin_prefetch(&(state[IDX(x + PREFETCH, ymd, z)]), 0, 0);
#endif
        RHS[x] = state[IDX(x, y, z)] +
                 (dt / ALPHA(x, y, z)) * ((Fxx(x + 1, x) - Fxx(x, x - 1)) / SQ(g->dx) +
                                          (Fxy(yp, ypd, y) - Fxy(y, ymd, ym)) / div_y +
                                          (Fxz(zp, zpd, z) - Fxz(z, zmd, zm)) / div_z) +
                 g->states_cur[IDX(x, y, z)];
    }

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
static void ecs_dg_adi_vol_y(ECS_Grid_node* g,
                             double const dt,
                             int const x,
                             int const z,
                             double const* const state,
                             double* const RHS,
                             double* const scratch) {
    int y;
    double* diag;
    double* l_diag;
    double* u_diag;
    double prev, next;

    /*TODO: Get rid of this by not calling dg_adi when on the boundary for DIRICHLET conditions*/
    if (g->bc->type == DIRICHLET &&
        (x == 0 || z == 0 || x == g->size_x - 1 || z == g->size_z - 1)) {
        for (y = 0; y < g->size_y; y++)
            RHS[y] = g->bc->value;
        return;
    }

    if (g->size_y == 1) {
        if (g->bc->type == DIRICHLET) {
            RHS[0] = g->bc->value;
        } else {
            RHS[0] = state[x + z * g->size_x];
        }
        return;
    }

    diag = (double*) malloc(g->size_y * sizeof(double));
    l_diag = (double*) malloc((g->size_y - 1) * sizeof(double));
    u_diag = (double*) malloc((g->size_y - 1) * sizeof(double));

    for (y = 1; y < g->size_y - 1; y++) {
        prev = ALPHA(x, y - 1, z) * DcY(x, y, z) / (ALPHA(x, y - 1, z) + ALPHA(x, y, z));
        next = ALPHA(x, y + 1, z) * DcY(x, y + 1, z) / (ALPHA(x, y + 1, z) + ALPHA(x, y, z));

        l_diag[y - 1] = -dt * prev / SQ(g->dy);
        diag[y] = 1. + dt * (prev + next) / SQ(g->dy);
        u_diag[y] = -dt * next / SQ(g->dy);
    }


    if (g->bc->type == NEUMANN) {
        /*zero flux boundary condition*/
        next = ALPHA(x, 1, z) * DcY(x, 1, z) / (ALPHA(x, 1, z) + ALPHA(x, 0, z));
        diag[0] = 1. + dt * next / SQ(g->dy);
        u_diag[0] = -dt * next / SQ(g->dy);

        prev = ALPHA(x, g->size_y - 2, z) * DcY(x, g->size_y - 1, z) /
               (ALPHA(x, g->size_y - 1, z) + ALPHA(x, g->size_y - 2, z));

        diag[g->size_y - 1] = 1. + dt * prev / SQ(g->dy);
        l_diag[g->size_y - 2] = -dt * prev / SQ(g->dy);

        RHS[0] = state[x + z * g->size_x] - dt * Fyy(1, 0) / (SQ(g->dy) * ALPHA(x, 0, z));
        y = g->size_y - 1;
        RHS[y] = state[x + (z + y * g->size_z) * g->size_x] +
                 (dt / ALPHA(x, y, z)) * Fyy(y, y - 1) / SQ(g->dy);
    } else {
        diag[0] = 1.0;
        u_diag[0] = 0;
        diag[g->size_y - 1] = 1.0;
        l_diag[g->size_y - 2] = 0;
        RHS[0] = g->bc->value;
        RHS[g->size_y - 1] = g->bc->value;
    }
    for (y = 1; y < g->size_y - 1; y++) {
#ifndef __PGI
        __builtin_prefetch(&state[x + (z + (y + PREFETCH) * g->size_z) * g->size_x], 0, 0);
        __builtin_prefetch(&(g->states[IDX(x, y + PREFETCH, z)]), 0, 1);
#endif
        RHS[y] = state[x + (z + y * g->size_z) * g->size_x] -
                 (dt / ALPHA(x, y, z)) * (Fyy(y + 1, y) - Fyy(y, y - 1)) / SQ(g->dy);
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
static void ecs_dg_adi_vol_z(ECS_Grid_node* g,
                             double const dt,
                             int const x,
                             int const y,
                             double const* const state,
                             double* const RHS,
                             double* const scratch) {
    int z;
    double* diag;
    double* l_diag;
    double* u_diag;
    double prev, next;

    /*TODO: Get rid of this by not calling dg_adi when on the boundary for DIRICHLET conditions*/
    if (g->bc->type == DIRICHLET &&
        (x == 0 || y == 0 || x == g->size_x - 1 || y == g->size_y - 1)) {
        for (z = 0; z < g->size_z; z++)
            RHS[z] = g->bc->value;
        return;
    }
    if (g->size_z == 1) {
        if (g->bc->type == DIRICHLET) {
            RHS[0] = g->bc->value;
        } else {
            RHS[0] = state[y + g->size_y * x];
        }
        return;
    }

    diag = (double*) malloc(g->size_z * sizeof(double));
    l_diag = (double*) malloc((g->size_z - 1) * sizeof(double));
    u_diag = (double*) malloc((g->size_z - 1) * sizeof(double));

    for (z = 1; z < g->size_z - 1; z++) {
        prev = ALPHA(x, y, z - 1) * DcZ(x, y, z) / (ALPHA(x, y, z - 1) + ALPHA(x, y, z));
        next = ALPHA(x, y, z + 1) * DcZ(x, y, z + 1) / (ALPHA(x, y, z + 1) + ALPHA(x, y, z));

        l_diag[z - 1] = -dt * prev / SQ(g->dz);
        diag[z] = 1. + dt * (prev + next) / SQ(g->dz);
        u_diag[z] = -dt * next / SQ(g->dz);
    }

    if (g->bc->type == NEUMANN) {
        /*zero flux boundary condition*/
        next = ALPHA(x, y, 1) * DcZ(x, y, 1) / (ALPHA(x, y, 1) + ALPHA(x, y, 0));
        diag[0] = 1. + dt * next / SQ(g->dz);
        u_diag[0] = -dt * next / SQ(g->dz);

        prev = ALPHA(x, y, g->size_z - 2) * DcZ(x, y, g->size_z - 1) /
               (ALPHA(x, y, g->size_z - 1) + ALPHA(x, y, g->size_z - 2));

        diag[g->size_z - 1] = 1. + dt * prev / SQ(g->dz);
        l_diag[g->size_z - 2] = -dt * prev / SQ(g->dz);

        RHS[0] = state[y + g->size_y * (x * g->size_z)] -
                 dt * Fzz(1, 0) / (SQ(g->dz) * ALPHA(x, y, 0));
        z = g->size_z - 1;
        RHS[z] = state[y + g->size_y * (x * g->size_z + z)] +
                 (dt / ALPHA(x, y, z)) * Fzz(z, z - 1) / SQ(g->dz);
    } else {
        diag[0] = 1.0;
        u_diag[0] = 0;
        diag[g->size_z - 1] = 1.0;
        l_diag[g->size_z - 2] = 0;
        RHS[0] = g->bc->value;
        RHS[g->size_z - 1] = g->bc->value;
    }
    for (z = 1; z < g->size_z - 1; z++) {
        RHS[z] = state[y + g->size_y * (x * g->size_z + z)] -
                 (dt / ALPHA(x, y, z)) * (Fzz(z + 1, z) - Fzz(z, z - 1)) / SQ(g->dz);
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
void ecs_set_adi_vol(ECS_Grid_node* g) {
    g->ecs_adi_dir_x->ecs_dg_adi_dir = ecs_dg_adi_vol_x;
    g->ecs_adi_dir_y->ecs_dg_adi_dir = ecs_dg_adi_vol_y;
    g->ecs_adi_dir_z->ecs_dg_adi_dir = ecs_dg_adi_vol_z;
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
static void ecs_dg_adi_tort_x(ECS_Grid_node* g,
                              const double dt,
                              const int y,
                              const int z,
                              double const* const state,
                              double* const RHS,
                              double* const scratch) {
    int yp, ypd, ym, ymd, zp, zpd, zm, zmd, div_y, div_z;
    int x;
    double* diag;
    double* l_diag;
    double* u_diag;

    if (g->bc->type == DIRICHLET &&
        (y == 0 || z == 0 || y == g->size_y - 1 || z == g->size_z - 1)) {
        for (x = 0; x < g->size_x; x++)
            RHS[x] = g->bc->value;
        return;
    }

    /*zero flux boundary condition*/
    div_y = (y == 0 || y == g->size_y - 1) ? 2 : 1;
    div_z = (z == 0 || z == g->size_z - 1) ? 2 : 1;
    if (g->size_y == 1) {
        yp = 0;
        ypd = 0;
        ym = 0;
        ymd = 0;
    } else {
        yp = (y == g->size_y - 1) ? y - 1 : y + 1;
        ypd = (y == g->size_y - 1) ? y : y + 1;
        ym = (y == 0) ? y + 1 : y - 1;
        ymd = (y == 0) ? 1 : y;
    }
    if (g->size_z == 1) {
        zp = 0;
        zpd = 0;
        zm = 0;
        zmd = 0;
    } else {
        zp = (z == g->size_z - 1) ? z - 1 : z + 1;
        zpd = (z == g->size_z - 1) ? z : z + 1;
        zm = (z == 0) ? z + 1 : z - 1;
        zmd = (z == 0) ? 1 : z;
    }

    if (g->size_x == 1) {
        if (g->bc->type == DIRICHLET) {
            RHS[0] = g->bc->value;
        } else {
            RHS[0] = 0;
            if (g->size_y > 1)
                RHS[0] += (DcY(0, ypd, z) * state[IDX(0, yp, z)] -
                           (DcY(0, ypd, z) + DcY(0, ymd, z)) * state[IDX(0, y, z)] +
                           DcY(0, ymd, z) * state[IDX(0, ym, z)]) /
                          (div_y * SQ(g->dy));
            if (g->size_z > 1)
                RHS[0] += (DcZ(0, y, zpd) * state[IDX(0, y, zp)] -
                           (DcZ(0, y, zpd) + DcZ(0, y, zmd)) * state[IDX(0, y, z)] +
                           DcZ(0, y, zmd) * state[IDX(0, y, zm)]) /
                          (div_z * SQ(g->dz));
            RHS[0] *= dt;
            RHS[0] += state[IDX(0, y, z)] + g->states_cur[IDX(0, y, z)];
        }
        return;
    }

    diag = (double*) malloc(g->size_x * sizeof(double));
    l_diag = (double*) malloc((g->size_x - 1) * sizeof(double));
    u_diag = (double*) malloc((g->size_x - 1) * sizeof(double));

    for (x = 1; x < g->size_x - 1; x++) {
        l_diag[x - 1] = -dt * DcX(x, y, z) / (2. * SQ(g->dx));
        diag[x] = 1. + dt * (DcX(x, y, z) + DcX(x + 1, y, z)) / (2. * SQ(g->dx));
        u_diag[x] = -dt * DcX(x + 1, y, z) / (2. * SQ(g->dx));
    }


    if (g->bc->type == NEUMANN) {
        diag[0] = 1. + 0.5 * dt * DcX(1, y, z) / SQ(g->dx);
        u_diag[0] = -0.5 * dt * DcX(1, y, z) / SQ(g->dx);
        diag[g->size_x - 1] = 1. + 0.5 * dt * DcX(g->size_x - 1, y, z) / SQ(g->dx);
        l_diag[g->size_x - 2] = -0.5 * dt * DcX(g->size_x - 1, y, z) / SQ(g->dx);


        RHS[0] = state[IDX(0, y, z)] +
                 dt * ((DcX(1, y, z) * state[IDX(1, y, z)] - DcX(1, y, z) * state[IDX(0, y, z)]) /
                           (2. * SQ(g->dx)) +
                       (DcY(0, ypd, z) * state[IDX(0, yp, z)] -
                        (DcY(0, ypd, z) + DcY(0, ymd, z)) * state[IDX(0, y, z)] +
                        DcY(0, ymd, z) * state[IDX(0, ym, z)]) /
                           (div_y * SQ(g->dy)) +
                       (DcZ(0, y, zpd) * state[IDX(0, y, zp)] -
                        (DcZ(0, y, zpd) + DcZ(0, y, zmd)) * state[IDX(0, y, z)] +
                        DcZ(0, y, zmd) * state[IDX(0, y, zm)]) /
                           (div_z * SQ(g->dz))) +
                 g->states_cur[IDX(0, y, z)];
        x = g->size_x - 1;
        RHS[x] =
            state[IDX(x, y, z)] +
            dt * ((DcX(x, y, z) * state[IDX(x - 1, y, z)] - DcX(x, y, z) * state[IDX(x, y, z)]) /
                      (2. * SQ(g->dx)) +
                  (DcY(x, ypd, z) * state[IDX(x, yp, z)] -
                   (DcY(x, ypd, z) + DcY(x, ymd, z)) * state[IDX(x, y, z)] +
                   DcY(x, ymd, z) * state[IDX(x, ym, z)]) /
                      (div_y * SQ(g->dy)) +
                  (DcZ(x, y, zpd) * state[IDX(x, y, zp)] -
                   (DcZ(x, y, zpd) + DcZ(x, y, zmd)) * state[IDX(x, y, z)] +
                   DcZ(x, y, zmd) * state[IDX(x, y, zm)]) /
                      (div_z * SQ(g->dz))) +
            g->states_cur[IDX(x, y, z)];

    } else {
        diag[0] = 1.0;
        u_diag[0] = 0.0;
        diag[g->size_x - 1] = 1.0;
        l_diag[g->size_x - 2] = 0.0;
        RHS[0] = g->bc->value;
        RHS[g->size_x - 1] = g->bc->value;
    }

    for (x = 1; x < g->size_x - 1; x++) {
#ifndef __PGI
        __builtin_prefetch(&(state[IDX(x + PREFETCH, y, z)]), 0, 1);
        __builtin_prefetch(&(state[IDX(x + PREFETCH, yp, z)]), 0, 0);
        __builtin_prefetch(&(state[IDX(x + PREFETCH, ym, z)]), 0, 0);
#endif

        RHS[x] = state[IDX(x, y, z)] +
                 dt * ((DcX(x + 1, y, z) * state[IDX(x + 1, y, z)] -
                        (DcX(x + 1, y, z) + DcX(x, y, z)) * state[IDX(x, y, z)] +
                        DcX(x, y, z) * state[IDX(x - 1, y, z)]) /
                           (2. * SQ(g->dx)) +
                       (DcY(x, ypd, z) * state[IDX(x, yp, z)] -
                        (DcY(x, ypd, z) + DcY(x, ymd, z)) * state[IDX(x, y, z)] +
                        DcY(x, ymd, z) * state[IDX(x, ym, z)]) /
                           (div_y * SQ(g->dy)) +
                       (DcZ(x, y, zpd) * state[IDX(x, y, zp)] -
                        (DcZ(x, y, zpd) + DcZ(x, y, zmd)) * state[IDX(x, y, z)] +
                        DcZ(x, y, zmd) * state[IDX(x, y, zm)]) /
                           (div_z * SQ(g->dz))) +
                 g->states_cur[IDX(x, y, z)];
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
static void ecs_dg_adi_tort_y(ECS_Grid_node* g,
                              double const dt,
                              int const x,
                              int const z,
                              double const* const state,
                              double* const RHS,
                              double* const scratch) {
    int y;
    double* diag;
    double* l_diag;
    double* u_diag;

    /*TODO: Get rid of this by not calling dg_adi when on the boundary for DIRICHLET conditions*/
    if (g->bc->type == DIRICHLET &&
        (x == 0 || z == 0 || x == g->size_x - 1 || z == g->size_z - 1)) {
        for (y = 0; y < g->size_y; y++)
            RHS[y] = g->bc->value;
        return;
    }

    if (g->size_y == 1) {
        if (g->bc->type == DIRICHLET) {
            RHS[0] = g->bc->value;
        } else {
            RHS[0] = state[x + z * g->size_x];
        }
        return;
    }

    diag = (double*) malloc(g->size_y * sizeof(double));
    l_diag = (double*) malloc((g->size_y - 1) * sizeof(double));
    u_diag = (double*) malloc((g->size_y - 1) * sizeof(double));

    for (y = 1; y < g->size_y - 1; y++) {
        l_diag[y - 1] = -dt * DcY(x, y, z) / (2. * SQ(g->dy));
        diag[y] = 1. + dt * (DcY(x, y, z) + DcY(x, y + 1, z)) / (2. * SQ(g->dy));
        u_diag[y] = -dt * DcY(x, y + 1, z) / (2. * SQ(g->dy));
    }

    if (g->bc->type == NEUMANN) {
        /*zero flux boundary condition*/
        diag[0] = 1. + 0.5 * dt * DcY(x, 1, z) / SQ(g->dy);
        u_diag[0] = -0.5 * dt * DcY(x, 1, z) / SQ(g->dy);
        diag[g->size_y - 1] = 1. + 0.5 * dt * DcY(x, g->size_y - 1, z) / SQ(g->dy);
        l_diag[g->size_y - 2] = -0.5 * dt * DcY(x, g->size_y - 1, z) / SQ(g->dy);


        RHS[0] = state[x + z * g->size_x] - dt * ((DcY(x, 1, z) * g->states[IDX(x, 1, z)] -
                                                   DcY(x, 1, z) * g->states[IDX(x, 0, z)]) /
                                                  (2. * SQ(g->dy)));
        y = g->size_y - 1;
        RHS[y] = state[x + (z + y * g->size_z) * g->size_x] -
                 dt *
                     (DcY(x, y, z) * g->states[IDX(x, y - 1, z)] -
                      DcY(x, y, z) * g->states[IDX(x, y, z)]) /
                     (2. * SQ(g->dy));
    } else {
        diag[0] = 1.0;
        u_diag[0] = 0.0;
        diag[g->size_y - 1] = 1.0;
        l_diag[g->size_y - 2] = 0.0;
        RHS[0] = g->bc->value;
        RHS[g->size_y - 1] = g->bc->value;
    }


    for (y = 1; y < g->size_y - 1; y++) {
#ifndef __PGI
        __builtin_prefetch(&state[x + (z + (y + PREFETCH) * g->size_z) * g->size_x], 0, 0);
        __builtin_prefetch(&(g->states[IDX(x, y + PREFETCH, z)]), 0, 1);
#endif
        RHS[y] = state[x + (z + y * g->size_z) * g->size_x] -
                 dt *
                     (DcY(x, y + 1, z) * g->states[IDX(x, y + 1, z)] -
                      (DcY(x, y + 1, z) + DcY(x, y, z)) * g->states[IDX(x, y, z)] +
                      DcY(x, y, z) * g->states[IDX(x, y - 1, z)]) /
                     (2. * SQ(g->dy));
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
static void ecs_dg_adi_tort_z(ECS_Grid_node* g,
                              double const dt,
                              int const x,
                              int const y,
                              double const* const state,
                              double* const RHS,
                              double* const scratch) {
    int z;
    double* diag;
    double* l_diag;
    double* u_diag;

    /*TODO: Get rid of this by not calling dg_adi when on the boundary for DIRICHLET conditions*/
    if (g->bc->type == DIRICHLET &&
        (x == 0 || y == 0 || x == g->size_x - 1 || y == g->size_y - 1)) {
        for (z = 0; z < g->size_z; z++)
            RHS[z] = g->bc->value;
        return;
    }

    if (g->size_z == 1) {
        if (g->bc->type == DIRICHLET) {
            RHS[0] = g->bc->value;
        } else {
            RHS[0] = state[y + g->size_y * x];
        }
        return;
    }
    diag = (double*) malloc(g->size_z * sizeof(double));
    l_diag = (double*) malloc((g->size_z - 1) * sizeof(double));
    u_diag = (double*) malloc((g->size_z - 1) * sizeof(double));


    for (z = 1; z < g->size_z - 1; z++) {
        l_diag[z - 1] = -dt * DcZ(x, y, z) / (2. * SQ(g->dz));
        diag[z] = 1. + dt * (DcZ(x, y, z) + DcZ(x, y, z + 1)) / (2. * SQ(g->dz));
        u_diag[z] = -dt * DcZ(x, y, z + 1) / (2. * SQ(g->dz));
    }

    if (g->bc->type == NEUMANN) {
        /*zero flux boundary condition*/
        diag[0] = 1. + 0.5 * dt * DcZ(x, y, 1) / SQ(g->dz);
        u_diag[0] = -0.5 * dt * DcZ(x, y, 1) / SQ(g->dz);
        diag[g->size_z - 1] = 1. + 0.5 * dt * DcZ(x, y, g->size_z - 1) / SQ(g->dz);
        l_diag[g->size_z - 2] = -0.5 * dt * DcZ(x, y, g->size_z - 1) / SQ(g->dz);

        RHS[0] = state[y + g->size_y * (x * g->size_z)] -
                 dt * ((DcZ(x, y, 1) * g->states[IDX(x, y, 1)] -
                        DcZ(x, y, 1) * g->states[IDX(x, y, 0)]) /
                       (2. * SQ(g->dz)));
        z = g->size_z - 1;
        RHS[z] = state[y + g->size_y * (x * g->size_z + z)] -
                 dt *
                     (DcZ(x, y, z) * g->states[IDX(x, y, z - 1)] -
                      DcZ(x, y, z) * g->states[IDX(x, y, z)]) /
                     (2. * SQ(g->dz));
    } else {
        diag[0] = 1.0;
        u_diag[0] = 0.0;
        diag[g->size_z - 1] = 1.0;
        l_diag[g->size_z - 2] = 0.0;
        RHS[0] = g->bc->value;
        RHS[g->size_z - 1] = g->bc->value;
    }


    for (z = 1; z < g->size_z - 1; z++) {
        RHS[z] = state[y + g->size_y * (x * g->size_z + z)] -
                 dt *
                     (DcZ(x, y, z + 1) * g->states[IDX(x, y, z + 1)] -
                      (DcZ(x, y, z + 1) + DcZ(x, y, z)) * g->states[IDX(x, y, z)] +
                      DcZ(x, y, z) * g->states[IDX(x, y, z - 1)]) /
                     (2. * SQ(g->dz));
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
void ecs_set_adi_tort(ECS_Grid_node* g) {
    g->ecs_adi_dir_x->ecs_dg_adi_dir = ecs_dg_adi_tort_x;
    g->ecs_adi_dir_y->ecs_dg_adi_dir = ecs_dg_adi_tort_y;
    g->ecs_adi_dir_z->ecs_dg_adi_dir = ecs_dg_adi_tort_z;
}


/*****************************************************************************
 *
 * Begin variable step code
 *
 *****************************************************************************/

void _rhs_variable_step_helper_tort(Grid_node* g, double const* const states, double* ydot) {
    int num_states_x = g->size_x, num_states_y = g->size_y, num_states_z = g->size_z;
    double dx = g->dx, dy = g->dy, dz = g->dz;
    int i, j, k, stop_i, stop_j, stop_k;
    double rate_x = 1. / (dx * dx);
    double rate_y = 1. / (dy * dy);
    double rate_z = 1. / (dz * dz);

    /*indices*/
    int index, prev_i, prev_j, prev_k, next_i, next_j, next_k;
    int xpd, xmd, ypd, ymd, zpd, zmd;
    double div_x, div_y, div_z;

    /* Euler advance x, y, z (all points) */
    stop_i = num_states_x - 1;
    stop_j = num_states_y - 1;
    stop_k = num_states_z - 1;
    if (g->bc->type == NEUMANN) {
        for (i = 0,
            index = 0,
            prev_i = num_states_y * num_states_z,
            next_i = num_states_y * num_states_z;
             i < num_states_x;
             i++) {
            /*Zero flux boundary conditions*/
            div_x = (i == 0 || i == stop_i) ? 2. : 1.;
            xpd = (i == stop_i) ? i : i + 1;
            xmd = (i == 0) ? 1 : i;

            for (j = 0, prev_j = index + num_states_z, next_j = index + num_states_z;
                 j < num_states_y;
                 j++) {
                div_y = (j == 0 || j == stop_j) ? 2. : 1.;
                ypd = (j == stop_j) ? j : j + 1;
                ymd = (j == 0) ? 1 : j;

                for (k = 0, prev_k = index + 1, next_k = index + 1; k < num_states_z;
                     k++, index++, prev_i++, next_i++, prev_j++, next_j++) {
                    div_z = (k == 0 || k == stop_k) ? 2. : 1.;
                    zpd = (k == stop_k) ? k : k + 1;
                    zmd = (k == 0) ? 1 : k;

                    if (stop_i > 0) {
                        ydot[index] += rate_x *
                                       (DcX(xmd, j, k) * states[prev_i] -
                                        (DcX(xmd, j, k) + DcX(xpd, j, k)) * states[index] +
                                        DcX(xpd, j, k) * states[next_i]) /
                                       div_x;
                    }
                    if (stop_j > 0) {
                        ydot[index] += rate_y *
                                       (DcY(i, ymd, k) * states[prev_j] -
                                        (DcY(i, ymd, k) + DcY(i, ypd, k)) * states[index] +
                                        DcY(i, ypd, k) * states[next_j]) /
                                       div_y;
                    }

                    if (stop_k > 0) {
                        ydot[index] += rate_z *
                                       (DcZ(i, j, zmd) * states[prev_k] -
                                        (DcZ(i, j, zmd) + DcZ(i, j, zpd)) * states[index] +
                                        DcZ(i, j, zpd) * states[next_k]) /
                                       div_z;
                    }

                    next_k = (k == stop_k - 1) ? index : index + 2;
                    prev_k = index;
                }
                prev_j = index - num_states_z;
                next_j = (j == stop_j - 1) ? prev_j : index + num_states_z;
            }
            prev_i = index - num_states_y * num_states_z;
            next_i = (i == stop_i - 1) ? prev_i : index + num_states_y * num_states_z;
        }
    } else {
        for (i = 0, index = 0, prev_i = 0, next_i = num_states_y * num_states_z; i < num_states_x;
             i++) {
            for (j = 0, prev_j = index - num_states_z, next_j = index + num_states_z;
                 j < num_states_y;
                 j++) {
                for (k = 0, prev_k = index - 1, next_k = index + 1; k < num_states_z;
                     k++, index++, prev_i++, next_i++, prev_j++, next_j++, next_k++, prev_k++) {
                    if (i == 0 || i == stop_i || j == 0 || j == stop_j || k == 0 || k == stop_k) {
                        // set to zero to prevent currents altering concentrations at the boundary
                        ydot[index] = 0;
                    } else {
                        ydot[index] += rate_x * (DcX(i, j, k) * states[prev_i] -
                                                 (DcX(i, j, k) + DcX(i + 1, j, k)) * states[index] +
                                                 DcX(i + 1, j, k) * states[next_i]);

                        ydot[index] += rate_y * (DcY(i, j, k) * states[prev_j] -
                                                 (DcY(i, j, k) + DcY(i, j + 1, k)) * states[index] +
                                                 DcY(i, j + 1, k) * states[next_j]);

                        ydot[index] += rate_z * (DcZ(i, j, k) * states[prev_k] -
                                                 (DcZ(i, j, k) + DcZ(i, j, k + 1)) * states[index] +
                                                 DcZ(i, j, k + 1) * states[next_k]);
                    }
                }
                prev_j = index - num_states_z;
                next_j = index + num_states_z;
            }
            prev_i = index - num_states_y * num_states_z;
            next_i = index + num_states_y * num_states_z;
        }
    }
}


void _rhs_variable_step_helper_vol(Grid_node* g, double const* const states, double* ydot) {
    int num_states_x = g->size_x, num_states_y = g->size_y, num_states_z = g->size_z;
    double dx = g->dx, dy = g->dy, dz = g->dz;
    int i, j, k, stop_i, stop_j, stop_k;
    double rate_x = g->dc_x / (dx * dx);
    double rate_y = g->dc_y / (dy * dy);
    double rate_z = g->dc_z / (dz * dz);


    /*indices*/
    int index, prev_i, prev_j, prev_k, next_i, next_j, next_k;

    /*zero flux boundary conditions*/
    int xpd, xmd, ypd, ymd, zpd, zmd;
    double div_x, div_y, div_z;

    /* Euler advance x, y, z (all points) */
    stop_i = num_states_x - 1;
    stop_j = num_states_y - 1;
    stop_k = num_states_z - 1;
    if (g->bc->type == NEUMANN) {
        for (i = 0,
            index = 0,
            prev_i = num_states_y * num_states_z,
            next_i = num_states_y * num_states_z;
             i < num_states_x;
             i++) {
            /*Zero flux boundary conditions*/
            div_x = (i == 0 || i == stop_i) ? 2. : 1.;
            xpd = (i == stop_i) ? i : i + 1;
            xmd = (i == 0) ? 1 : i;

            for (j = 0, prev_j = index + num_states_z, next_j = index + num_states_z;
                 j < num_states_y;
                 j++) {
                div_y = (j == 0 || j == stop_j) ? 2. : 1.;
                ypd = (j == stop_j) ? j : j + 1;
                ymd = (j == 0) ? 1 : j;

                for (k = 0, prev_k = index + 1, next_k = index + 1; k < num_states_z;
                     k++, index++, prev_i++, next_i++, prev_j++, next_j++) {
                    div_z = (k == 0 || k == stop_k) ? 2. : 1.;
                    zpd = (k == stop_k) ? k : k + 1;
                    zmd = (k == 0) ? 1 : k;

                    /*x-direction*/
                    if (stop_i > 0) {
                        ydot[index] += rate_x *
                                       (FLUX(next_i, index) * PERM(xpd, j, k) +
                                        FLUX(prev_i, index) * PERM(xmd, j, k)) /
                                       (VOLFRAC(index) * div_x);
                    }

                    /*y-direction*/
                    if (stop_j > 0) {
                        ydot[index] += rate_y *
                                       (FLUX(next_j, index) * PERM(i, ypd, k) +
                                        FLUX(prev_j, index) * PERM(i, ymd, k)) /
                                       (VOLFRAC(index) * div_y);
                    }

                    /*z-direction*/
                    if (stop_k > 0) {
                        ydot[index] += rate_z *
                                       (FLUX(next_k, index) * PERM(i, j, zpd) +
                                        FLUX(prev_k, index) * PERM(i, j, zmd)) /
                                       (VOLFRAC(index) * div_z);
                    }

                    next_k = (k == stop_k - 1) ? index : index + 2;
                    prev_k = index;
                }
                prev_j = index - num_states_z;
                next_j = (j == stop_j - 1) ? prev_j : index + num_states_z;
            }
            prev_i = index - num_states_y * num_states_z;
            next_i = (i == stop_i - 1) ? prev_i : index + num_states_y * num_states_z;
        }
    } else {
        for (i = 0, index = 0, prev_i = 0, next_i = num_states_y * num_states_z; i < num_states_x;
             i++) {
            for (j = 0, prev_j = index - num_states_z, next_j = index + num_states_z;
                 j < num_states_y;
                 j++) {
                for (k = 0, prev_k = index - 1, next_k = index + 1; k < num_states_z;
                     k++, index++, prev_i++, next_i++, prev_j++, next_j++, next_k++, prev_k++) {
                    if (i == 0 || i == stop_i || j == 0 || j == stop_j || k == 0 || k == stop_k) {
                        // set to zero to prevent currents altering concentrations at the boundary
                        ydot[index] = 0;
                    } else {
                        /*x-direction*/
                        ydot[index] += rate_x *
                                       (FLUX(next_i, index) * PERM(i + 1, j, k) +
                                        FLUX(prev_i, index) * PERM(i, j, k)) /
                                       (VOLFRAC(index));

                        /*y-direction*/
                        ydot[index] += rate_y *
                                       (FLUX(next_j, index) * PERM(i, j + 1, k) +
                                        FLUX(prev_j, index) * PERM(i, j, k)) /
                                       (VOLFRAC(index));

                        /*z-direction*/
                        ydot[index] += rate_z *
                                       (FLUX(next_k, index) * PERM(i, j, k + 1) +
                                        FLUX(prev_k, index) * PERM(i, j, k)) /
                                       (VOLFRAC(index));
                    }
                }
                prev_j = index - num_states_z;
                next_j = index + num_states_z;
            }
            prev_i = index - num_states_y * num_states_z;
            next_i = index + num_states_y * num_states_z;
        }
    }
}
