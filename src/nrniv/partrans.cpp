#include <../../nrnconf.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <InterViews/resource.h>
#include <OS/list.h>
#include <nrnoc2iv.h>
#include <nrniv_mf.h>
#include <nrnmpi.h>
#include <nrnhash.h>
#include <mymath.h>
#if defined(HAVE_STDINT_H)
#include <stdint.h>
#endif

// replaces use of qsort_r in sidsort for bbcorewrite
#include <vector>
#include <utility>
#include <algorithm>

#include <map> // Introduced for NonVSrcUpdateInfo

#ifndef NRNLONGSGID
#define NRNLONGSGID 0
#endif

#if NRNLONGSGID
#define sgid_t int64_t
#if PARANEURON
  extern void sgid_alltoallv(sgid_t* s, int* scnt, int* sdispl, sgid_t* r, int* rcnt, int* rdispl)
  {
    if (nrn_sparse_partrans > 0)
      {
	nrnmpi_long_alltoallv_sparse(s, scnt, sdispl, r, rcnt, rdispl);
      } else {
      nrnmpi_long_alltoallv(s, scnt, sdispl, r, rcnt, rdispl);
    }
  }
#endif //PARANEURON
#else // not NRNLONGSGID
#define sgid_t int
#if PARANEURON
  extern void sgid_alltoallv(sgid_t* s, int* scnt, int* sdispl, sgid_t* r, int* rcnt, int* rdispl)
  {
    if (nrn_sparse_partrans > 0)
      {
	nrnmpi_int_alltoallv_sparse(s, scnt, sdispl, r, rcnt, rdispl);
      } else {
      nrnmpi_int_alltoallv(s, scnt, sdispl, r, rcnt, rdispl);
    }
  }
#endif //PARANEURON
#endif // not NRNLONGSGID

void nrnmpi_source_var();
void nrnmpi_target_var();
void nrnmpi_setup_transfer();
void nrn_partrans_clear();
static void mpi_transfer();
static void thread_transfer(NrnThread*);
static void thread_vi_compute(NrnThread*);
static void mk_ttd();
extern double t;
extern "C" int v_structure_change;
extern "C" int structure_change_cnt;
extern int nrn_node_ptr_change_cnt_;
extern "C" double* nrn_recalc_ptr(double*);
extern const char *bbcore_write_version;
// see lengthy comment in ../nrnoc/fadvance.cpp
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

/*

16-5-2014
Gap junctions with extracellular require that the voltage source be v + vext.

A solution to the v+vext problem is to create a thread specific source
value buffer just for extracellular nodes.
 NODEV(_nd) + _nd->extnode->v[0] . 
 That is, if there is no extracellular the ttd.sv and and poutsrc_
pointers stay exactly the same.  Whereas, if there is extracellular,
those would point into the source value buffer of the correct thread. 
 Of course, it is necessary that the source value buffer be computed
prior to either parallel transfer or mpi_transfer.  Note that some
sources that are needed by another thread may not be needed by mpi and
vice versa.  For the fixed step method, mpi transfer occurs prior to
thread transfer.  For global variable step methods (cvode and lvardt do
not work with extracellular anyway):
 1) for multisplit, mpi between ms_part3 and ms_part4
 2) not multisplit, mpi between transfer_part1 and transfer_part2
 with thread transfer in ms_part4 and transfer_part2.
 Therefore it is possible to do the v+vext computation at the beginning
of mpi transfer except that mpi_transfer is not called if there is only
one process.  Also, this would be very cache inefficient for threads
since mpi transfer is done only by thread 0. 

Therefore we need yet another callback.
 void* nrnthread_vi_compute(NrnThread*)
 called, if needed, at the end of update(nt) and before mpi transfer in
nrn_finitialize. 

*/

/*

29-9-2016

Want to allow a source to be any range variable at a node or in a
Point_process.  e.g an ion concentration.  The v+vext change restricted
sources to voltage partly in order to simplify pointer recalculation for
cache efficiency.  If the source is not voltage, the calling context of
pc.source_var must be sufficient to easily store enough info to update
the pointer for cache efficiency or changed number of threads.  ie.  in
general, (Node*, mechtype, parray_index).  If the variable is a member
of a Point_process, it would be simple to pass it as the third arg and
store the reference.  However, we reject that due to not handling the
typical case of concentration.  So we impose the limitation that
structural changes that destroy the Node or move the pointer to another
Node generally require clearing and re-declaring the source, sid, target
relations, (nseg change, point process relocation).  Which is, in fact,
the current limitation on voltage sources.  The substantive
implementation change is to provide a safe pointer update for when we
know the Node*.  An extra map<sgid_t ssid, pair<int mechtype, int
parray_index> > should do nicely.  with voltages NOT being in the map. 
Also a decent error message can be generated.  Finally, it is fairly
clear how to make this work with coreneuron by perhaps adding a triple
of sid, mechtype, parray_index integer vectors to the <gidgroup>_gap.dat
file. 

*/

#if 1 || PARANEURON
extern void (*nrnthread_v_transfer_)(NrnThread*); // before nonvint and BEFORE INITIAL
extern void (*nrnthread_vi_compute_)(NrnThread*);
extern void (*nrnmpi_v_transfer_)(); // before nrnthread_v_transfer and after update. Called by thread 0.
extern void (*nrn_mk_transfer_thread_data_)();
#endif
#if PARANEURON
extern double nrnmpi_transfer_wait_;
extern void nrnmpi_barrier();
extern void nrnmpi_int_allgather(int*, int*, int);
extern int nrnmpi_int_allmax(int);
extern void sgid_alltoallv(sgid_t*, int*, int*, sgid_t*, int*, int*);
extern void nrnmpi_int_alltoallv(int*, int*, int*,  int*, int*, int*);
extern void nrnmpi_dbl_alltoallv(double*, int*, int*,  double*, int*, int*);
extern void nrnmpi_dbl_alltoallv_sparse(double*, int*, int*,  double*, int*, int*);
#endif

