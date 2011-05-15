#include <../../nrnconf.h>

#include <stdio.h>
#include <errno.h>
#include <InterViews/resource.h>
#include <OS/list.h>
#include <nrnoc2iv.h>
#include <nrniv_mf.h>
#include <nrnmpi.h>
#include <nrnhash.h>
#include <mymath.h>

extern "C" {
void nrnmpi_source_var();
void nrnmpi_target_var();
void nrnmpi_setup_transfer();
void nrn_partrans_clear();
static void mpi_transfer();
static void thread_transfer(NrnThread*);
static void mk_ttd();
extern double t;
extern int v_structure_change;
extern double* nrn_recalc_ptr(double*);
// see lengthy comment in ../nrnoc/fadvance.c
// nrnmpi_v_transfer requires existence of nrnthread_v_transfer even if there
// is only one thread.
// Thread 0 does the nrnmpi_v_transfer into incoming_src_buf.
// Data destined for targets in thread owned memory
// is copied to the proper place by each thread via nrnthread_v_transfer
// MPI_Alltoallv is used to transfer interprocessor data.
// The basic assumption is that this will be mostly used for gap junctions in which
// most often one source voltage goes to one target or at least only a few targets.
// Note that the source data for nrnthread_v_transfer is in incoming_src_buf,
// source locations owned by thread, and source locations owned by other threads.

#if 1 || PARANEURON
extern void (*nrnthread_v_transfer_)(NrnThread*); // before nonvint and BEFORE INITIAL
extern void (*nrnmpi_v_transfer_)(); // before nrnthread_v_transfer and after update. Called by thread 0.
extern void (*nrn_mk_transfer_thread_data_)();
#endif
#if PARANEURON
extern double nrnmpi_transfer_wait_;
extern void nrnmpi_barrier();
extern void nrnmpi_int_allgather(int*, int*, int);
extern int nrnmpi_int_allmax(int);
extern void nrnmpi_int_allgatherv(int*, int*, int*, int*);
extern void nrnmpi_dbl_allgatherv(double*, double*, int*, int*);
extern void nrnmpi_int_alltoallv(int*, int*, int*,  int*, int*, int*);
extern void nrnmpi_dbl_alltoallv(double*, int*, int*,  double*, int*, int*);
#endif
}

struct TransferThreadData {
	int cnt;
	double** tv; // pointers to the ParallelContext.target_var
	double** sv; // pointers to the ParallelContext.source_var (or into MPI target buffer)
};
static TransferThreadData* transfer_thread_data_;
static int n_transfer_thread_data_;

void nrn_partrans_update_ptrs();

declareNrnHash(MapInt2Int, int, int);
implementNrnHash(MapInt2Int, int, int);
declarePtrList(DblPList, double)
implementPtrList(DblPList, double)
#define PPList partrans_PPList
declarePtrList(PPList, Point_process)
implementPtrList(PPList, Point_process)
declareList(IntList, int)
implementList(IntList, int)
static double* insrc_buf_; // Receives the interprocessor data destined for other threads.
static double* outsrc_buf_;
static double** poutsrc_; // prior to mpi copy src value to proper place in outsrc_buf_
static int insrc_buf_size_, *insrccnt_, *insrcdspl_;
static int outsrc_buf_size_, *outsrccnt_, *outsrcdspl_;
static MapInt2Int* sid2insrc_; // received interprocessor sid data is
// associated with which insrc_buf index. Created by nrnmpi_setup_transfer
// and used by mkttd
		
static DblPList* targets_;
static IntList* sgid2targets_;
static PPList* target_pntlist_;
static DblPList* sources_;
static IntList* sgids_;
static MapInt2Int* sgid2srcindex_;
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
	Point_process* pp = 0;
	alloclists();
	int iarg = 1;
	is_setup_ = false;
	if (hoc_is_object_arg(iarg)) {
		pp = ob2pntproc(*hoc_objgetarg(iarg++));
	}
	double* ptv = hoc_pgetarg(iarg++);
	int sgid = (int)(*getarg(iarg++));
	targets_->append(ptv);
	target_pntlist_->append(pp);
	sgid2targets_->append(sgid);
//	printf("nrnmpi_target_var target_val=%g sgid=%d\n", *ptv, sgid);
}

void nrn_partrans_update_ptrs() {
	int i, n;
	if (sources_) {
		n = sources_->count();
		DblPList* newsrc = new DblPList(n);
		for (i=0; i < n; ++i) {
			double* pd = nrn_recalc_ptr(sources_->item(i));
			newsrc->append(pd);
		}
		delete sources_;
		sources_ = newsrc;
	}
	if (targets_) {
		n = targets_->count();
		DblPList* newtar = new DblPList(n);
		for (i=0; i < n; ++i) {
			double* pd = nrn_recalc_ptr(targets_->item(i));
			newtar->append(pd);
		}
		delete targets_;
		targets_ = newtar;
	}
}

