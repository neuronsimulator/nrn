#include <../../nrnconf.h>

#include <stdio.h>
#include <errno.h>
#include <InterViews/resource.h>
#include <OS/list.h>
#include <nrnoc2iv.h>
#include <nrnmpi.h>
#include <nrnhash.h>
#include <mymath.h>

extern "C" {
void nrnmpi_source_var();
void nrnmpi_target_var();
void nrnmpi_setup_transfer();
void nrn_partrans_clear();
static void var_transfer();
extern double t;
extern void nrnmpi_int_allgather(int*, int*, int);
extern void nrnmpi_int_allgatherv(int*, int*, int*, int*);
extern void nrnmpi_dbl_allgatherv(double*, double*, int*, int*);
#if 1 || PARANEURON
extern void (*nrnmpi_v_transfer_)(); // before nonvint and BEFORE INITIAL
#endif
#if PARANEURON
extern double nrnmpi_transfer_wait_;
#endif
}

void nrn_partrans_update_ptrs(int, double**, double*);

declareNrnHash(MapInt2Int, int, int);
implementNrnHash(MapInt2Int, int, int);
declarePtrList(DblPList, double)
implementPtrList(DblPList, double)
declareList(IntList, int)
implementList(IntList, int)
static int src_buf_size_, *srccnt_, *srcdspl_;
static DblPList* targets_;
static IntList* sgid2targets_;
static DblPList* sources_;
static IntList* sgids_;
static MapInt2Int* sgid2srcindex_;
static double* incoming_source_buf_;
static double* outgoing_source_buf_;
static int* s2t_index_;

static boolean is_setup_;
static void alloclists();

void nrnmpi_source_var() {
	alloclists();
	is_setup_ = false;
	double* psv = hoc_pgetarg(1);
	int sgid = (int)(*getarg(2));
	int i;
	if (sgid2srcindex_->find(sgid, i)) {
		char tmp[10];
		sprintf(tmp, "%d", sgid);
		hoc_execerror("source var gid already in use:", tmp);
	}
	(*sgid2srcindex_)[sgid] = sources_->count();
	sources_->append(psv);
	sgids_->append(sgid);
//	printf("nrnmpi_source_var source_val=%g sgid=%d\n", *psv, sgid);
}

void nrnmpi_target_var() {
	alloclists();
	is_setup_ = false;
	double* ptv = hoc_pgetarg(1);
	int sgid = (int)(*getarg(2));
	targets_->append(ptv);
	sgid2targets_->append(sgid);
//	printf("nrnmpi_target_var target_val=%g sgid=%d\n", *ptv, sgid);
}

void nrn_partrans_update_ptrs(int ncnt, double** oldvp, double* newv) {
	int i, n;
	if (sources_) {
		n = sources_->count();
		DblPList* newsrc = new DblPList(n);
		for (i=0; i < n; ++i) {
			if (nrn_isdouble(sources_->item(i), 0, ncnt-1)) {
				int k = (int)(*sources_->item(i));
				if (sources_->item(i) == oldvp[k]) {
					newsrc->append(newv + k);
				}else{
					newsrc->append(sources_->item(i));
				}
			}else{ // not v
				newsrc->append(sources_->item(i));
			}
		}
		delete sources_;
		sources_ = newsrc;
	}
	if (targets_) {
		n = targets_->count();
		DblPList* newtar = new DblPList(n);
		for (i=0; i < n; ++i) {
			if (nrn_isdouble(targets_->item(i), 0, ncnt-1)) {
				int k = (int)(*targets_->item(i));
				if (targets_->item(i) == oldvp[k]) {
					newtar->append(newv + k);
				}else{
					newtar->append(targets_->item(i));
				}
			}else{ // not v
				newtar->append(targets_->item(i));
			}
		}
		delete targets_;
		targets_ = newtar;
	}
}

//static FILE* xxxfile;

void var_transfer() {
	if (!is_setup_) {
		hoc_execerror("ParallelContext.setup_transfer()", "needs to be called.");
	}
//	fprintf(xxxfile, "%g\n", t);
	int i, n = sources_->count();
	for (i=0; i < n; ++i) {
		outgoing_source_buf_[i] = *sources_->item(i);
	}
#if 0
for (i=0; i < 10; ++i) {
	printf("outgoing %d %g\n", i, outgoing_source_buf_[i]);
}
#endif
#if PARANEURON
	if (nrnmpi_numprocs > 1) {
		double wt = nrnmpi_wtime();
		nrnmpi_dbl_allgatherv(outgoing_source_buf_, incoming_source_buf_,
			srccnt_, srcdspl_);
		nrnmpi_transfer_wait_ += nrnmpi_wtime() - wt;
		errno = 0;
	}
#endif
	n = targets_->count();
	for (i=0; i < n; ++i) {
		*targets_->item(i) = incoming_source_buf_[s2t_index_[i]];
	}
#if 0
for (i=0; i < 10; ++i) {
	printf("targets %d %d %g\n", i, s2t_index_[i], *targets_->item(i));
}
#endif
}