struct TransferThreadData {
	int cnt;
	double** tv; // pointers to the ParallelContext.target_var
	double** sv; // pointers to the ParallelContext.source_var (or into MPI target buffer)
};
static TransferThreadData* transfer_thread_data_;
static int n_transfer_thread_data_;

// for the case where we need vi = v + vext as the source voltage
struct SourceViBuf {
	int cnt;
	Node** nd;
	double* val;
};
static SourceViBuf* source_vi_buf_;
static int n_source_vi_buf_;

void nrn_partrans_update_ptrs();

declareNrnHash(MapSgid2Int, sgid_t, int);
implementNrnHash(MapSgid2Int, sgid_t, int);
declareNrnHash(MapNode2PDbl, Node*, double*);
implementNrnHash(MapNode2PDbl, Node*, double*);
declarePtrList(DblPList, double)
implementPtrList(DblPList, double)
declarePtrList(NodePList, Node)
implementPtrList(NodePList, Node)
#define PPList partrans_PPList
declarePtrList(PPList, Point_process)
implementPtrList(PPList, Point_process)
declareList(IntList, int)
implementList(IntList, int)
declareList(SgidList, sgid_t)
implementList(SgidList, sgid_t)
static double* insrc_buf_; // Receives the interprocessor data destined for other threads.
static double* outsrc_buf_;
static double** poutsrc_; // prior to mpi copy src value to proper place in outsrc_buf_
static int* poutsrc_indices_; // for recalc pointers
static int insrc_buf_size_, *insrccnt_, *insrcdspl_;
static int outsrc_buf_size_, *outsrccnt_, *outsrcdspl_;
static MapSgid2Int* sid2insrc_; // received interprocessor sid data is
// associated with which insrc_buf index. Created by nrnmpi_setup_transfer
// and used by mk_ttd
		
// ordered by calls to nrnmpi_target_var()
static DblPList* targets_; // list of target double*
static SgidList* sgid2targets_; // list of target sgid
static PPList* target_pntlist_;  // list of target Point_process
static IntList* target_parray_index_; // to recompute targets_ for cache_efficint

// ordered by calls to nrnmpi_source_var()
typedef std::vector<double*> DblPVec;
static NodePList* visources_; // list of source Node*, (multiples possible)
static SgidList* sgids_; // source gids
static MapSgid2Int* sgid2srcindex_; // sgid2srcindex[sgids[i]] == i

typedef std::map<sgid_t, std::pair<int, int> > NonVSrcUpdateInfo;
static NonVSrcUpdateInfo non_vsrc_update_info_; // source ssid -> (type,parray_index)


static int max_targets_;

static int target_ptr_update_cnt_ = 0;
static int target_ptr_need_update_cnt_ = 0;
static int vptr_change_cnt_ = 0;

static bool is_setup_;
static void alloclists();

// deleted when setup_transfer called
// defined persistently when pargap_jacobi_setup(0) called.
static int imped_current_type_count_;
static int* imped_current_type_;
static Memb_list** imped_current_ml_;

static void delete_imped_info() {
  if (imped_current_type_count_) {
    imped_current_type_count_ = 0;
    delete [] imped_current_type_;
    delete [] imped_current_ml_;
  }
}

// pv2node extended to any range variable in the section
// This helper searches over all the mechanisms in the node.
// If *pv exists, store mechtype and parray_index.
static bool non_vsrc_setinfo(sgid_t ssid, Node* nd, double* pv) {
  for (Prop* p = nd->prop; p; p = p->next) {
    if (pv >= p->param && pv < (p->param + p->param_size)) {
      non_vsrc_update_info_[ssid] = std::pair<int,int>(p->type, pv - p->param);
      //printf("non_vsrc_setinfo %p %d %ld %s\n", pv, p->type, pv-p->param, memb_func[p->type].sym->name);
      return true;
    }
  }
  return false;
}

static double* non_vsrc_update(Node* nd, int type, int ix) {
  for (Prop* p = nd->prop; p; p = p->next) {
    if (type == p->type) {
      return p->param + ix;
    }
  }
  char buf[100];
  sprintf(buf, "%d of %s", ix, memb_func[type].sym->name);
  hoc_execerror("partrans update: could not find parameter index", buf);
  return NULL;
}

// Find the Node associated with the voltage.
// Easy if v in the currently accessed section.
// Extended to any pointer to range variable in the section.
// If not a voltage save pv associated with mechtype, p_array_index
// in non_vsrc_update_info_
static Node* pv2node(sgid_t ssid, double* pv) {
	Section* sec = chk_access();
	Node* nd = sec->parentnode;
	if (nd) {
		if (&NODEV(nd) == pv || non_vsrc_setinfo(ssid, nd, pv)) {
			return nd;
		}
	}
	for (int i = 0; i < sec->nnode; ++i) {
		nd = sec->pnode[i];
		if (&NODEV(nd) == pv || non_vsrc_setinfo(ssid, nd, pv)) {
			return nd;
		}
	}
	
	hoc_execerror("Pointer to v is not in the currently accessed section", 0);
	return NULL;
}

