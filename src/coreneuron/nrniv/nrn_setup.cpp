/*
Copyright (c) 2014 EPFL-BBP, All rights reserved.

THIS SOFTWARE IS PROVIDED BY THE BLUE BRAIN PROJECT "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE BLUE BRAIN PROJECT
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "coreneuron/nrnconf.h"
#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/nrnoc/nrnoc_decl.h"
#include "coreneuron/nrniv/vrecitem.h"
#include "coreneuron/utils/randoms/nrnran123.h"
#include "coreneuron/utils/sdprintf.h"
#include "coreneuron/nrniv/nrn_datareader.h"
#include "coreneuron/nrniv/nrn_assert.h"
#include "coreneuron/nrniv/nrnmutdec.h"
#include "coreneuron/nrniv/memory.h"

// file format defined in cooperation with nrncore/src/nrniv/nrnbbcore_write.cpp
// single integers are ascii one per line. arrays are binary int or double
// Note that regardless of the gid contents of a group, since all gids are
// globally unique, a filename convention which involves the first gid
// from the group is adequate. Also note that balance is carried out from a
// per group perspective and launching a process consists of specifying
// a list of group ids (first gid of the group) for each process.
//
// <firstgid>_1.dat
// n_presyn, n_netcon
// output_gids (npresyn) with -(type+1000*index) for those acell with no gid
// netcon_srcgid (nnetcon) -(type+1000*index) refers to acell with no gid
//                         -1 means the netcon has no source (not implemented)
// Note that the negative gids are only thread unique and not process unique.
// We create a thread specific hash table for the negative gids for each thread
// when <firstgid>_1.dat is read and then destroy it after <firstgid>_2.dat
// is finished using it.  An earlier implementation which attempted to
// encode the thread number into the negative gid
// (i.e -ith - nth*(type +1000*index)) failed due to not large enough 
// integer domain size.
//
// <firstgid>_2.dat
// n_output n_real_output, nnode
// nmech - includes artcell mechanisms
// for the nmech tml mechanisms
//   type, nodecount
// ndata, nidata, nvdata, nweight
// v_parent_index (nnode)
// actual_a, b, area, v (nnode)
// for the nmech tml mechanisms
//   nodeindices (nodecount) but only if not an artificial cell
//   data (nodecount*param_size)
//   pdata (nodecount*dparam_size) but only if dparam_size > 0 on this side.
// output_vindex (n_presyn) >= 0 associated with voltages -(type+1000*index) for acell
// output_threshold (n_real_output)
// netcon_pnttype (nnetcon) <=0 if a NetCon does not have a target.
// netcon_pntindex (nnetcon)
// weights (nweight)
// delays (nnetcon)
// for the nmech tml mechanisms that have a nrn_bbcore_write method
//   type
//   icnt
//   dcnt
//   int array (number specified by the nodecount nrn_bbcore_write
//     to be intepreted by this side's nrn_bbcore_read method)
//   double array
// #VectorPlay_instances, for each of these instances
// 4 (VecPlayContinuousType)
// mtype
// index (from Memb_list.data)
// vecsize
// yvec
// tvec
//
// The critical issue requiring careful attention is that a coreneuron
// process reads many bluron thread files with a result that, although
// the conceptual
// total n_pre is the sum of all the n_presyn from each thread as is the
// total number of output_gid, the number of InputPreSyn instances must
// be computed here from a knowledge of all thread's netcon_srcgid after
// all thread's output_gids have been registered. We want to save the
// "individual allocation of many small objects" memory overhead by
// allocating a single InputPreSyn array for the entire process.
// For this reason cellgroup data are divided into two separate
// files with the first containing output_gids and netcon_srcgid which are
// stored in the nt.presyns array and nt.netcons array respectively


#if !defined(NRN_SOA_PAD)
// for layout 0, every range variable array must have a size which
// is a multiple of NRN_SOA_PAD doubles
#define NRN_SOA_PAD 1
#endif
#if !defined(NRN_SOA_BYTE_ALIGN)
// for layout 0, every range variable array must be aligned by at least 16 bytes (the size of the simd memory bus)
#define NRN_SOA_BYTE_ALIGN (2*sizeof(double))
#endif

static MUTDEC
static void read_phase1(data_reader &F,NrnThread& nt);
static void determine_inputpresyn(void);
static void read_phase2(data_reader &F, NrnThread& nt);
static void setup_ThreadData(NrnThread& nt);
static size_t model_size(void);

static int n_inputpresyn_;
static InputPreSyn* inputpresyn_; // the global array of instances.

// InputPreSyn.nc_index_ to + InputPreSyn.nc_cnt_ give the NetCon*
NetCon** netcon_in_presyn_order_;

// Wrap read_phase1 and read_phase2 calls to allow using  nrn_multithread_job.
// Args marshaled by store_phase_args are used by phase1_wrapper
// and phase2_wrapper.
static int ngroup_w;
static int* gidgroups_w;
static const char* path_w;
static data_reader* file_reader_w;
static bool byte_swap_w;
static void store_phase_args(int ngroup, int* gidgroups, data_reader* file_reader,
  const char* path, int byte_swap) {
  ngroup_w = ngroup;
  gidgroups_w = gidgroups;
  file_reader_w = file_reader;
  path_w = path;
  byte_swap_w = (bool) byte_swap;
}

#define implement_phase_wrapper(A) \
static void* phase##A##_wrapper_w(NrnThread* nt) { \
  int i = nt->id; \
  char fnamebuf[1000]; \
  if (i < ngroup_w) { \
    sd_ptr fname = sdprintf(fnamebuf, sizeof(fnamebuf), "%s/%d_"#A".dat", path_w, gidgroups_w[i]); \
    file_reader_w[i].open(fname,byte_swap_w); \
    read_phase##A(file_reader_w[i], *nt); \
    file_reader_w[i].close(); \
    if (A == 2) { \
      setup_ThreadData(*nt); \
    } \
  } \
  return NULL; \
} \
static void phase##A##_wrapper() { \
  nrn_multithread_job(phase##A##_wrapper_w); \
} \
// end of implement_phase_wrapper

implement_phase_wrapper(1)
implement_phase_wrapper(2)

/* read files.dat file and distribute cellgroups to all mpi ranks */
void nrn_read_filesdat(int &ngrp, int * &grp, const char *filesdat)
{
    FILE *fp = fopen( filesdat, "r" );
  
    if ( !fp ) {
      nrnmpi_fatal_error( "No input file with nrnthreads, exiting..." );
    }

    int iNumFiles;
    nrn_assert( fscanf( fp, "%d\n", &iNumFiles ) == 1 );

    if ( nrnmpi_numprocs > iNumFiles ) {
        nrnmpi_fatal_error( "The number of CPUs cannot exceed the number of input files" );
    }

    ngrp = 0;
    grp = new int[iNumFiles / nrnmpi_numprocs + 1];

    // irerate over gids in files.dat
    for ( int iNum = 0; iNum < iNumFiles; ++iNum ) {
        int iFile;

        nrn_assert( fscanf( fp, "%d\n", &iFile ) == 1 );
        if ( ( iNum % nrnmpi_numprocs ) == nrnmpi_myid ) {
            grp[ngrp] = iFile;
            ngrp++;
        }
    }

    fclose( fp );
}