//static FILE* xxxfile;

static void rm_ttd() {
	if (!transfer_thread_data_){ return; }
	for (int i=0; i < n_transfer_thread_data_; ++ i) {
		TransferThreadData& ttd = transfer_thread_data_[i];
		if (ttd.tv) delete [] ttd.tv;
		if (ttd.sv) delete [] ttd.sv;
	}
	delete [] transfer_thread_data_;
	transfer_thread_data_ = 0;
	n_transfer_thread_data_ = 0;
	nrnthread_v_transfer_ = 0;
}

static void mk_ttd() {
	int i, j, k, tid, n;
	rm_ttd();
	if (!targets_ || targets_->count() == 0) { return; }
	n = targets_->count();
    if (nrn_nthread > 1) for (i=0; i < n; ++i) {
	Point_process* pp = target_pntlist_->item(i);
	int sgid = sgid2targets_->item(i);
	if (!pp) {
fprintf(stderr, "Do not know the POINT_PROCESS target for source id %d\n", sgid);
hoc_execerror("For multiple threads, the target pointer must reference a range variable of a POINT_PROCESS.",
"Note that it is fastest to supply a reference to the POINT_PROCESS as the first arg.");
	}
    }
	transfer_thread_data_ = new TransferThreadData[nrn_nthread];
	for (tid = 0; tid < nrn_nthread; ++tid) {
		transfer_thread_data_[tid].cnt = 0;
	}
	n_transfer_thread_data_ = nrn_nthread;
	// how many targets in each thread
    if (nrn_nthread == 1) {
	transfer_thread_data_[0].cnt = target_pntlist_->count();
    }else{
	for (i=0; i < n; ++i) {
		assert(target_pntlist_->item(i));
		tid = ((NrnThread*)target_pntlist_->item(i)->_vnt)->id;
		++transfer_thread_data_[tid].cnt;
	}
    }
	// allocate
	for (tid = 0; tid < nrn_nthread; ++tid) {
		TransferThreadData& ttd = transfer_thread_data_[tid];
		ttd.tv = new double*[ttd.cnt];
		ttd.sv = new double*[ttd.cnt];
		ttd.cnt = 0;
	}
	// count again and fill pointers
	for (i=0; i < n; ++i) {
		if (nrn_nthread == 1) {
			tid = 0;
		}else{
			tid = ((NrnThread*)target_pntlist_->item(i)->_vnt)->id;
		}
		TransferThreadData& ttd = transfer_thread_data_[tid];
		j = ttd.cnt++;
		ttd.tv[j] = targets_->item(i);
		// perhaps inter- or intra-thread, perhaps interprocessor
		int sid = sgid2targets_->item(i);
		if (sgid2srcindex_->find(sid, k)) {
			ttd.sv[j] = sources_->item(k);
		}else if (sid2insrc_ && sid2insrc_->find(sid, k)) {
			ttd.sv[j] = insrc_buf_ + k;
		}else{
			assert(0);
		}
	}
	nrnthread_v_transfer_ = thread_transfer;
}

void mpi_transfer() {
	int i, n = outsrc_buf_size_;
	for (i=0; i < n; ++i) {
		outsrc_buf_[i] = *poutsrc_[i];
	}
#if PARANEURON
	if (nrnmpi_numprocs > 1) {
		double wt = nrnmpi_wtime();
		nrnmpi_dbl_alltoallv(outsrc_buf_, outsrccnt_, outsrcdspl_,
			insrc_buf_, insrccnt_, insrcdspl_);
		nrnmpi_transfer_wait_ += nrnmpi_wtime() - wt;
		errno = 0;
	}
#endif
	// insrc_buf_ will get transferred to targets by thread_transfer
}

void thread_transfer(NrnThread* _nt) {
	if (!is_setup_) {
		hoc_execerror("ParallelContext.setup_transfer()", "needs to be called.");
	}
//	fprintf(xxxfile, "%g\n", t);
	// an edited old comment prior to allowing simultaneous threads and mpi.
		// for threads we do direct transfers under the assumption
		// that v is being transferred and they were set in a
		// previous multithread job. For the fixed step method this
		// call is from nonvint which in the same thread job as update
		// and that is the case even with multisplit. So we really
		// need to break the job between update and nonvint. Too bad.
		// For global cvode, things are ok except if the source voltage
		// is at a zero area node since nocap_v_part2 is a part
		// of this job and in fact the v does not get updated til
		// the next job in nocap_v_part3. Again, too bad. But it is
		// quite ambiguous, stability wise,
		// to have a gap junction in a zero area node, anyway, since
		// the system is then truly a DAE.
		// For now we presume we have dealt with these matters and
		// do the transfer.
	assert(n_transfer_thread_data_ == nrn_nthread);
	TransferThreadData& ttd = transfer_thread_data_[_nt->id];
	for (int i = 0; i < ttd.cnt; ++i) {
		*(ttd.tv[i]) = *(ttd.sv[i]);
	}
}