void nrnmpi_source_var() {
	alloclists();
	is_setup_ = false;
	double* psv = hoc_pgetarg(1); // but might not be a voltage
	sgid_t sgid = (sgid_t)(*getarg(2));
	int i;
	if (sgid2srcindex_->find(sgid, i)) {
		char tmp[40];
		sprintf(tmp, "%lld", (long long)sgid);
		hoc_execerror("source var gid already in use:", tmp);
	}
	(*sgid2srcindex_)[sgid] = visources_->count();
	visources_->append(pv2node(sgid, psv));
	sgids_->append(sgid);
	//printf("nrnmpi_source_var %p source_val=%g sgid=%ld\n", psv, *psv, (long)sgid);
}

static int compute_parray_index(Point_process* pp, double* ptv) {
	if (!pp) {
		return -1;
	}
	long i =  ptv - pp->prop->param;
	if (i < 0 || i >= pp->prop->param_size) {
		i = -1;
	}
	return int(i);
}
static double* tar_ptr(Point_process* pp, int index) {
	return pp->prop->param + index;
}

static void check_pointers() {
//printf("check_pointers\n");
	for (int i = 0; i < targets_->count(); ++i) {
		double* pd = tar_ptr(target_pntlist_->item(i), target_parray_index_->item(i));
		assert(targets_->item(i) == pd);
	}
}

static void target_ptr_update() {
//printf("target_ptr_update\n");
	if (targets_) {
		int n = targets_->count();
		DblPList* newtar = new DblPList(n);
		for (int i=0; i < n; ++i) {
			double* pd = tar_ptr(target_pntlist_->item(i), target_parray_index_->item(i));
			newtar->append(pd);
		}
		delete targets_;
		targets_ = newtar;
	}
	mk_ttd();
	target_ptr_update_cnt_ = target_ptr_need_update_cnt_;
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
	sgid_t sgid = (sgid_t)(*getarg(iarg++));
	targets_->append(ptv);
	target_pntlist_->append(pp);
	target_parray_index_->append(compute_parray_index(pp, ptv));
	sgid2targets_->append(sgid);
	//printf("nrnmpi_target_var %p target_val=%g sgid=%ld\n", ptv, *ptv, (long)sgid);
}

void nrn_partrans_update_ptrs() {
	// These pointer changes require that the targets be range variables
	// of a point process and the sources be range variables
	int i, n;
	if (visources_) {
		n = visources_->count();
		// update the poutsrc that have no extracellular
		for (i=0; i < outsrc_buf_size_; ++i) {
		    int isrc = poutsrc_indices_[i];
		    Node* nd = visources_->item(long(isrc));
		    NonVSrcUpdateInfo::iterator it;
		    it = non_vsrc_update_info_.find(sgids_->item(isrc));
		    if (it != non_vsrc_update_info_.end()) {
			poutsrc_[i] = non_vsrc_update(nd, it->second.first, it->second.second);
		    }else if (!nd->extnode) {
			poutsrc_[i] = &(NODEV(nd));
		    }else{
			// pointers into SourceViBuf updated when
			// latter is (re-)created
		    }
		}
	}
	vptr_change_cnt_ = nrn_node_ptr_change_cnt_;
	// the target vgap pointers also need updating but they will not
	// change til after this returns ... (verify this)
	++target_ptr_need_update_cnt_;
}

//static FILE* xxxfile;

static void rm_ttd() {
	if (!transfer_thread_data_){ return; }
	for (int i=0; i < n_transfer_thread_data_; ++ i) {
		TransferThreadData& ttd = transfer_thread_data_[i];
		if (ttd.cnt) {
			delete [] ttd.tv;
			delete [] ttd.sv;
		}
	}
	delete [] transfer_thread_data_;
	transfer_thread_data_ = 0;
	n_transfer_thread_data_ = 0;
	nrnthread_v_transfer_ = 0;
}

static void rm_svibuf() {
	if (!source_vi_buf_){ return; }
	for (int i=0; i < n_source_vi_buf_; ++ i) {
		SourceViBuf& svib = source_vi_buf_[i];
		if (svib.cnt) {
			delete [] svib.nd;
			delete [] svib.val;
		}
	}
	delete [] source_vi_buf_;
	source_vi_buf_ = 0;
	n_source_vi_buf_ = 0;
	nrnthread_vi_compute_ = 0;
}

static MapNode2PDbl* mk_svibuf() {
	rm_svibuf();
	if (!visources_ || visources_->count() == 0) { return NULL; }
	// any use of extracellular?
	int has_ecell = 0;
	for (int tid = 0; tid < nrn_nthread; ++tid) {
		if (nrn_threads[tid]._ecell_memb_list) {
			has_ecell = 1;
			break;
		}
	}
	if (!has_ecell) { return NULL; }

	source_vi_buf_ = new SourceViBuf[nrn_nthread];
	n_source_vi_buf_ = nrn_nthread;
	NonVSrcUpdateInfo::iterator it;

	for (int tid = 0; tid < nrn_nthread; ++tid) {
		source_vi_buf_[tid].cnt = 0;
	}
	// count
	for (int i=0; i < visources_->count(); ++i) {
		Node* nd = visources_->item(i);
		it = non_vsrc_update_info_.find(sgids_->item(i));
		if (nd->extnode	&& it == non_vsrc_update_info_.end()) {
			assert(nd->_nt >= nrn_threads && nd->_nt < (nrn_threads + nrn_nthread));
			++source_vi_buf_[nd->_nt->id].cnt;
		}
	}
	// allocate
	for (int tid=0; tid < nrn_nthread; ++tid) {
		SourceViBuf& svib = source_vi_buf_[tid];
		if (svib.cnt) {
			svib.nd = new Node*[svib.cnt];
			svib.val = new double[svib.cnt];
		}
		svib.cnt = 0; // recount on fill
	}
	// fill
	for (int i=0; i < visources_->count(); ++i) {
		Node* nd = visources_->item(i);
		it = non_vsrc_update_info_.find(sgids_->item(i));
		if (nd->extnode	&& it == non_vsrc_update_info_.end()) {
			int tid = nd->_nt->id;			
			SourceViBuf& svib = source_vi_buf_[tid];
			svib.nd[svib.cnt] = nd;
			++svib.cnt;
		}
	}
	// now the only problem is how to get TransferThreadData and poutsrc_
	// to point to the proper SourceViBuf given that sgid2srcindex
	// only gives us the Node* and we dont want to search linearly
	// (during setup) everytime we we want to associate.
	// We can do the poutsrc_ now by creating a temporary Node* to
	// double* map .. The TransferThreadData can be done later
	// in mk_ttd using the same map and then deleted.
	MapNode2PDbl* ndvi2pd = new MapNode2PDbl(1000);
	for (int tid=0; tid < nrn_nthread; ++tid) {
		SourceViBuf& svib = source_vi_buf_[tid];
		for (int i = 0; i < svib.cnt; ++i) {
			Node* nd = svib.nd[i];
			(*ndvi2pd)[nd] = svib.val + i;
		}
	}
	for (int i=0; i < outsrc_buf_size_; ++i) {
		int isrc = poutsrc_indices_[i];
		Node* nd = visources_->item(isrc);
		double* pd = NULL;
		it = non_vsrc_update_info_.find(sgids_->item(isrc));
		if (nd->extnode	&& it == non_vsrc_update_info_.end()) {
			nrn_assert(ndvi2pd->find(nd, pd));
			poutsrc_[i] = pd;
		}
	}
	nrnthread_vi_compute_ = thread_vi_compute;
	return ndvi2pd;
}