void nrn_setup(const char *path, const char *filesdat, int byte_swap, int threading) {

  /// Number of local cell groups
  int ngroup = 0;

  /// Array of cell group numbers (indices)
  int *gidgroups = NULL;

  double time = nrnmpi_wtime(); 

  nrn_read_filesdat(ngroup, gidgroups, filesdat);

  assert(ngroup > 0);
  MUTCONSTRUCT(1)
  // temporary bug work around. If any process has multiple threads, no
  // process can have a single thread. So, for now, if one thread, make two.
  // Fortunately, empty threads work fine.
  /// Allocate NrnThread* nrn_threads of size ngroup (minimum 2)
  nrn_threads_create(ngroup == 1?2:ngroup, threading); // serial/parallel threads

  /// Reserve vector of maps of size ngroup for negative gid-s
  /// std::vector< std::map<int, PreSyn*> > neg_gid2out;
  netpar_tid_gid2ps_alloc(ngroup);

  // bug fix. gid2out is cumulative over all threads and so do not
  // know how many there are til after phase1
  // A process's complete set of output gids and allocation of each thread's
  // nt.presyns and nt.netcons arrays.
  // Generates the gid2out map which is needed
  // to later count the required number of InputPreSyn
  /// gid2out - map of output presyn-s
  /// std::map<int, PreSyn*> gid2out;
  nrn_reset_gid2out();

  data_reader *file_reader=new data_reader[ngroup];

  /* nrn_multithread_job supports serial, pthread, and openmp. */
  store_phase_args(ngroup, gidgroups, file_reader, path, byte_swap);
  phase1_wrapper();

  // from the netpar::gid2out map and the netcon_srcgid array,
  // fill the netpar::gid2in, and from the number of entries,
  // allocate the process wide InputPreSyn array
  determine_inputpresyn();


  // read the rest of the gidgroup's data and complete the setup for each
  // thread.
  /* nrn_multithread_job supports serial, pthread, and openmp. */
  phase2_wrapper();

  /// Generally, tables depend on a few parameters. And if those parameters change,
  /// then the table needs to be recomputed. This is obviously important in NEURON
  /// since the user can change those parameters at any time. However, there is no
  /// c example for CoreNEURON so can't see what it looks like in that context.
  /// Boils down to setting up a function pointer of the function _check_table_thread(),
  /// which is only executed by StochKV.c.
  nrn_mk_table_check(); // was done in nrn_thread_memblist_setup in multicore.c

  delete [] file_reader;

  netpar_tid_gid2ps_free();

  if (nrn_nthread > 1) {
    // NetCvode construction assumed one thread. Need nrn_nthread instances
    // of NetCvodeThreadData
    nrn_p_construct();
  }

  model_size();
  delete []gidgroups;

  if ( nrnmpi_myid == 0 ) {
	  printf( " Nrn Setup Done (time: %g)\n", nrnmpi_wtime() - time );
  }

}

