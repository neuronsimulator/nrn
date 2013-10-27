#ifndef multicore_h
#define multicore_h


#if defined(__cplusplus)
extern "C" {
#endif

#include <membfunc.h>


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
        int ncell; /* analogous to old rootnodecount */
	int end;    /* 1 + position of last in v_node array. Now v_node_count. */
	int id; /* this is nrn_threads[id] */
	int _stop_stepping; /* delivered an all thread HocEvent */

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
