#include <../../nrnconf.h>
#include <assert.h>
#include "nrnassrt.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* do not want the redef in the dynamic load case */
#include <nrnmpiuse.h>
#if NRNMPI_DYNAMICLOAD
#include <nrnmpi_dynam.h> /* define all the nrnmpi functions name to f_name */
#endif

#include <nrnmpi.h>
#include <mpispike.h>


#if NRNMPI_DYNAMICLOAD
#else
#include "nrnmpi_def_cinc"
#endif

extern double nrn_timeus();

#if NRNMPI
#include <mpi.h>
#define nrn_mpi_assert(arg) nrn_assert(arg == MPI_SUCCESS)

#if NRN_MUSIC
#include "nrnmusicapi.h"
MPI_Comm nrnmusic_comm;
extern int nrnmusic;
#endif

MPI_Comm nrnmpi_world_comm;
MPI_Comm nrnmpi_comm;
MPI_Comm nrn_bbs_comm;
static MPI_Group grp_bbs;
static MPI_Group grp_net;

static int nrnmpi_numprocs_subworld = 1;
static int nrnmpi_subworld_id = -1;
static int nrnmpi_subworld_change_cnt = 0;

extern void nrnmpi_spike_initialize();

#define nrnmpidebugleak 0
#if nrnmpidebugleak
extern void nrnmpi_checkbufleak();
#endif

static int nrnmpi_under_nrncontrol_;
static int nrnmpi_is_setup_;
#endif