void setup_ThreadData(NrnThread& nt) {
  for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
    Memb_func& mf = memb_func[tml->index];
    Memb_list* ml = tml->ml;
    if (mf.thread_size_) {
      ml->_thread = (ThreadDatum*)ecalloc(mf.thread_size_, sizeof(ThreadDatum));
      if (mf.thread_mem_init_) {
        MUTLOCK
        (*mf.thread_mem_init_)(ml->_thread);
        MUTUNLOCK
      }
    }
    else ml->_thread = NULL;
  }
}


void read_phase1(data_reader &F, NrnThread& nt) {
  nt.n_presyn = F.read_int(); /// Number of PreSyn-s in NrnThread nt
  nt.n_netcon = F.read_int(); /// Number of NetCon-s in NrnThread nt
  nt.presyns = new PreSyn[nt.n_presyn];
  nt.netcons = new NetCon[nt.n_netcon];

  /// Checkpoint in bluron is defined for both phase 1 and phase 2 since they are written together
  /// output_gid has all of output PreSyns, netcon_srcgid is created for NetCons, which might be
  /// 10k times more than output_gid.
  int* output_gid = F.read_int_array(nt.n_presyn);
  int* netcon_srcgid = F.read_int_array(nt.n_netcon);
  F.close();

  // for checking whether negative gids fit into the gid space
  // not used for now since negative gids no longer encode the thread id.
  double dmaxint = 1073741824.; //2^30
  for (;;) {
    if (dmaxint*2. == double(int(dmaxint*2.))) {
      dmaxint *= 2.;
    }else{
      if (dmaxint*2. - 1. == double(int(dmaxint*2. - 1.))) {
        dmaxint = 2.*dmaxint - 1.;
        break;
      }
    }
  }

  for (int i=0; i < nt.n_presyn; ++i) {
    int gid = output_gid[i];
    // Note that the negative (type, index)
    // coded information goes into the neg_gid2out[tid] hash table.
    // See netpar.cpp for the netpar_tid_... function implementations.
    // Both that table and the process wide gid2out table can be deleted
    // before the end of setup

    MUTLOCK
    /// Put gid into the gid2out hash table with correspondent output PreSyn
    netpar_tid_set_gid2node(nt.id, gid, nrnmpi_myid, nt.presyns + i);
    MUTUNLOCK

    if (gid < 0) {
      nt.presyns[i].output_index_ = -1;
    }
    nt.presyns[i].nt_ = &nt;
  }
  delete [] output_gid;
  // encode netcon_srcgid_ values in nt.netcons
  // which allows immediate freeing of that array.
  for (int i=0; i < nt.n_netcon; ++i) {
    nt.netcons[i].u.srcgid_ = netcon_srcgid[i];
    // do not need to worry about negative gid overlap since only use
    // it to search for PreSyn in this thread.
  }
  delete [] netcon_srcgid;
}

