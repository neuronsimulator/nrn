#ifndef multicore_h
#define multicore_h

#include "simcore/nrnconf.h"
#include "simcore/nrnoc/membfunc.h"

#if defined(__cplusplus)
class NetCon;
extern "C" {
#else
typedef void NetCon;
#endif

typedef struct NrnThreadMembList{ /* patterned after CvMembList in cvodeobj.h */
	struct NrnThreadMembList* next;
	struct Memb_list* ml;
	int index;
} NrnThreadMembList;

typedef struct NrnThreadBAList {
	struct Memb_list* ml; /* an item in the NrnThreadMembList */
	struct BAMech* bam;
	struct NrnThreadBAList* next;
} NrnThreadBAList;

typedef struct NrnThread {
	double _t;
	double _dt;
	double cj;

	NrnThreadMembList* tml;
        Point_process* synapses;
        NetCon* netcons;

	NrnThreadMembList* acell_tml;
	Point_process* acells; // no gid for these
	NetCon* acell_netcons; // source is no-gid acell
	int n_syn, n_netcon, n_acell, n_acellnetcon, n_weight; // only for model_size

        int ncell; /* analogous to old rootnodecount */
	int end;    /* 1 + position of last in v_node array. Now v_node_count. */
	int id; /* this is nrn_threads[id] */
	int _stop_stepping; /* delivered an all thread HocEvent */

	size_t _ndata, _nidata, _nvdata; /* sizes */
	double* _data; /* all the other double* and Datum to doubles point into here*/
	int* _idata; /* all the Datum to ints index into here */
	void** _vdata; /* all the Datum to pointers index into here */

	double* _actual_rhs;
	double* _actual_d;
	double* _actual_a;
	double* _actual_b;
	double* _actual_v;
	double* _actual_area;
	int* _v_parent_index;
	char* _sp13mat; /* handle to general sparse matrix */
	struct Memb_list* _ecell_memb_list; /* normally nil */

#if 1
	double _ctime; /* computation time in seconds (using nrnmpi_wtime) */
#endif

	NrnThreadBAList* tbl[BEFORE_AFTER_SIZE]; /* wasteful since almost all empty */

} NrnThread;

extern void nrn_threads_create(int n, int parallel);
extern int nrn_nthread;
extern NrnThread* nrn_threads;
extern void nrn_thread_error(const char*);
extern void nrn_multithread_job(void*(*)(NrnThread*));
extern void nrn_onethread_job(int, void*(*)(NrnThread*));
extern void nrn_wait_for_threads(void);
extern void nrn_thread_table_check(void);

#define FOR_THREADS(nt) for (nt = nrn_threads; nt < nrn_threads + nrn_nthread; ++nt)

#if defined(__cplusplus)
}
#endif

#endif