// The simplest conceivable transfer is to use MPI_Allgatherv and send
// all sources to all machines. More complicated and possibly more efficient
// in terms of total received buffer size
// would be to use MPI_Alltoallv in which distinct data is sent and received.
// Most transfer are one to one, at most one to a few, so now we use alltoallv.
// The old comment read: "
// We begin with MPI_Allgatherv. We try
// to save a trivial copy by making
// outgoing_source_buf a pointer into the incoming_source_buf.
// "  But this was a mistake as many mpi implementations do not allow overlap
// of send and receive buffers.

void nrnmpi_setup_transfer() {
#if !PARANEURON
	if (nrnmpi_numprocs > 1) {
		hoc_execerror("To use ParallelContext.setup_transfer when nhost > 1, NEURON must be configured with --with-paranrn", 0);
	}
#endif
//	char ctmp[100];
//	sprintf(ctmp, "vartrans%d", nrnmpi_myid);
//	xxxfile = fopen(ctmp, "w");
	alloclists();
	is_setup_ = true;
//	printf("nrnmpi_setup_transfer\n");
	if (insrc_buf_) { delete [] insrc_buf_; insrc_buf_ = 0; }
	if (outsrc_buf_) { delete [] outsrc_buf_; outsrc_buf_ = 0; }
	if (sid2insrc_) { delete sid2insrc_; sid2insrc_ = 0; }
#if PARANEURON
	// if there are no targets anywhere, we do not need to do anything
	if (nrnmpi_int_allmax(targets_->count()) == 0) {
		return;
	}
    if (nrnmpi_numprocs > 1) {
	if (!insrccnt_) {
		insrccnt_ = new int[nrnmpi_numprocs];
		insrcdspl_ = new int[nrnmpi_numprocs+1];
		outsrccnt_ = new int[nrnmpi_numprocs];
		outsrcdspl_ = new int[nrnmpi_numprocs+1];
	}

	// This machine needs to send which sources to which other machines.
	// It does not need to send to itself.
	// Which targets have sources on other machines.(none if nrnmpi_numprocs=1)
	// 1) list sources needed that are on other machines.
	// 2) send that info to all machines.
	// 3) source machine can figure out which machines want its sids
	//    and therefore construct outsrc_buf, etc.
	// 4) Notify target machines which sids the source machine will send
	// 5) The target machines can figure out where the sids are coming from
	//    and therefore construct insrc_buf, etc.

	// 1)
	// It will often be the case that multiple targets will need the
	// same source. We desire to count the needed interprocessor sids only
	// once. We do not want to count needed intraprocessor sids.
	// At the end of this section, needsrc is an array of needsrc_cnt
	// sids needed by this machine where the sources are on other machines.
	int needsrc_cnt = 0;
	MapInt2Int* seen = new MapInt2Int(targets_->count());//for single counting
	int* needsrc = new int[targets_->count()]; // more than we need
	for (int i = 0; i < sgid2targets_->count(); ++i) {
		int sid = sgid2targets_->item(i);
		// only need it if not a source on this machine
		int srcindex;
		if (!sgid2srcindex_->find(sid, srcindex)) {
			if (!seen->find(sid, srcindex)) {
				(*seen)[sid] = srcindex;
				needsrc[needsrc_cnt++] = sid;
			}
		}
	}
	delete seen;
#if 0
	for (int i=0; i < needsrc_cnt; ++i) {
		printf("%d step 1 need %d\n", nrnmpi_myid, needsrc[i]);
	}
#endif

	// 2)
	// reuse insrccnt to send the needsrc_cnt to all machines
	insrccnt_[nrnmpi_myid] = needsrc_cnt;
	int cnt = insrccnt_[nrnmpi_myid];
	nrnmpi_int_allgather(&cnt, insrccnt_, 1);
	// send the needsrc info to all machines
	insrcdspl_[0] = 0;
	for (int i=0; i < nrnmpi_numprocs; ++i) {
		insrcdspl_[i+1] = insrcdspl_[i] + insrccnt_[i];
	}
	int* need = new int[insrcdspl_[nrnmpi_numprocs]];
	nrnmpi_int_allgatherv(needsrc, need, insrccnt_, insrcdspl_);
#if 0
	nrnmpi_barrier();
	if (nrnmpi_myid == 0) {
		for (int i = 0; i < nrnmpi_numprocs; ++i) {
printf("%d step 2 need count %d from rank %d\n", nrnmpi_myid, insrccnt_[i], i);
			for (int j=0; j < insrccnt_[i]; ++j) {
printf("%d    interested in %d\n", nrnmpi_myid, need[insrcdspl_[i]+j]);
			}
		}
	}
	nrnmpi_barrier();
#endif

	// 3)
	// Now that we know what machines are interested in our sids...
	// construct outsrc_buf, outsrc_buf_size, outsrccnt_, outsrcdspl_
	// and poutsrc_;
	outsrcdspl_[0] = 0;
	for (int i=0; i < nrnmpi_numprocs; ++i) {
		outsrccnt_[i] = 0;
		for (int j = 0; j < insrccnt_[i]; ++j) {
			int sid = need[insrcdspl_[i] + j];
			int whocares;
			if (sgid2srcindex_->find(sid, whocares)) {
				++outsrccnt_[i];
			}
		}
		outsrcdspl_[i+1] = outsrcdspl_[i] + outsrccnt_[i];
	}
	outsrc_buf_size_ = outsrcdspl_[nrnmpi_numprocs];
	outsrc_buf_ = new double[outsrc_buf_size_];
	poutsrc_ = new double*[outsrc_buf_size_];
	for (int i=0; i < nrnmpi_numprocs; ++i) {
		int k = 0;
		for (int j = 0; j < insrccnt_[i]; ++j) {
			int sid = need[insrcdspl_[i] + j];
			int srcindex;
			if (sgid2srcindex_->find(sid, srcindex)) {
		    poutsrc_[outsrcdspl_[i] + k] = sources_->item(srcindex);
		    outsrc_buf_[outsrcdspl_[i] + k] = double(sid); // see step 5
		    ++k;
#if 0
printf("%d step 3 send sid %d to rank %d\n", nrnmpi_myid, sid, i);
#endif
			}
		}
	}
	delete [] need;

	//4)
	// tell the target machines what sids the source machines will be
	// sending.
	insrcdspl_[0] = 0;
	int* ones = new int[nrnmpi_numprocs];
	for (int i=0; i < nrnmpi_numprocs; ++i) {
		ones[i] = 1;
		insrcdspl_[i+1] = insrcdspl_[i] + ones[i];
	}
	nrnmpi_int_alltoallv(outsrccnt_, ones, insrcdspl_, insrccnt_, ones, insrcdspl_);
	insrcdspl_[0] = 0;
	for (int i = 0; i < nrnmpi_numprocs; ++i) {
		insrcdspl_[i+1] = insrcdspl_[i] + insrccnt_[i];
	}
	insrc_buf_size_ = insrcdspl_[nrnmpi_numprocs];
	insrc_buf_ = new double[insrc_buf_size_];
	delete [] ones;
#if 0
for (int i=0; i < nrnmpi_numprocs; ++i) {
printf("%d step 4  %d sids coming from rank %d\n", nrnmpi_myid, insrccnt_[i], i);
}
#endif

	//5)
	// send the sids sent by this machine so the target machines
	// can create transfer information (use double(sid) in outsrc_buf
	// that was setup in step 3.
	nrnmpi_dbl_alltoallv(outsrc_buf_, outsrccnt_, outsrcdspl_,
		insrc_buf_, insrccnt_, insrcdspl_);		
	errno = 0;
	// map sid to insrc_buf_ indices.
	// mk_ttd can then construct the right pointer to the source.
	sid2insrc_ = new MapInt2Int(insrc_buf_size_);
	for (int i=0; i < insrc_buf_size_; ++i) {
		int sid = int(insrc_buf_[i]);
		int whocares;
		assert(!sid2insrc_->find(sid, whocares));
		(*sid2insrc_)[sid] = i;
	}
	nrnmpi_v_transfer_ = mpi_transfer;
    }	
#endif //PARANEURON
	nrn_mk_transfer_thread_data_ = mk_ttd;
	if (!v_structure_change) {
		mk_ttd();
	}
}

void alloclists() {
	if (!targets_) {
		targets_ = new DblPList(100);
		target_pntlist_ = new PPList(100);
		sgid2targets_ = new IntList(100);
		sources_ = new DblPList(100);
		sgids_ = new IntList(100);
		sgid2srcindex_ = new MapInt2Int(256);
	}
}

void nrn_partrans_clear() {
	if (!targets_) { return; }
	nrnthread_v_transfer_ = 0;
	nrnmpi_v_transfer_ = 0;
	delete sgid2srcindex_;
	delete sgids_;
	delete sources_;
	delete sgid2targets_;
	delete targets_;
	delete target_pntlist_;
	targets_ = 0;
	rm_ttd();
	if (insrc_buf_) { delete [] insrc_buf_; insrc_buf_ = 0; }
	if (outsrc_buf_) { delete [] outsrc_buf_; outsrc_buf_ = 0; }
	if (sid2insrc_) { delete sid2insrc_; sid2insrc_ = 0; }
	nrn_mk_transfer_thread_data_ = 0;
}