void determine_inputpresyn() {
  /// THIS WHOLE FUNCTION NEEDS SERIOUS OPTIMIZATION!
  // all the output_gid have been registered and associated with PreSyn.
  // now count the needed InputPreSyn by filling the netpar::gid2in map
  nrn_reset_gid2in();

  // now have to fill the new table
  int n_psi = 0;
  for (int ith = 0; ith < nrn_nthread; ++ith) {
    NrnThread& nt = nrn_threads[ith];
    for (int i = 0; i < nt.n_netcon; ++i) {
      int gid = nt.netcons[i].u.srcgid_;
      if (gid >= 0) {
        n_psi += input_gid_register(gid);
      }
    }
  }

  n_inputpresyn_ = n_psi;
  inputpresyn_ = new InputPreSyn[n_psi];

  // associate gid with InputPreSyn
  n_psi = 0;
  for (int ith = 0; ith < nrn_nthread; ++ith) {
    NrnThread& nt = nrn_threads[ith];
    for (int i = 0; i < nt.n_netcon; ++i) {
      int gid = nt.netcons[i].u.srcgid_;
      if (gid >= 0) {
        n_psi += input_gid_associate(gid, inputpresyn_ + n_psi);
      }
    }
  }

  // now, we can opportunistically create the NetCon* pointer array
  // to save some memory overhead for
  // "large number of small array allocation" by
  // counting the number of NetCons each PreSyn and InputPreSyn point to,
  // and instead of a NetCon** InputPreSyn.dil_, merely use a
  // int InputPreSyn.nc_index_ into the pointer array.
  // Conceivably the nt.netcons could become a process global array
  // in which case the NetCon* pointer array could become an integer index
  // array. More speculatively, the index array could be eliminated itself
  // if the process global NetCon array were ordered properly but that
  // would interleave NetCon from different threads. Not a problem for
  // serial threads but the reordering would propagate to nt.pntprocs
  // if the NetCon data pointers are also replaced by integer indices.

  // First, allocate the pointer array.
  int n_nc = 0;
  for (int ith = 0; ith < nrn_nthread; ++ith) {
    n_nc += nrn_threads[ith].n_netcon;
  }
  netcon_in_presyn_order_ = new NetCon*[n_nc];

  // count the NetCon each PreSyn and InputPresyn points to.
  for (int ith = 0; ith < nrn_nthread; ++ith) {
    NrnThread& nt = nrn_threads[ith];
    for (int i = 0; i < nt.n_netcon; ++i) {
      int gid = nt.netcons[i].u.srcgid_;
      PreSyn* ps; InputPreSyn* psi;
      netpar_tid_gid2ps(ith, gid, &ps, &psi);
      if (ps) {
        ++ps->nc_cnt_;
      }else if (psi) {
        ++psi->nc_cnt_;
      }
    }
  }
  // fill the indices with the offset values and reset the nc_cnt_
  int offset = 0;
  for (int ith = 0; ith < nrn_nthread; ++ith) {
    NrnThread& nt = nrn_threads[ith];
    for (int i = 0; i < nt.n_presyn; ++i) {
      PreSyn& ps = nt.presyns[i];
      ps.nc_index_ = offset;
      offset += ps.nc_cnt_;
      ps.nc_cnt_ = 0;
    }
  }
  for (int i=0; i < n_inputpresyn_; ++i) {
    InputPreSyn& psi = inputpresyn_[i];
    psi.nc_index_ = offset;
    offset += psi.nc_cnt_;
    psi.nc_cnt_ = 0;
  }
  // fill the netcon_in_presyn_order and recompute nc_cnt_
  // note that not all netcon_in_presyn will be filled if there are netcon
  // with no presyn (ie. nc->u.srcgid_ = -1) but that is ok since they are
  // only used via ps.nc_index_ and ps.nc_cnt_;
  for (int ith = 0; ith < nrn_nthread; ++ith) {
    NrnThread& nt = nrn_threads[ith];
    for (int i = 0; i < nt.n_netcon; ++i) {
      NetCon* nc = nt.netcons + i;
      int gid = nc->u.srcgid_;
      PreSyn* ps; InputPreSyn* psi;
      netpar_tid_gid2ps(ith, gid, &ps, &psi);
      if (ps) {
        netcon_in_presyn_order_[ps->nc_index_ + ps->nc_cnt_] = nc;
        nc->src_ = ps; // maybe nc->src_ is not needed
        ++ps->nc_cnt_;
      }else if (psi) {
        netcon_in_presyn_order_[psi->nc_index_ + psi->nc_cnt_] = nc;
        nc->src_ = psi; // maybe nc->src_ is not needed
        ++psi->nc_cnt_;
      }
    }
  }
}
int nrn_soa_padded_size(int cnt, int layout) {
  return coreneuron::soa_padded_size<NRN_SOA_PAD>(cnt, layout);
}

// file data is AoS. ie.
// organized as cnt array instances of mtype each of size sz.
// So input index i refers to i_instance*sz + i_item offset
// Return the corresponding SoA index -- taking into account the
// alignment requirements. Ie. i_instance + i_item*align_cnt.

int nrn_param_layout(int i, int mtype, Memb_list* ml) {
  int layout = nrn_mech_data_layout_[mtype];
  if (layout == 1) { return i; }
  assert(layout == 0);
  int sz = nrn_prop_param_size_[mtype];
  int cnt = ml->nodecount;
  int i_cnt = i / sz;
  int i_sz = i % sz;
  return nrn_i_layout(i_cnt, cnt, i_sz, sz, layout);
}

int nrn_i_layout(int icnt, int cnt, int isz, int sz, int layout) {
  if (layout == 1) {
    return icnt*sz + isz;
  }else if (layout == 0) {
    int padded_cnt = nrn_soa_padded_size(cnt, layout); // may want to factor out to save time
    return icnt + isz*padded_cnt;
  }
  assert(0);
  return 0;
}

#define MECHLAYOUT(TPE,TYPE) \
static void mech##TPE##layout(data_reader &F, TYPE* data, int cnt, int sz, int layout){\
  if (layout == 1) { /* AoS */\
    F.read##TPE##array(data, cnt*sz);\
  }else if (layout == 0) { /* SoA */\
    int align_cnt = nrn_soa_padded_size(cnt, layout);\
    TYPE* d = F.read##TPE##array(cnt*sz);\
    for (int i=0; i < cnt; ++i) {\
      for (int j=0; j < sz; ++j) {\
        data[i + j*align_cnt] = d[i*sz + j];\
      }\
    }\
    delete [] d;\
  }\
}

MECHLAYOUT(_dbl_,double) // mech_dbl_layout
MECHLAYOUT(_int_,int)   // mech_int_layout

/* nrn_threads_free() presumes all NrnThread and NrnThreadMembList data is
 * allocated with malloc(). This is not the case here, so let's try and fix
 * things up first. */