// The simplest conceivable transfer is to use MPI_Allgatherv and send
// all sources to all machines. More complicated and possibly more efficient
// in terms of total received buffer size
// would be to use MPI_Alltoallv in which distinct data is sent and received.
// We begin with MPI_Allgatherv. We try
// to save a trivial copy by making
// outgoing_source_buf a pointer into the incoming_source_buf.

void nrnmpi_setup_transfer() {
//	char ctmp[100];
//	sprintf(ctmp, "vartrans%d", nrnmpi_myid);
//	xxxfile = fopen(ctmp, "w");
	alloclists();
	is_setup_ = true;
//	printf("nrnmpi_setup_transfer\n");
	// send this machine source count to all other machines and get the
	// source counts for all machines. This allows us to create the
	// proper incoming source buffer and make the outgoing source pointer
	// point to the right spot in that.
	if (!srccnt_) {
		srccnt_ = new int[nrnmpi_numprocs];
		srcdspl_ = new int[nrnmpi_numprocs];
	}
	srccnt_[nrnmpi_myid] = sources_->count();
#if PARANEURON
	if (nrnmpi_numprocs > 1) {
		nrnmpi_int_allgather(srccnt_ + nrnmpi_myid, srccnt_, 1);
		errno = 0;
	}
#else
	if (nrnmpi_numprocs > 1) {
		hoc_execerror("To use ParallelContext.setup_transfer when nhost > 1, NEURON must be configured with --with-paranrn", 0);
	}
#endif
	
	// allocate the source buffer
	int i, j;
	src_buf_size_ = 0;
	for (i=0; i < nrnmpi_numprocs; ++i) {
		srcdspl_[i] = src_buf_size_;
		src_buf_size_ += srccnt_[i];
	}
	if (incoming_source_buf_) {
		delete [] incoming_source_buf_;
		incoming_source_buf_ = 0;
		outgoing_source_buf_ = 0;
	}
	if (src_buf_size_ == 0) {
		nrnmpi_v_transfer_ = 0;
		return;
	}
	int osb = srcdspl_[nrnmpi_myid];
	incoming_source_buf_ = new double[src_buf_size_];
	outgoing_source_buf_ = incoming_source_buf_ + osb;

	// send this machines list of sgids (in source buf order) to all
	// machines and receive corresponding lists from all machines. This
	// allows us to create the source buffer index to target list
	// (by finding the index into the sgid to target list).
	int* sgid = new int[src_buf_size_];
	for (i = 0, j = osb; i < sources_->count(); ++i, ++j) {
		sgid[j] = sgids_->item(i);
	}
#if PARANEURON
	if (nrnmpi_numprocs > 1) {
		nrnmpi_int_allgatherv(sgid + osb, sgid, srccnt_, srcdspl_);
	}
	errno = 0;
#endif
	
	if (s2t_index_) { delete [] s2t_index_; }
	s2t_index_ = new int[targets_->count()];
	MapInt2Int* mi2 = new MapInt2Int(20);
//printf("srcbufsize=%d\n", src_buf_size_);
	for (i = 0; i < src_buf_size_; ++i) {
		if (mi2->find(sgid[i], j)) {
			char tmp[10];
			sprintf(tmp, "%d", sgid[i]);
			hoc_execerror("multiple instances of source gid:", tmp);
		}
		(*mi2)[sgid[i]] = i;
	}
	for (i=0; i < targets_->count(); ++i) {
		assert(mi2->find(sgid2targets_->item(i), s2t_index_[i]));
	}
	delete [] sgid;
	delete mi2;
	nrnmpi_v_transfer_ = var_transfer;
}

void alloclists() {
	if (!targets_) {
		targets_ = new DblPList(100);
		sgid2targets_ = new IntList(100);
		sources_ = new DblPList(100);
		sgids_ = new IntList(100);
		sgid2srcindex_ = new MapInt2Int(256);
	}
	nrnmpi_v_transfer_ = var_transfer;
}

void nrn_partrans_clear() {
	if (!targets_) { return; }
	nrnmpi_v_transfer_ = 0;
	delete sgid2srcindex_;
	delete sgids_;
	delete sources_;
	delete sgid2targets_;
	delete targets_;
	targets_ = 0;
}