static void mk_ttd() {
	int i, j, k, tid, n;
	MapNode2PDbl* ndvi2pd = mk_svibuf();
	rm_ttd();
	if (!targets_ || targets_->count() == 0) {
		if (ndvi2pd) { delete ndvi2pd; }
		// some MPI transfer code paths require that all ranks
		// have a nrn_thread_v_transfer.
		// As mentioned in http://static.msi.umn.edu/tutorial/scicomp/general/MPI/content3_new.html
		// "Communications may, or may not, be synchronized,
		// depending on how the vendor chose to implement them."
		// In particular the BG/Q (and one other machine) is sychronizing.
		// (but see: http://www-01.ibm.com/support/docview.wss?uid=isg1IZ58190 )
		if (nrnmpi_numprocs > 1 && max_targets_) {
			nrnthread_v_transfer_ = thread_transfer;
		}
		return;
	}
	n = targets_->count();
    if (nrn_nthread > 1) for (i=0; i < n; ++i) {
	Point_process* pp = target_pntlist_->item(i);
	int sgid = sgid2targets_->item(i);
	if (!pp) {
fprintf(stderr, "Do not know the POINT_PROCESS target for source id %lld\n", (long long)sgid);
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
		if (ttd.cnt) {
			ttd.tv = new double*[ttd.cnt];
			ttd.sv = new double*[ttd.cnt];
		}
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
		// if inter- or intra-thread, perhaps SourceViBuf
		sgid_t sid = sgid2targets_->item(i);
		if (sgid2srcindex_->find(sid, k)) {
			Node* nd = visources_->item(k);
			NonVSrcUpdateInfo::iterator it;
			it = non_vsrc_update_info_.find(sid);
			if (it != non_vsrc_update_info_.end()) {
				ttd.sv[j] = non_vsrc_update(nd, it->second.first, it->second.second);
			}else if (nd->extnode) {
				double* pd;
				nrn_assert(ndvi2pd->find(nd, pd));
				ttd.sv[j] = pd;
			}else{
				ttd.sv[j] = &(NODEV(nd));
			}
		}else if (sid2insrc_ && sid2insrc_->find(sid, k)) {
			ttd.sv[j] = insrc_buf_ + k;
		}else{
fprintf(stderr, "No source_var for target_var sid = %lld\n", (long long)sid);
			assert(0);
		}
	}
	if (ndvi2pd) { delete ndvi2pd; }
	nrnthread_v_transfer_ = thread_transfer;
}

void thread_vi_compute(NrnThread* _nt) {
	// vi+vext needed by either mpi or thread transfer copied into
	// the source value buffer for this thread. Note that relevant
	// poutsrc_ and ttd[_nt->id].sv items
	// point into this source value buffer
	if (!source_vi_buf_) { return; }
	SourceViBuf& svb = source_vi_buf_[_nt->id];
	for (int i = 0; i < svb.cnt; ++i) {
		Node* nd = svb.nd[i];
		assert(nd->extnode);
		svb.val[i] = NODEV(nd) + nd->extnode->v[0];
	}
}

void mpi_transfer() {
	int i, n = outsrc_buf_size_;
	if (nrn_node_ptr_change_cnt_ > vptr_change_cnt_) {
		nrn_partrans_update_ptrs();
	}
	for (i=0; i < n; ++i) {
		outsrc_buf_[i] = *poutsrc_[i];
	}
#if PARANEURON
	if (nrnmpi_numprocs > 1) {
		double wt = nrnmpi_wtime();
                if (nrn_sparse_partrans > 0)
                  {
                    nrnmpi_dbl_alltoallv_sparse(outsrc_buf_, outsrccnt_, outsrcdspl_,
                                                insrc_buf_, insrccnt_, insrcdspl_);
                  }
                else
                  {
                    nrnmpi_dbl_alltoallv(outsrc_buf_, outsrccnt_, outsrcdspl_,
                                         insrc_buf_, insrccnt_, insrcdspl_);
                  }
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
	if (!targets_ || targets_->count() == 0) {
		return;
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
	if (target_ptr_need_update_cnt_ > target_ptr_update_cnt_) {
		target_ptr_update();
	}
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

// 22-08-2014  For setup of the All2allv pattern, use the rendezvous rank
// idiom.
#define HAVEWANT_t sgid_t
#define HAVEWANT_alltoallv sgid_alltoallv
#define HAVEWANT2Int MapSgid2Int
#if PARANEURON
#include "have2want.cpp"
#endif

void nrnmpi_setup_transfer() {
#if !PARANEURON
	if (nrnmpi_numprocs > 1) {
		hoc_execerror("To use ParallelContext.setup_transfer when nhost > 1, NEURON must be configured with --with-paranrn", 0);
	}
#endif
	int nhost = nrnmpi_numprocs;
//	char ctmp[100];
//	sprintf(ctmp, "vartrans%d", nrnmpi_myid);
//	xxxfile = fopen(ctmp, "w");
	alloclists();
	is_setup_ = true;
//	printf("nrnmpi_setup_transfer\n");
	delete_imped_info();
	if (insrc_buf_) { delete [] insrc_buf_; insrc_buf_ = 0; }
	if (outsrc_buf_) { delete [] outsrc_buf_; outsrc_buf_ = 0; }
	if (sid2insrc_) { delete sid2insrc_; sid2insrc_ = 0; }
	if (poutsrc_) { delete [] poutsrc_ ; poutsrc_ = 0; }
	if (poutsrc_indices_) { delete [] poutsrc_indices_ ; poutsrc_indices_ = 0; }
#if PARANEURON
	// if there are no targets anywhere, we do not need to do anything
	max_targets_ = nrnmpi_int_allmax(targets_->count());
	if (max_targets_ == 0) {
		return;
	}
    if (nrnmpi_numprocs > 1) {
        if (insrccnt_) { delete [] insrccnt_; insrccnt_ = NULL; }
        if (insrcdspl_) { delete [] insrcdspl_; insrcdspl_ = NULL; }
        if (outsrccnt_) { delete [] outsrccnt_; outsrccnt_ = NULL; }
        if (outsrcdspl_) { delete [] outsrcdspl_; outsrcdspl_ = NULL; }

	// This is an old comment prior to using the want_to_have rendezvous
	// rank function in want2have.cpp. The old method did not scale
	// to more sgids than could fit on a single rank, because
	// each rank sent its "need" list to every rank.
	// <old comment>
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
	// <new comment>
	// 1) List sources needed by this rank and sources that this rank owns.
	// 2) Call the have_to_want function. Returns two sets of three
	//    vectors. The first set of three vectors is a an sgid buffer,
	//    along with counts and displacements. The sgids in the ith region
	//    of the buffer are the sgids from this rank that are
	//    wanted by the ith rank. For the second set, the sgids in the ith
	//    region are the sgids on the ith rank that are wanted by this rank.
	// 3) First return triple creates the proper outsrc_buf_.
	// 4) The second triple is creates the insrc_buf_.

	// 1)
	// It will often be the case that multiple targets will need the
	// same source. We count the needed sids only once regardless of
	// how often they are used.
	// At the end of this section, needsrc is an array of needsrc_cnt
	// sids needed by this machine. The 'seen' table values are unused
	// but the keys are all the (unique) sgid needed by this process.
	int needsrc_cnt = 0;
	MapSgid2Int* seen = new MapSgid2Int(targets_->count());//for single counting
	int szalloc = targets_->count();
	szalloc = szalloc ? szalloc : 1;
	sgid_t* needsrc = new sgid_t[szalloc]; // more than we need
	for (int i = 0; i < sgid2targets_->count(); ++i) {
		sgid_t sid = sgid2targets_->item(i);
		// only need it if not a source on this machine
		int srcindex;
// Note that although old comment possibly mention that we do not transfer
// intraprocessor sids, it is actually a good idea to do so in order to
// produce a reasonable error message about using the same sid for multiple
// source variables. Hence the following statement is commented out.
//		if (!sgid2srcindex_->find(sid, srcindex)) {
			if (!seen->find(sid, srcindex)) {
				(*seen)[sid] = srcindex;
				needsrc[needsrc_cnt++] = sid;
			}
//		}
	}
#if 0
	for (int i=0; i < needsrc_cnt; ++i) {
		printf("%d step 1 need %d\n", nrnmpi_myid, needsrc[i]);
	}
#endif

	// 1 continued) Create an array of sources this rank owns.
	// This already exists as a vector in the SgidList* sgids_ but
	// that is private so go ahead and copy.
	sgid_t* ownsrc = new sgid_t[sgids_->count() + 1]; // not 0 length if count is 0
	for (int i=0; i < sgids_->count(); ++i) {
		ownsrc[i] = sgids_->item(i);
	}
	
	// 2) Call the have_to_want function.
	sgid_t* send_to_want;
	int *send_to_want_cnt, *send_to_want_displ;
	sgid_t* recv_from_have;
	int *recv_from_have_cnt, *recv_from_have_displ;

	have_to_want(ownsrc, sgids_->count(), needsrc, needsrc_cnt,
		send_to_want, send_to_want_cnt, send_to_want_displ,
		recv_from_have, recv_from_have_cnt, recv_from_have_displ,
		default_rendezvous);

	// sanity check. all the sgids we are asked to send, we actually have
	int n = send_to_want_displ[nhost];
#if 0 // done in passing in step 3 below
	for (int i=0; i < n; ++i) {
		sgid_t sgid = send_to_want[i];
		int x;
		nrn_assert(sgid2srcindex_->find(sgid, x));
	}
#endif
	// sanity check. all the sgids we receive, we actually need.
	// also set the seen value to the proper recv_from_have index.
	// i.e it is the sid2insrc_ in step 4.
	n = recv_from_have_displ[nhost];
	for (int i=0; i < n; ++i) {
		sgid_t sgid = recv_from_have[i];
		int x;
		nrn_assert(seen->find(sgid, x));
		(*seen)[sgid] = i;
	}

	// clean up a little
	delete [] ownsrc;
	delete [] needsrc;
	delete [] recv_from_have;

	// 3) First return triple creates the proper outsrc_buf_.
	// Now that we know what machines are interested in our sids...
	// construct outsrc_buf, outsrc_buf_size, outsrccnt_, outsrcdspl_
	// and poutsrc_;
	outsrccnt_ = send_to_want_cnt;
	outsrcdspl_ = send_to_want_displ;
	outsrc_buf_size_ = outsrcdspl_[nrnmpi_numprocs];
	szalloc = outsrc_buf_size_ ? outsrc_buf_size_ : 1;
	outsrc_buf_ = new double[szalloc];
	poutsrc_ = new double*[szalloc];
	poutsrc_indices_ = new int[szalloc];
	for (int i=0; i < outsrc_buf_size_; ++i) {
		sgid_t sid = send_to_want[i];
		int srcindex;
		nrn_assert(sgid2srcindex_->find(sid, srcindex));
		Node* nd = visources_->item(long(srcindex));
		NonVSrcUpdateInfo::iterator it;
		it = non_vsrc_update_info_.find(sid);
		if (it != non_vsrc_update_info_.end()) {
			poutsrc_[i] = non_vsrc_update(nd, it->second.first, it->second.second);
		}else if (!nd->extnode) {
			poutsrc_[i] = &(NODEV(nd));
		}else{
			// the v+vext case can only be done after mk_svib()
		}
		poutsrc_indices_[i] = srcindex;
		outsrc_buf_[i] = double(sid); // see step 5
	}
	delete [] send_to_want;

	// 4) The second triple is creates the insrc_buf_.
	// From the recv_from_have and seen table, construct the insrc...
	insrccnt_ = recv_from_have_cnt;
	insrcdspl_ = recv_from_have_displ;
	insrc_buf_size_ = insrcdspl_[nrnmpi_numprocs];
	szalloc = insrc_buf_size_ ? insrc_buf_size_ : 1;
	insrc_buf_ = new double[szalloc];

	// map sid to insrc_buf_ indices.
	// mk_ttd can then construct the right pointer to the source.
	sid2insrc_ = seen; // since seen was constructed and then modified
		// way above this. Might be better to reconstruct here.

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
		target_parray_index_ = new IntList(100);
		sgid2targets_ = new SgidList(100);
		visources_ = new NodePList(100);
		sgids_ = new SgidList(100);
		sgid2srcindex_ = new MapSgid2Int(256);
	}
}

void nrn_partrans_clear() {
	if (!targets_) { return; }
	nrnthread_v_transfer_ = 0;
	nrnthread_vi_compute_ = 0;
	nrnmpi_v_transfer_ = 0;
	delete sgid2srcindex_;
	delete sgids_;
	delete visources_;
	delete sgid2targets_;
	delete targets_;
	delete target_pntlist_;
	delete target_parray_index_;
	targets_ = 0;
	max_targets_ = 0;
	rm_svibuf();
	rm_ttd();
	if (insrc_buf_) { delete [] insrc_buf_; insrc_buf_ = 0; }
	if (outsrc_buf_) { delete [] outsrc_buf_; outsrc_buf_ = 0; }
	if (sid2insrc_) { delete sid2insrc_; sid2insrc_ = 0; }
	if (poutsrc_) { delete [] poutsrc_ ; poutsrc_ = 0; }
	if (poutsrc_indices_) { delete [] poutsrc_indices_ ; poutsrc_indices_ = 0; }
	non_vsrc_update_info_.clear();
	nrn_mk_transfer_thread_data_ = 0;
}

// assume one thread and no extracellular

static double *vgap1, *vgap2;
static int imped_change_cnt;

void pargap_jacobi_setup(int mode) {
  if (!nrnthread_v_transfer_) { return; }

  // list of gap junction types and memb_list for each
 if (mode == 0) {
  if (imped_change_cnt != structure_change_cnt) {
    delete_imped_info();
    imped_change_cnt = structure_change_cnt;
  }
  if (imped_current_type_count_ == 0 && targets_ && targets_->count() > 0) {
    for (int i=0; i < targets_->count(); ++i) {
      Point_process* pp = target_pntlist_->item(i);
      if (!pp) {
        hoc_execerror("For impedance, pc.target_var requires that its first arg be a reference to the POINT_PROCESS", 0);
      }
      int type = pp->prop->type;
      if (imped_current_type_count_ == 0) {
        imped_current_type_count_ = 1;
        imped_current_type_ = new int[5];
        imped_current_ml_ = new Memb_list*[5];
        imped_current_type_[0] = type;
      }
      int add = 1;
      for (int k=0; k < imped_current_type_count_; ++k) {
        if (type == imped_current_type_[k]) {
          add = 0;
          break;
        }
      }
      if (add) {
        assert(imped_current_type_count_ < 5);
        imped_current_type_[imped_current_type_count_] = type;
        imped_current_type_count_ += 1;
      }
    }
    NrnThread* nt = nrn_threads;
    for (int k=0; k < imped_current_type_count_; ++k) {
      for (NrnThreadMembList* tml = nt->tml; tml; tml = tml->next) {
        if (imped_current_type_[k] == tml->index) {
          imped_current_ml_[k] = tml->ml;
        }
      }
    }
    // are all the instances in use
    int ninst = 0;
    for (int k=0; k < imped_current_type_count_; ++k) {
      ninst += imped_current_ml_[k]->nodecount;
    }
    if (ninst != targets_->count()) {
      char buf[100];
      sprintf(buf, "that is %d != %ld", ninst, targets_->count());
      hoc_execerror("number of gap junctions not equal to number of pc.transfer_var", buf);
    }
  }
 }
  if (target_ptr_need_update_cnt_ > target_ptr_update_cnt_) { 
    target_ptr_update();
  }
  TransferThreadData* ttd = transfer_thread_data_;
  if (mode == 0) { // setup
    if (visources_->count()) {vgap1 = new double[visources_->count()];}
    if (ttd && ttd->cnt) {vgap2 = new double[ttd->cnt];}
    for (int i=0; i < visources_->count(); ++i) {
      vgap1[i] = NODEV(visources_->item(i));
    }
    if (ttd) for (int i=0; i < ttd->cnt; ++i) {
      vgap2[i] = *(ttd->tv[i]);
    }
  }else{ // tear down
    for (int i=0; i < visources_->count(); ++i) {
      NODEV(visources_->item(i)) = vgap1[i];
    }
    if (ttd) for (int i=0; i < ttd->cnt; ++i) {
      *(ttd->tv[i]) = vgap2[i];
    }
    if (vgap1) { delete [] vgap1;  vgap1 = NULL; }
    if (vgap2) { delete [] vgap2;  vgap2 = NULL; }
  }
}

void pargap_jacobi_rhs(double* b, double* x) {
  // helper for complex impedance with parallel gap junctions
  // b = b - R*x  R are the off diagonal gap elements of the jacobian.
  // we presume 1 thread. First nrn_thread[0].end equations are in node order.
  if (!nrnthread_v_transfer_) { return; }

  NrnThread* _nt = nrn_threads;

  // transfer gap node voltages to gap vpre
  for (int i=0; i < visources_->count(); ++i) {
    Node* nd = visources_->item(i);
    NODEV(nd) = x[nd->v_node_index];
  }  
  mpi_transfer();
  thread_transfer(_nt);

  // set gap node voltages to 0 so we can use nrn_cur to set rhs
  for (int i=0; i < visources_->count(); ++i) {
    Node* nd = visources_->item(i);
    NODEV(nd) = 0.0;
  }
  // Initialize rhs to 0.
  for (int i=0; i < _nt->end; ++i) {
    VEC_RHS(i) = 0.0;
  }

  for (int k=0; k < imped_current_type_count_; ++k) {
    int type = imped_current_type_[k];
    Memb_list* ml = imped_current_ml_[k];
    (*memb_func[type].current)(_nt, ml, type);
  }

  // possibly many gap junctions in same node (and possible even different
  // types) but rhs is the accumulation of all those instances at each node
  // so ...  The only thing that can go wrong is if there are intances of
  // gap junctions that are not being used  (not in the target list).
  for (int i=0; i < _nt->end; ++i) {
    b[i] += VEC_RHS(i);
  }
}

extern size_t nrnbbcore_gap_write(const char* path, int* group_ids);

/*
  file format for <path>/<group_id>_gap.dat
  All gap info for thread. No file if ntar == 0
  ntar  // number of targets in this thread (vpre)
  nsrc  // number of sources in this thread (v)
  type // gap mechanism
  ix_vpre // range variable index relative to beginning of instance
  sid_target  // ntar = memb_list[type].nodecount of these lines 
    //there are no indices for target vpre since sorted in memb_list order.
  sid_src // nsrc of these (sorted in increasing order of index)
  <index into _actual_v> // nsrc of these

  Assert all gaps have the same type.
  Assert ntar = memb_list[type].nodecount.
  Assert no extracellular.
*/

/*
  The original file creation for each thread was accomplished by
  a serial function that:
  Verified the assertion constraints.
  Created an nthread array of BBCoreGapInfo.
  Wrote the <gid>_gap.dat files forall the threads.
  Cleaned up the BBCoreGapInfo (and gap_ml).

  So a simple factoring of the verify and create portions suffices
  for both files and direct. Note that direct calls back for each thread
  in index order. So for, tid 0, verify and create will be accomplished,
  and for tid nthread-1, cleanup of BBCoreGapInfo will take place (the
  contents of each thread are passed back and cleaned up in coreneuron
  after use there.
*/

struct BBCoreGapInfo {
  int nsrc, ntar;
  int* sid_src;
  int* v_index;
  int* sid_target;
  int* vpre_index;

  Memb_list* gap_ml;

  // until CoreNEURON gap up to date with respect to NEURON
  int type; // only one type allowed for half gap.
  int ix_vpre; // data offset within instance of a half gap.
};

static BBCoreGapInfo* gi; // array of size nthread

static void nrnbbcore_gap_info();
static void sidsort(int* sids, int cnt, int* indices);

extern "C" {
void get_partrans_setup_info(int tid, int& ntar, int& nsrc,
int& type, int& ix_vpre, int*& sid_target, int*& sid_src, int*& v_indices);
} // extern "C"

void get_partrans_setup_info(int tid, int& ntar, int& nsrc,
  int& type, int& ix_vpre, int*& sid_target, int*& sid_src, int*& v_indices) {

  if (tid == 0) { // first call
    nrnbbcore_gap_info();
  }
  assert(gi);

  BBCoreGapInfo& g = gi[tid];
  ntar = g.ntar;
  nsrc = g.nsrc;
  type = gi[0].type;
  ix_vpre = gi[0].ix_vpre;
  sid_target = g.sid_target;
  sid_src = g.sid_src;
  v_indices = g.v_index;

  if (tid == nrn_nthread-1) {
    delete [] gi;
    gi = NULL;
  }
}

size_t nrnbbcore_gap_write(const char* path, int* group_ids) {
  nrnbbcore_gap_info();
  if (gi == NULL) { return 0; }

  // print the files
  for (int tid = 0; tid < nrn_nthread; ++tid) {
    BBCoreGapInfo& g = gi[tid];
    if (g.ntar == 0) { continue; }

    assert(g.nsrc > 0);
    int type = g.type;
    int ix_vpre = g.ix_vpre;

    char fname[1000];
    sprintf(fname, "%s/%d_gap.dat", path, group_ids[tid]);
    FILE* f = fopen(fname, "wb");
    assert(f);
    fprintf(f, "%s\n", bbcore_write_version);
    fprintf(f, "%d ntar\n", g.ntar);
    fprintf(f, "%d nsrc\n", g.nsrc);
    fprintf(f, "%d %s\n", type, memb_func[type].sym->name);
    fprintf(f, "%d ix_vpre\n", ix_vpre); // range variable offset from instance

    int chkpnt = 0;
#define CHKPNT fprintf(f, "chkpnt %d\n", chkpnt++);
    CHKPNT fwrite(g.sid_target, g.ntar, sizeof(int), f);
    CHKPNT fwrite(g.sid_src, g.nsrc, sizeof(int), f);
    CHKPNT fwrite(g.v_index, g.nsrc, sizeof(int), f);

    fclose(f);
  }

  // cleanup
  for (int tid=0; tid < nrn_nthread; ++tid) {
    BBCoreGapInfo& g = gi[tid];
    delete [] g.sid_src;
    delete [] g.sid_target;
    delete [] g.v_index;
    delete [] g.vpre_index;
  }
  delete [] gi;
  gi = NULL;
  return 0;
}

static void nrnbbcore_gap_info() {
  if (non_vsrc_update_info_.size() > 0) {
    hoc_execerror("Non Voltage transfer sources not supported in CoreNEURON", NULL);
  }
  // if there are no targets, then there are no sources
  if (!targets_ || !targets_->count()) {
    assert(sgids_->count() == 0);
    return;
  }

  // type
  assert(target_pntlist_ && target_pntlist_->count() == targets_->count());
  int type = target_pntlist_->item(0)->prop->type;
  // all the same type
  for (int i=0; i < targets_->count(); ++i) {
    assert(type == target_pntlist_->item(i)->prop->type);
  }

  int ix_vpre = target_parray_index_->item(0);

  // space for the info
  gi = new BBCoreGapInfo[nrn_nthread];
  for (int tid = 0; tid < nrn_nthread; ++tid) {
    BBCoreGapInfo& g = gi[tid];
    int n = transfer_thread_data_[tid].cnt;
    g.sid_src = new int[n]; // but perhaps fewer sources than targets
    g.v_index = new int[n]; // fewer sources than targets
    g.sid_target = new int[n];
    g.vpre_index = new int[n];
    g.ntar = 0; // will end up as gap_ml[tid].nodecount
    g.nsrc = 0; // will end up as number of distinct gap_ml[tid].nodeindices

    g.type = type;
    g.ix_vpre = ix_vpre;
  }

  // memb_lists of the threads.
  for (int tid=0; tid < nrn_nthread; ++tid) {
    NrnThread& nt = nrn_threads[tid];
    for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
      if (tml->index == type) {
        gi[tid].gap_ml = tml->ml;
      }
    }
  }

  // gap info for targets, segregate into threads
  for (int i=0; i < targets_->count(); ++i) {
    int sid = sgid2targets_->item(i);
    NrnThread* nt = (NrnThread*)target_pntlist_->item(i)->_vnt;
    int tid = nt ? nt->id : 0;
    Memb_list* ml = gi[tid].gap_ml;
    int ix = targets_->item(i) - ml->data[0];

    BBCoreGapInfo& g = gi[tid];
    g.sid_target[g.ntar] = sid;
    g.vpre_index[g.ntar] = ix;
    g.ntar += 1;
  }

  // gap info for sources, segregate into threads.
  for (int i=0; i < sgids_->count(); ++i) {
    int sid = sgids_->item(i);
    Node* nd = visources_->item(i);
    assert(nd->extnode == NULL);
    int tid = nd->_nt ? nd->_nt->id : 0;
    int ix = nd->_v - nrn_threads[tid]._actual_v;
    assert(ix >= 0 && ix < nrn_threads[tid].end);
    BBCoreGapInfo& g = gi[tid];
    g.sid_src[g.nsrc] = sid;
    g.v_index[g.nsrc] = ix;
    g.nsrc += 1;
  }

  // Sort each threads sources and targets so that the indices are
  // monotonically increasing. (In fact the target indices will then be
  // in memb_list order and that array will not have to be transferred.)
  for (int tid=0; tid < nrn_nthread; ++tid) {
    BBCoreGapInfo& g = gi[tid];
    sidsort(g.sid_src, g.nsrc, g.v_index);
    sidsort(g.sid_target, g.ntar, g.vpre_index);
  }
}

static bool mycompar(const std::pair<int, int>& a, const std::pair<int, int>& b) {
  return a.second < b.second;
}

static void sidsort(int* sids, int cnt, int* indices) {
  std::vector< std::pair< int, int> > si(cnt); // sids,indices
  for (int i=0; i < cnt; ++i) {
    si[i].first = sids[i];
    si[i].second = indices[i];
  }

  std::sort(si.begin(), si.end(), mycompar);

  // copy back
  for (int i=0; i < cnt; ++i) {
    sids[i] = si[i].first;
    indices[i] = si[i].second;
  }

  // check that the sort is correct
  for (int i=1; i < cnt; ++i) {
    assert(indices[i-1] <= indices[i]);
  }
}