void nrn_cleanup() {

  nrnmpi_gid_clear();

  for (int it = 0; it < nrn_nthread; ++it) {
    NrnThread* nt = nrn_threads + it;
    NrnThreadMembList * next_tml = NULL;
    for (NrnThreadMembList *tml = nt->tml; tml; tml = next_tml) {
      Memb_list* ml = tml->ml;

      ml->data = NULL; // this was pointing into memory owned by nt
      delete[] ml->pdata;
      ml->pdata = NULL;
      delete[] ml->nodeindices;
      ml->nodeindices = NULL;

      next_tml = tml->next;
      free(tml->ml);
      free(tml);
    }

    nt->_actual_rhs = NULL;
    nt->_actual_d = NULL;
    nt->_actual_a = NULL;
    nt->_actual_b = NULL;


    delete[] nt->_v_parent_index;
    nt->_v_parent_index = NULL;

    free(nt->_data);
    nt->_data = NULL;

    free(nt->_idata);
    nt->_idata = NULL;

    free(nt->_vdata);
    nt->_vdata = NULL;

    if (nt->pntprocs) {
        delete[] nt->pntprocs;
        nt->pntprocs = NULL;
    }

    if (nt->presyns) {
        delete [] nt->presyns;
        nt->presyns = NULL;
    }

    if (nt->netcons) {
        delete [] nt->netcons;
        nt->netcons = NULL;
    }

    if (nt->weights) {
        delete [] nt->weights;
        nt->weights = NULL;
    }

    if (nt->_shadow_rhs) {
        free(nt->_shadow_rhs);
        nt->_shadow_rhs = NULL;
    }

    if (nt->_shadow_d) {
        free(nt->_shadow_d);
        nt->_shadow_d = NULL;
    }

    free(nt->_ml_list);
  }

  delete [] inputpresyn_;
  delete [] netcon_in_presyn_order_;

  nrn_threads_free();
}