extern "C" void nrnmpi_init(int nrnmpi_under_nrncontrol, int* pargc, char*** pargv) {
#if NRNMPI
    int i, b, flag;
    if (nrnmpi_use) {
        return;
    }
    nrnmpi_under_nrncontrol_ = nrnmpi_under_nrncontrol;
    if (nrnmpi_under_nrncontrol_) {
#if 0
{int i;
printf("nrnmpi_init: argc=%d\n", *pargc);
for (i=0; i < *pargc; ++i) {
        printf("%d |%s|\n", i, (*pargv)[i]);
}
}
#endif

#if NRN_MUSIC
        nrnmusic_init(pargc, pargv); /* see src/nrniv/nrnmusic.cpp */
#endif

#if !ALWAYS_CALL_MPI_INIT
        /* this is not good. depends on mpirun adding at least one
           arg that starts with -p4 but that probably is dependent
           on mpich and the use of the ch_p4 device. We are trying to
           work around the problem that MPI_Init may change the working
           directory and so when not invoked under mpirun we would like to
           NOT call MPI_Init.
        */
        b = 0;
        for (i = 0; i < *pargc; ++i) {
            if (strncmp("-p4", (*pargv)[i], 3) == 0) {
                b = 1;
                break;
            }
            if (strcmp("-mpi", (*pargv)[i]) == 0) {
                b = 1;
                break;
            }
        }
        if (nrnmpi_under_nrncontrol_ == 2) {
            b = 1;
            nrnmpi_under_nrncontrol_ = 1;
        }
#if NRN_MUSIC
        if (nrnmusic) {
            b = 1;
        }
#endif
        if (!b) {
            nrnmpi_under_nrncontrol_ = 0;
            return;
        }
#endif
        MPI_Initialized(&flag);

        /* only call MPI_Init if not already initialized */
        if (!flag) {
#if (NRN_ENABLE_THREADS)
            int required = MPI_THREAD_SERIALIZED;
            int provided;
            nrn_mpi_assert(MPI_Init_thread(pargc, pargv, required, &provided));
            if (required > provided) {
                nrn_cannot_use_threads_and_mpi = 1;
            }
#else
            nrn_mpi_assert(MPI_Init(pargc, pargv));
#endif
            nrnmpi_under_nrncontrol_ = 1;
#if NRN_MUSIC
        } else if (!nrnmusic) {
            nrnmpi_under_nrncontrol_ = 0;
#endif
        }

#if NRN_MUSIC
        if (nrnmusic) {
            nrn_mpi_assert(MPI_Comm_dup(nrnmusic_comm, &nrnmpi_world_comm));
        } else {
#else
        {
#endif
            nrn_mpi_assert(MPI_Comm_dup(MPI_COMM_WORLD, &nrnmpi_world_comm));
        }
    }
    grp_bbs = MPI_GROUP_NULL;
    grp_net = MPI_GROUP_NULL;
    nrn_mpi_assert(MPI_Comm_dup(nrnmpi_world_comm, &nrnmpi_comm));
    nrn_mpi_assert(MPI_Comm_dup(nrnmpi_world_comm, &nrn_bbs_comm));
    nrn_mpi_assert(MPI_Comm_rank(nrnmpi_world_comm, &nrnmpi_myid_world));
    nrn_mpi_assert(MPI_Comm_size(nrnmpi_world_comm, &nrnmpi_numprocs_world));
    nrnmpi_numprocs = nrnmpi_numprocs_bbs = nrnmpi_numprocs_world;
    nrnmpi_myid = nrnmpi_myid_bbs = nrnmpi_myid_world;
    nrnmpi_spike_initialize();
    nrnmpi_use = 1;
    nrnmpi_subworld_change_cnt = 0;  // increment from within void nrnmpi_subworld_size(int n)
    nrnmpi_subworld_id = 0;          // Subworld index of current rank
    nrnmpi_numprocs_subworld = nrnmpi_numprocs_bbs;  // Size of subworld of current rank

    /*begin instrumentation*/
#if USE_HPM
    hpmInit(nrnmpi_myid_world, "mpineuron");
#endif
#if 0
{int i;
printf("nrnmpi_init: argc=%d\n", *pargc);
for (i=0; i < *pargc; ++i) {
        printf("%d |%s|\n", i, (*pargv)[i]);
}
}
#endif
#if 1
    if (nrnmpi_myid == 0) {
        printf("numprocs=%d\n", nrnmpi_numprocs_world);
    }
#endif

#endif /* NRNMPI */
}

double nrnmpi_wtime() {
#if NRNMPI
    if (nrnmpi_use) {
        return MPI_Wtime();
    }
#endif
    return nrn_timeus();
}

void nrnmpi_terminate() {
#if NRNMPI
    if (nrnmpi_use) {
#if 0
                printf("%d nrnmpi_terminate\n", nrnmpi_myid_world);
#endif
#if USE_HPM
        hpmTerminate(nrnmpi_myid_world);
#endif
        if (nrnmpi_under_nrncontrol_) {
#if NRN_MUSIC
            if (nrnmusic) {
                nrnmusic_terminate();
            } else
#endif
                MPI_Finalize();
        }
        nrnmpi_use = 0;
#if nrnmpidebugleak
        nrnmpi_checkbufleak();
#endif
    }
#endif /*NRNMPI*/
}

void nrnmpi_abort(int errcode) {
#if NRNMPI
    int flag;
    MPI_Initialized(&flag);
    if (flag) {
        MPI_Abort(MPI_COMM_WORLD, errcode);
    } else {
        abort();
    }
#else
    abort();
#endif
}

#if NRNMPI


void nrnmpi_subworld_size(int n) {
    /* n is the (desired) size of a subworld (pc.nhost) */
    /* A subworld (net) is contiguous */
    /* In case pc.nhost_world/n is not an integer, there are
       pc.nhost_world/n + 1 subworlds and the last subworld
       has pc.nhost_world%n ranks. All the other subworlds have n ranks.
    */
    if (nrnmpi_use != 1) {
        return;
    }
    if (nrnmpi_comm != MPI_COMM_NULL) {
        nrn_mpi_assert(MPI_Comm_free(&nrnmpi_comm));
        nrnmpi_comm = MPI_COMM_NULL;
    }
    if (nrn_bbs_comm != MPI_COMM_NULL) {
        nrn_mpi_assert(MPI_Comm_free(&nrn_bbs_comm));
        nrn_bbs_comm = MPI_COMM_NULL;
    }
    if (grp_bbs != MPI_GROUP_NULL) {
        nrn_mpi_assert(MPI_Group_free(&grp_bbs));
        grp_bbs = MPI_GROUP_NULL;
    }
    if (grp_net != MPI_GROUP_NULL) {
        nrn_mpi_assert(MPI_Group_free(&grp_net));
        grp_net = MPI_GROUP_NULL;
    }
    MPI_Group wg;
    nrn_mpi_assert(MPI_Comm_group(nrnmpi_world_comm, &wg));
    int r = nrnmpi_myid_world;
    /* special cases */
    if (n == 1) {
        nrn_mpi_assert(MPI_Group_incl(wg, 1, &r, &grp_net));
        nrn_mpi_assert(MPI_Comm_dup(nrnmpi_world_comm, &nrn_bbs_comm));
        nrn_mpi_assert(MPI_Comm_create(nrnmpi_world_comm, grp_net, &nrnmpi_comm));
        nrn_mpi_assert(MPI_Comm_rank(nrnmpi_comm, &nrnmpi_myid));
        nrn_mpi_assert(MPI_Comm_size(nrnmpi_comm, &nrnmpi_numprocs));
        nrn_mpi_assert(MPI_Comm_rank(nrn_bbs_comm, &nrnmpi_myid_bbs));
        nrn_mpi_assert(MPI_Comm_size(nrn_bbs_comm, &nrnmpi_numprocs_bbs));
        nrnmpi_subworld_id = nrnmpi_myid_bbs;
        nrnmpi_numprocs_subworld = nrnmpi_numprocs_bbs;
    } else if (n == nrnmpi_numprocs_world) {
        nrn_mpi_assert(MPI_Group_incl(wg, 1, &r, &grp_bbs));
        nrn_mpi_assert(MPI_Comm_dup(nrnmpi_world_comm, &nrnmpi_comm));
        nrn_mpi_assert(MPI_Comm_create(nrnmpi_world_comm, grp_bbs, &nrn_bbs_comm));
        nrn_mpi_assert(MPI_Comm_rank(nrnmpi_comm, &nrnmpi_myid));
        nrn_mpi_assert(MPI_Comm_size(nrnmpi_comm, &nrnmpi_numprocs));
        if (r == 0) {
            nrn_mpi_assert(MPI_Comm_rank(nrn_bbs_comm, &nrnmpi_myid_bbs));
            nrn_mpi_assert(MPI_Comm_size(nrn_bbs_comm, &nrnmpi_numprocs_bbs));
        } else {
            nrnmpi_myid_bbs = -1;
            nrnmpi_numprocs_bbs = -1;
        }
        nrnmpi_subworld_id = 0;
        nrnmpi_numprocs_subworld = nrnmpi_numprocs;
    } else {
        int nw = nrnmpi_numprocs_world;
        int nb = nw / n; /* nrnmpi_numprocs_bbs */
        int ib;
        int range[3];
        if (nw % n) {
            nb += 1; /* and the last will have pc.nhost = nw%n */
        }
        /* A subworld (net) has contiguous ranks. */
        /* Every rank is in a specific nrnmpi_comm communicator */
        ib = r / n;
        range[0] = ib * n;           /* first rank in group */
        range[1] = range[0] + n - 1; /* last rank in group */
        if (range[1] >= nw) {
            range[1] = nw - 1;
        }
        range[2] = 1; /* stride */
        nrn_mpi_assert(MPI_Group_range_incl(wg, 1, &range, &grp_net));
        nrn_mpi_assert(MPI_Comm_create(nrnmpi_world_comm, grp_net, &nrnmpi_comm));
        nrn_mpi_assert(MPI_Comm_rank(nrnmpi_comm, &nrnmpi_myid));
        nrn_mpi_assert(MPI_Comm_size(nrnmpi_comm, &nrnmpi_numprocs));

        /* nrn_bbs_com ranks stride is nrnmpi_numprocs */
        /* only rank 0 of each subworld participates in nrn_bbs_comm */
        range[0] = 0;            /* first world rank in nrn_bbs_comm */
        range[1] = (nb - 1) * n; /* last world rank in nrn_bbs_comm */
        range[2] = n;            /* stride */
        nrn_mpi_assert(MPI_Group_range_incl(wg, 1, &range, &grp_bbs));
        nrn_mpi_assert(MPI_Comm_create(nrnmpi_world_comm, grp_bbs, &nrn_bbs_comm));
        if (r % n == 0) { /* only rank 0 participates in nrn_bbs_comm */
            nrn_mpi_assert(MPI_Comm_rank(nrn_bbs_comm, &nrnmpi_myid_bbs));
            nrn_mpi_assert(MPI_Comm_size(nrn_bbs_comm, &nrnmpi_numprocs_bbs));
        } else {
            nrnmpi_myid_bbs = -1;
            nrnmpi_numprocs_bbs = -1;
        }
        nrnmpi_subworld_id = r / n;
        nrnmpi_numprocs_subworld = n;
        if ((nw % n != 0) && (nrnmpi_subworld_id == (nb - 1))) {
            nrnmpi_numprocs_subworld = nw % n; /* and the last will have pc.nhost = nw%n */
        }
    }
    nrnmpi_subworld_change_cnt++;
    nrn_mpi_assert(MPI_Group_free(&wg));
}

/* so src/nrnpython/inithoc.cpp does not have to include a c++ mpi.h */
int nrnmpi_wrap_mpi_init(int* flag) {
    return MPI_Initialized(flag);
}

void nrnmpi_get_subworld_info(int* cnt, int* index, int* rank, int* numprocs, int* numprocs_world) {
    *cnt = nrnmpi_subworld_change_cnt;
    *index = nrnmpi_subworld_id;
    *rank = nrnmpi_myid;
    *numprocs = nrnmpi_numprocs_subworld;
    *numprocs_world = nrnmpi_numprocs_world;
}

#endif