void read_phase2(data_reader &F, NrnThread& nt) {
  NrnThreadMembList* tml;
  int n_outputgid = F.read_int();
  nrn_assert(n_outputgid > 0); // avoid n_outputgid unused warning
  nt.ncell = F.read_int();
  nt.end = F.read_int();
  int nmech = F.read_int();

  /// Checkpoint in bluron is defined for both phase 1 and phase 2 since they are written together
  //printf("ncell=%d end=%d nmech=%d\n", nt.ncell, nt.end, nmech);
  //printf("nart=%d\n", nart);
  NrnThreadMembList* tml_last = NULL;
  nt._ml_list = (Memb_list**)ecalloc(n_memb_func, sizeof(Memb_list*));
  int shadow_rhs_cnt = 0;
  for (int i=0; i < nmech; ++i) {
    tml = (NrnThreadMembList*)emalloc(sizeof(NrnThreadMembList));
    tml->ml = (Memb_list*)emalloc(sizeof(Memb_list));
    tml->next = NULL;
    tml->index = F.read_int();
    tml->ml->nodecount = F.read_int();;
    if (memb_func[tml->index].is_point && nrn_is_artificial_[tml->index] == 0){
      // Avoid race for multiple PointProcess instances in same compartment.
      if (tml->ml->nodecount > shadow_rhs_cnt) {
        shadow_rhs_cnt = tml->ml->nodecount;
      }
    }
    nt._ml_list[tml->index] = tml->ml;
    //printf("index=%d nodecount=%d membfunc=%s\n", tml->index, tml->ml->nodecount, memb_func[tml->index].sym?memb_func[tml->index].sym:"None");
    if (nt.tml) {
      tml_last->next = tml;
    }else{
      nt.tml = tml;
    }
    tml_last = tml;
  }

  if (shadow_rhs_cnt) {
    nt._shadow_rhs = (double*)coreneuron::ecalloc_align(nrn_soa_padded_size(shadow_rhs_cnt,0),
      NRN_SOA_BYTE_ALIGN, sizeof(double));
    nt._shadow_d = (double*)coreneuron::ecalloc_align(nrn_soa_padded_size(shadow_rhs_cnt,0),
      NRN_SOA_BYTE_ALIGN, sizeof(double));
  }

  nt._ndata = F.read_int();
  nt._nidata = F.read_int();
  nt._nvdata = F.read_int();
  nt.n_weight = F.read_int();

  nt._data = NULL; // allocated below after padding

  if (nt._nidata) nt._idata = (int*)ecalloc(nt._nidata, sizeof(int));
  else nt._idata = NULL;
  // see patternstim.cpp
  int zzz = (&nt == nrn_threads) ? nrn_extra_thread0_vdata : 0;
  if (nt._nvdata+zzz) 
    nt._vdata = (void**)ecalloc(nt._nvdata + zzz, sizeof(void*));
  else
    nt._vdata = NULL;
  //printf("_ndata=%d _nidata=%d _nvdata=%d\n", nt._ndata, nt._nidata, nt._nvdata);

  // The data format begins with the matrix data
  int ne = nrn_soa_padded_size(nt.end, 0);
  size_t offset = 6*ne;

  // Memb_list.data points into the nt.data array.
  // Also count the number of Point_process
  int npnt = 0;
  for (tml = nt.tml; tml; tml = tml->next) {
    Memb_list* ml = tml->ml;
    int type = tml->index;
    int layout = nrn_mech_data_layout_[type];
    offset = nrn_soa_padded_size(offset, layout);
    int n = ml->nodecount;
    int sz = nrn_prop_param_size_[type];
    ml->data = (double*)0 + offset; // adjust below since nt._data not allocated
    offset += nrn_soa_padded_size(n, layout)*sz;
    if (pnt_map[type] > 0) {
      npnt += n;
    }
  }
  nt.pntprocs = new Point_process[npnt]; // includes acell with and without gid
  nt.n_pntproc = npnt;
  //printf("offset=%ld ndata=%ld\n", offset, nt._ndata);
  // assert(offset == nt._ndata); // not with alignment
  nt._ndata = offset;

  // now that we know the effect of padding, we can allocate data space,
  // fill matrix, and adjust Memb_list data pointers
  nt._data = (double*)coreneuron::ecalloc_align(nt._ndata, NRN_SOA_BYTE_ALIGN, sizeof(double));
  nt._actual_rhs = nt._data + 0*ne;
  nt._actual_d = nt._data + 1*ne;
  nt._actual_a = nt._data + 2*ne;
  nt._actual_b = nt._data + 3*ne;
  nt._actual_v = nt._data + 4*ne;
  nt._actual_area = nt._data + 5*ne;
  for (tml= nt.tml; tml; tml = tml->next) {
    Memb_list* ml = tml->ml;
    ml->data = nt._data + (ml->data - (double*)0);
  }

  // matrix info
  nt._v_parent_index = F.read_int_array(nt.end);
  F.read_dbl_array(nt._actual_a, nt.end);
  F.read_dbl_array(nt._actual_b, nt.end);
  F.read_dbl_array(nt._actual_area, nt.end);
  F.read_dbl_array(nt._actual_v, nt.end);

  Memb_list** mlmap = new Memb_list*[n_memb_func];
  int synoffset = 0;
  int* pnt_offset = new int[n_memb_func];

  // All the mechanism data and pdata. 
  // Also fill in mlmap and pnt_offset
  // Complete spec of Point_process except for the acell presyn_ field.
  for (tml = nt.tml; tml; tml = tml->next) {
    int type = tml->index;
    Memb_list* ml = tml->ml;
    mlmap[type] = ml;
    int is_art = nrn_is_artificial_[type];
    int n = ml->nodecount;
    int szp = nrn_prop_param_size_[type];
    int szdp = nrn_prop_dparam_size_[type];

    if (!is_art) { 
        ml->nodeindices = F.read_int_array(ml->nodecount);
    } else { 
        ml->nodeindices = NULL;
    }
    
    int layout = nrn_mech_data_layout_[type];
    mech_dbl_layout(F, ml->data, n, szp, layout);
    
    if (szdp) {
      ml->pdata = new int[nrn_soa_padded_size(n, layout)*szdp];
      mech_int_layout(F, ml->pdata, n, szdp, layout);
    }else{
      ml->pdata = NULL;
    }
    if (pnt_map[type] > 0) { // POINT_PROCESS mechanism including acell
      int cnt = ml->nodecount;
      Point_process* pnt = NULL;
      pnt = nt.pntprocs + synoffset;
      pnt_offset[type] = synoffset;
      synoffset += cnt;
      for (int i=0; i < cnt; ++i) {
        Point_process* pp = pnt + i;
        pp->_type = type;
        pp->_i_instance = i;
        nt._vdata[ml->pdata[nrn_i_layout(i, cnt, 1, szdp, layout)]] = pp;
        pp->_presyn = NULL;
        pp->_tid = nt.id;
      }
    }
  }

  // Some pdata may index into data which has been reordered from AoS to
  // SoA. The two possibilities are if semantics is -5 (pointer)
  // or 0-999 (ion variables). Note that pdata has a layout and the
  // type block in nt.data into which it indexes, has a layout.
  for (tml = nt.tml; tml; tml = tml->next) {
    int type = tml->index;
    int layout = nrn_mech_data_layout_[type];
    int* pdata = tml->ml->pdata;
    int cnt = tml->ml->nodecount;
    int szdp = nrn_prop_dparam_size_[type];
    int* semantics = memb_func[type].dparam_semantics;

    if( szdp ) {
        if(!semantics) continue; // temporary for HDFReport, Binreport which will be skipped in bbcore_write of HBPNeuron
        nrn_assert(semantics);
    }


    for (int i=0; i < szdp; ++i) {
      int s = semantics[i];	
      if (s == -5) { //pointer
        nrn_assert(0); // not implemented yet
      }else if (s >=0 && s < 1000) { //ion
        int etype = s;
        int elayout = nrn_mech_data_layout_[etype];
        if (elayout == 1) { continue; } /* ion is AoS so nothing to do */
        assert(elayout == 0);
        /* ion is SoA so must recalculate pdata values */
        Memb_list* eml = mlmap[etype];
        int edata0 = eml->data - nt._data;
        int ecnt = eml->nodecount;
        int esz = nrn_prop_param_size_[etype];
        for (int iml=0; iml < cnt; ++iml) {
          int* pd = pdata;
          pd += nrn_i_layout(iml, cnt, i, szdp, layout);
          int ix = *pd - edata0;
          nrn_assert((ix >= 0) && (ix < ecnt*esz));
          /* Original pd order assumed ecnt groups of esz */
          *pd = edata0 + nrn_param_layout(ix, etype, eml);
        }
      }
    }
  }

  /// Fill the BA lists
  BAMech** bamap = new BAMech*[n_memb_func]; 
  for (int i=0; i < BEFORE_AFTER_SIZE; ++i) {
    BAMech* bam;
    NrnThreadBAList* tbl, **ptbl;
    for (int ii=0; ii < n_memb_func; ++ii) {
      bamap[ii] = (BAMech*)0;
    }
    for (bam = bamech_[i]; bam; bam = bam->next) {
      bamap[bam->type] = bam;
    }
    /* unnecessary but keep in order anyway */
    ptbl = nt.tbl + i;
    for (tml = nt.tml; tml; tml = tml->next) {
      if (bamap[tml->index]) {
        Memb_list* ml = tml->ml;
        tbl = (NrnThreadBAList*)emalloc(sizeof(NrnThreadBAList));
        tbl->next = (NrnThreadBAList*)0;
        tbl->bam = bamap[tml->index];
        tbl->ml = ml;
        *ptbl = tbl;
        ptbl = &(tbl->next);
      }
    }
  }
  delete [] bamap;

  // Real cells are at the beginning of the nt.presyns followed by
  // acells (with and without gids mixed together)
  // Here we associate the real cells with voltage pointers and
  // acell PreSyn with the Point_process.
  //nt.presyns order same as output_vindex order
  int* output_vindex = F.read_int_array(nt.n_presyn);
  double* output_threshold = F.read_dbl_array(nt.ncell);
  for (int i=0; i < nt.n_presyn; ++i) { // real cells
    PreSyn* ps = nt.presyns + i;
    int ix = output_vindex[i];
    if (ix < 0) {
      ix = -ix;
      int index = ix/1000;
      int type = ix - index*1000;
      Point_process* pnt = nt.pntprocs + (pnt_offset[type] + index);
      ps->pntsrc_ = pnt;
      pnt->_presyn = ps;
      if (ps->gid_ < 0) {
        ps->gid_ = -1;
      }
    }else{
      assert(ps->gid_ > -1);
      ps->thvar_ = nt._actual_v + ix;
      assert (ix < nt.end);
      ps->threshold_ = output_threshold[i];
    }
  }
  delete [] output_vindex;
  delete [] output_threshold;

  int nnetcon = nt.n_netcon;
  int nweight = nt.n_weight;
//printf("nnetcon=%d nweight=%d\n", nnetcon, nweight);
  // it may happen that Point_process structures will be made unnecessary
  // by factoring into NetCon.

  // Make NetCon.target_ point to proper Point_process. Only the NetCon
  // with pnttype[i] > 0 have a target.
  int* pnttype = F.read_int_array(nnetcon);
  int* pntindex = F.read_int_array(nnetcon);
  for (int i=0; i < nnetcon; ++i) {
    int type = pnttype[i];
    if (type > 0) {
      int index = pnt_offset[type] + pntindex[i]; /// Potentially uninitialized pnt_offset[], check for previous assignments
      Point_process* pnt = nt.pntprocs + index;
      NetCon& nc = nt.netcons[i];
      nc.target_ = pnt;
      nc.active_ = true;
    }
  }
  delete [] pntindex;
  delete [] pnt_offset;

  // weights in netcons order in groups defined by Point_process target type.
  nt.weights = F.read_dbl_array(nweight);
  int iw = 0;
  for (int i=0; i < nnetcon; ++i) {
    NetCon& nc = nt.netcons[i];
    nc.u.weight_ = nt.weights + iw;
    iw += pnt_receive_size[pnttype[i]];
  }
  assert(iw == nweight);
  delete [] pnttype;

  // delays in netcons order
  double* delay = F.read_dbl_array(nnetcon);
  for (int i=0; i < nnetcon; ++i) {
    NetCon& nc = nt.netcons[i];
    nc.delay_ = delay[i];
  }
  delete [] delay;

  // BBCOREPOINTER information
  npnt = F.read_int();
  for (int i=0; i < npnt; ++i) {
    int type = F.read_int();
    assert(nrn_bbcore_read_[type]);
    int icnt = F.read_int();
    int dcnt = F.read_int();
    int* iArray = NULL;
    double* dArray = NULL;
    if (icnt) 
    {
      iArray = F.read_int_array(icnt);
    }
    if (dcnt) 
    {
      dArray = F.read_dbl_array(dcnt);
    }
    int ik = 0;
    int dk = 0;
    Memb_list* ml = mlmap[type];
    int dsz = nrn_prop_param_size_[type];
    int pdsz = nrn_prop_dparam_size_[type];
    int cntml = ml->nodecount;
    int layout = nrn_mech_data_layout_[type];
    for (int j=0; j < cntml; ++j) {
      double* d = ml->data;
      Datum* pd = ml->pdata;
      d += nrn_i_layout(j, cntml, 0, dsz, layout);
      pd += nrn_i_layout(j, cntml, 0, pdsz, layout);
      int aln_cntml = nrn_soa_padded_size(cntml, layout);
      (*nrn_bbcore_read_[type])(dArray, iArray, &dk, &ik, 0, aln_cntml, d, pd, ml->_thread, &nt, 0.0);
    }
    assert(dk == dcnt);
    assert(ik == icnt);
    if (ik) 
    {
      delete [] iArray;
    }
    if (dk)
    {
      delete [] dArray;
    }
  }
  delete [] mlmap;

  // VecPlayContinuous instances
  // No attempt at memory efficiency
  int n = F.read_int();
  nt.n_vecplay = n;
  if (n) {
    nt._vecplay = new void*[n];
  }else{
    nt._vecplay = NULL;
  }
  for (int i=0; i < n; ++i) {
    int vtype = F.read_int();
    nrn_assert(vtype == VecPlayContinuousType);
    int mtype = F.read_int();
    Memb_list* ml = nt._ml_list[mtype];
    int ix = F.read_int();
    int sz = F.read_int();
    IvocVect* yvec = vector_new(sz);
    F.read_dbl_array(vector_vec(yvec), sz);
    IvocVect* tvec = vector_new(sz);
    F.read_dbl_array(vector_vec(tvec), sz);
    ix = nrn_param_layout(ix, mtype, ml);
    nt._vecplay[i] = new VecPlayContinuous(ml->data + ix, yvec, tvec, NULL, nt.id);
  }
}

static size_t memb_list_size(NrnThreadMembList* tml) {
  size_t sz_ntml = sizeof(NrnThreadMembList);
  size_t sz_ml = sizeof(Memb_list);
  size_t szi = sizeof(int);
  size_t nbyte = sz_ntml + sz_ml;
  nbyte += tml->ml->nodecount*szi;
  nbyte += nrn_prop_dparam_size_[tml->index]*tml->ml->nodecount*sizeof(Datum);
#ifdef DEBUG
  int i = tml->index;
  printf("%s %d psize=%d ppsize=%d cnt=%d nbyte=%ld\n", memb_func[i].sym, i, nrn_prop_param_size_[i], nrn_prop_dparam_size_[i], tml->ml->nodecount, nbyte);
#endif
  return nbyte;
}

size_t model_size(void) {
  size_t nbyte = 0;
  size_t szd = sizeof(double);
  size_t szi = sizeof(int);
  size_t szv = sizeof(void*);
  size_t sz_th = sizeof(NrnThread);
  size_t sz_ps = sizeof(PreSyn);
  size_t sz_pp = sizeof(Point_process);
  size_t sz_nc = sizeof(NetCon);
  size_t sz_psi = sizeof(InputPreSyn);
  NrnThreadMembList* tml;
  size_t nccnt = 0;

  for (int i=0; i < nrn_nthread; ++i) {
    NrnThread& nt = nrn_threads[i];
    size_t nb_nt = 0; // per thread
    nccnt += nt.n_netcon;

    // Memb_list size
    int nmech = 0;
    for (tml=nt.tml; tml; tml = tml->next) {
      nb_nt += memb_list_size(tml);
      ++nmech;
    }

    // basic thread size includes mechanism data and G*V=I matrix
    nb_nt += sz_th;
    nb_nt += nt._ndata*szd + nt._nidata*szi + nt._nvdata*szv;
    nb_nt += nt.end*szi; // _v_parent_index

#ifdef DEBUG
    printf("ncell=%d end=%d nmech=%d\n", nt.ncell, nt.end, nmech);
    printf("ndata=%ld nidata=%ld nvdata=%ld\n", nt._ndata, nt._nidata, nt._nvdata);
    printf("nbyte so far %ld\n", nb_nt);
    printf("n_presyn = %d sz=%ld nbyte=%ld\n", nt.n_presyn, sz_ps, nt.n_presyn*sz_ps);
    printf("n_pntproc=%d sz=%ld nbyte=%ld\n", nt.n_pntproc, sz_pp, nt.n_pntproc*sz_pp);
    printf("n_netcon=%d sz=%ld nbyte=%ld\n", nt.n_netcon, sz_nc, nt.n_netcon*sz_nc);
    printf("n_weight = %d\n", nt.n_weight);
#endif

    // spike handling
    nb_nt += nt.n_pntproc*sz_pp + nt.n_netcon*sz_nc + nt.n_presyn*sz_ps
             + nt.n_weight*szd;
    nbyte += nb_nt;
#ifdef DEBUG
    printf("%d thread %d total bytes %ld\n", nrnmpi_myid, i, nb_nt);
#endif
  }

#ifdef DEBUG
  printf("%d n_inputpresyn=%d sz=%ld nbyte=%ld\n", nrnmpi_myid, n_inputpresyn_, sz_psi, n_inputpresyn_*sz_psi);
  printf("%d netcon pointers %ld  nbyte=%ld\n", nrnmpi_myid, nccnt, nccnt*sizeof(NetCon*));
#endif
  nbyte += n_inputpresyn_*sz_psi + nccnt*sizeof(NetCon*);
  nbyte += output_presyn_size();
  nbyte += input_presyn_size();

#ifdef DEBUG
  printf("nrnran123 size=%ld cnt=%ld nbyte=%ld\n", nrnran123_state_size(), nrnran123_instance_count(), nrnran123_instance_count()*nrnran123_state_size());
#endif

  nbyte += nrnran123_instance_count() * nrnran123_state_size();

#ifdef DEBUG
  printf("%d total bytes %ld\n", nrnmpi_myid, nbyte);
#endif

  return nbyte;
}
