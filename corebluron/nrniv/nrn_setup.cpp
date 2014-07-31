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

#include "corebluron/nrnconf.h"
#include "corebluron/nrnoc/multicore.h"
#include "corebluron/nrniv/nrniv_decl.h"
#include "corebluron/nrnoc/nrnoc_decl.h"
#include "corebluron/nrniv/vrecitem.h"
#include "corebluron/utils/randoms/nrnran123.h"

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
// Therefore BBS_gid2ps(gid) cannot be used to determine the PreSyn for the
// negative gids since the former hash table is for the whole process. Instead
// we create a thread specific hash table for the negative gids for each thread
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
// pd_index vecsize
// yvec
// tvec
//
// The critical issue requiring careful attention is that a corebluron
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

static void read_phase1(const char* fname, NrnThread& nt);
static void determine_inputpresyn(void);
static void read_phase2(const char* fname, NrnThread& nt);
static void setup_ThreadData(NrnThread& nt);
static size_t model_size(void);

static int n_inputpresyn_;
static InputPreSyn* inputpresyn_; // the global array of instances.

// InputPreSyn.nc_index_ to + InputPreSyn.nc_cnt_ give the NetCon*
NetCon** netcon_in_presyn_order_;

static int chkpnt;

void nrn_setup(int ngroup, int* gidgroups, const char *path) {
  char fname[100];
  assert(ngroup > 0);
#if 0
  nrn_threads_create(ngroup, 0); // serial threads
#else
  // temporary bug work around. If any process has multiple threads, no
  // process can have a single thread. So, for now, if one thread, make two.
  // Fortunately, empty threads work fine.
  { int ng = ngroup;
    if (ng == 1) { ng += 1; }
    nrn_threads_create(ng, 0); // serial threads
  }
#endif

  netpar_tid_gid2ps_alloc(ngroup);

  // bug fix. gid2out is cumulative over all threads and so do not
  // know how many there are til after phase1
  nrn_alloc_gid2out(1000, 100);
  // A process's complete set of output gids and allocation of each thread's
  // nt.presyns and nt.netcons arrays.
  // Generates the gid2out hash table which is needed
  // to later count the required number of InputPreSyn
  for (int i = 0; i < ngroup; ++i) {
    sprintf(fname, "%s/%d_1.dat", path, gidgroups[i]);
    read_phase1(fname, nrn_threads[i]);
  }

  if (nrnmpi_myid == 0)
  {
    printf("read_phase1 is done\n");
    fflush(0);
  }
  // from the netpar::gid2out_ hash table and the netcon_srcgid array,
  // fill the netpar::gid2in_ hash table, and from the number of entries,
  // allocate the process wide InputPreSyn array
  determine_inputpresyn();
//  nrn_alloc_gid2out(1,1);

  // read the rest of the gidgroup's data and complete the setup for each
  // thread.
  for (int i = 0; i < ngroup; ++i) {
    sprintf(fname, "%s/%d_2.dat", path, gidgroups[i]);
    read_phase2(fname, nrn_threads[i]);
    setup_ThreadData(nrn_threads[i]); // nrncore does this in multicore.c in thread_memblist_setup
    nrn_mk_table_check(); // was done in nrn_thread_memblist_setup in multicore.c
  }

  if (nrnmpi_myid == 0)
  {
    printf("read_phase2 is done\n");
    fflush(0);
  }

  netpar_tid_gid2ps_free();

  if (nrn_nthread > 1) {
    // NetCvode construction assumed one thread. Need nrn_nthread instances
    // of NetCvodeThreadData
    nrn_p_construct();
  }

  model_size();
}

void setup_ThreadData(NrnThread& nt) {
  for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
    Memb_func& mf = memb_func[tml->index];
    Memb_list* ml = tml->ml;
    if (mf.thread_size_) {
      ml->_thread = (ThreadDatum*)ecalloc(mf.thread_size_, sizeof(ThreadDatum));
      if (mf.thread_mem_init_) {
        (*mf.thread_mem_init_)(ml->_thread);
      }
    }
  }
}

static int read_int_(FILE* f) {
  int i = 0;
  char line[100];
  assert(fgets(line, 100, f));
  assert(sscanf(line, "%d", &i) == 1);
  return i;
}
#define read_int() read_int_(f)

static double* read_dbl_array_(double* p, size_t n, FILE* f) {
  double* a;
  if (p) {
    a = p;
  }else{
    a = new double[n];
  }
  int i= -1;
  char line[100];
  assert(fgets(line, 100, f));
  assert(sscanf(line, "chkpnt %d\n", &i) == 1);
  assert(i == chkpnt++);
  assert(fread(a, sizeof(double), n, f) == n);
  return a;
}
#define read_dbl_array(p, n) read_dbl_array_(p, n, f)

static int* read_int_array_(int* p, size_t n, FILE* f) {
  int* a;
  if (p) {
    a = p;
  }else{
    a = new int[n];
  }
  int i= -1;
  char line[100];
  assert(fgets(line, 100, f));
  assert(sscanf(line, "chkpnt %d\n", &i) == 1);
  assert(i == chkpnt++);
  assert(fread(a, sizeof(int), n, f) == n);
  return a;
}
#define read_int_array(p,n) read_int_array_(p, n, f)

void nrnthreads_free_helper(NrnThread* nt) {
	if (nt->pntprocs) delete[] nt->pntprocs;
	if (nt->presyns) delete [] nt->presyns;
	if (nt->netcons) delete [] nt->netcons;
	if (nt->weights) delete [] nt->weights;
}

void read_phase1(const char* fname, NrnThread& nt) {
  FILE* f = fopen(fname, "r");
  assert(f);
  nt.n_presyn = read_int();
  nt.n_netcon = read_int();
  nt.presyns = new PreSyn[nt.n_presyn];
  nt.netcons = new NetCon[nt.n_netcon];

  /// Checkpoint in bluron is defined for both phase 1 and phase 2 since they are written together
  chkpnt = 0;
  int* output_gid = read_int_array(NULL, nt.n_presyn);
  int* netcon_srcgid = read_int_array(NULL, nt.n_netcon);
  fclose(f);
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

  // allocate a proper size neg_gid2out_[tid] (see netpar.cpp);
  int negcnt = 1; // don't trouble ourselves with size 0 tables.
  for (int i=0; i < nt.n_presyn; ++i) {
    if (output_gid[i] < 0) {
      ++negcnt;
    }
  }
  // I no longer remember how generous we should be with the initial size
  // of a hash table.
  netpar_tid_gid2ps_alloc_item(nt.id, negcnt, 100);

  for (int i=0; i < nt.n_presyn; ++i) {
    int gid = output_gid[i];
    // Note that the negative (type, index)
    // coded information goes into the neg_gid2out_[tid] hash table.
    // See netpar.cpp for the netpar_tid_... function implementations.
    // Both that table and the process wide gid2out_ table can be deleted
    // before the end of setup
    netpar_tid_set_gid2node(nt.id, gid, nrnmpi_myid);
    netpar_tid_cell(nt.id, gid, nt.presyns + i);
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
  // all the output_gid have been registered and associated with PreSyn.
  // now count the needed InputPreSyn by filling the netpar::gid2in_ hash table
  // unfortunately, need space for a hash table in order to count
  // use total number of netcon for a temporary table
  int n_psi = 0;
  for (int ith = 0; ith < nrn_nthread; ++ith) {
    n_psi += nrn_threads[ith].n_netcon;
  }
  nrn_alloc_gid2in(n_psi, n_psi);

  // now do the actual count
  n_psi = 0;
  for (int ith = 0; ith < nrn_nthread; ++ith) {
    NrnThread& nt = nrn_threads[ith];
    for (int i = 0; i < nt.n_netcon; ++i) {
      int gid = nt.netcons[i].u.srcgid_;
      if (gid >= 0) {
        n_psi += input_gid_register(gid);
      }
    }
  }

  // free and alloc a more appropriate space
  nrn_alloc_gid2in(3*n_psi, n_psi);
  // now have to fill the new table
  n_psi = 0;
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

void read_phase2(const char* fname, NrnThread& nt) {
  FILE* f = fopen(fname, "r");
  NrnThreadMembList* tml;
  assert(f);
  int n_outputgid = read_int();
  assert(n_outputgid > 0); // avoid n_outputgid unused warning
  nt.ncell = read_int();
  nt.end = read_int();
  int nmech = read_int();

  /// Checkpoint in bluron is defined for both phase 1 and phase 2 since they are written together
  chkpnt = 2;
  //printf("ncell=%d end=%d nmech=%d\n", nt.ncell, nt.end, nmech);
  //printf("nart=%d\n", nart);
  NrnThreadMembList* tml_last = NULL;
  for (int i=0; i < nmech; ++i) {
    tml = (NrnThreadMembList*)emalloc(sizeof(NrnThreadMembList));
    tml->ml = (Memb_list*)emalloc(sizeof(Memb_list));
    tml->next = NULL;
    tml->index = read_int();
    tml->ml->nodecount = read_int();;
    //printf("index=%d nodecount=%d membfunc=%s\n", tml->index, tml->ml->nodecount, memb_func[tml->index].sym?memb_func[tml->index].sym:"None");
    if (nt.tml) {
      tml_last->next = tml;
    }else{
      nt.tml = tml;
    }
    tml_last = tml;
  }
  nt._ndata = read_int();
  nt._nidata = read_int();
  nt._nvdata = read_int();
  nt.n_weight = read_int();

  nt._data = (double*)ecalloc(nt._ndata, sizeof(double));
  if (nt._nidata) nt._idata = (int*)ecalloc(nt._nidata, sizeof(int));
  // see patternstim.cpp
  int zzz = (&nt == nrn_threads) ? nrn_extra_thread0_vdata : 0;
  if (nt._nvdata+zzz) 
    nt._vdata = (void**)ecalloc(nt._nvdata + zzz, sizeof(void*));
  //printf("_ndata=%d _nidata=%d _nvdata=%d\n", nt._ndata, nt._nidata, nt._nvdata);

  // The data format defines the order of matrix data
  int ne = nt.end;
  nt._actual_rhs = nt._data + 0*ne;
  nt._actual_d = nt._data + 1*ne;
  nt._actual_a = nt._data + 2*ne;
  nt._actual_b = nt._data + 3*ne;
  nt._actual_v = nt._data + 4*ne;
  nt._actual_area = nt._data + 5*ne;
  size_t offset = 6*ne;

  // Memb_list.data points into the nt.data array.
  // Also count the number of Point_process
  int npnt = 0;
  for (tml = nt.tml; tml; tml = tml->next) {
    Memb_list* ml = tml->ml;
    int type = tml->index;
    int n = ml->nodecount;
    int sz = nrn_prop_param_size_[type];
    ml->data = nt._data + offset;
    offset += n*sz;
    if (pnt_map[type] > 0) {
      npnt += n;
    }
  }
  nt.pntprocs = new Point_process[npnt]; // includes acell with and without gid
  nt.n_pntproc = npnt;
  //printf("offset=%ld ndata=%ld\n", offset, nt._ndata);
  assert(offset == nt._ndata);

  // matrix info
  nt._v_parent_index = read_int_array(NULL, nt.end);
  read_dbl_array(nt._actual_a, nt.end);
  read_dbl_array(nt._actual_b, nt.end);
  read_dbl_array(nt._actual_area, nt.end);
  read_dbl_array(nt._actual_v, nt.end);

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
    if (!is_art) {ml->nodeindices = read_int_array(NULL, ml->nodecount);}
    read_dbl_array(ml->data, n*szp);
    if (szdp) {
      ml->pdata = read_int_array(NULL, n*szdp);
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
        pp->type = type;
        pp->data = ml->data + i*szp;
        pp->pdata = ml->pdata + i*szdp;
        nt._vdata[pp->pdata[1]] = pp;
        pp->presyn_ = NULL;
        pp->_vnt = &nt;
      }
    }
  }

  // Real cells are at the beginning of the nt.presyns followed by
  // acells (with and without gids mixed together)
  // Here we associate the real cells with voltage pointers and
  // acell PreSyn with the Point_process.
  //nt.presyns order same as output_vindex order
  int* output_vindex = read_int_array(NULL, nt.n_presyn);
  double* output_threshold = read_dbl_array(NULL, nt.ncell);
  for (int i=0; i < nt.n_presyn; ++i) { // real cells
    PreSyn* ps = nt.presyns + i;
    int ix = output_vindex[i];
    if (ix < 0) {
      ix = -ix;
      int index = ix/1000;
      int type = ix - index*1000;
      Point_process* pnt = nt.pntprocs + (pnt_offset[type] + index);
      ps->pntsrc_ = pnt;
      pnt->presyn_ = ps;
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
  int* pnttype = read_int_array(NULL, nnetcon);
  int* pntindex = read_int_array(NULL, nnetcon);
  for (int i=0; i < nnetcon; ++i) {
    int type = pnttype[i];
    if (type > 0) {
      int index = pnt_offset[type] + pntindex[i];
      Point_process* pnt = nt.pntprocs + index;
      NetCon& nc = nt.netcons[i];
      nc.target_ = pnt;
      nc.active_ = true;
    }
  }
  delete [] pntindex;
  delete [] pnt_offset;

  // weights in netcons order in groups defined by Point_process target type.
  nt.weights = read_dbl_array(NULL, nweight);
  int iw = 0;
  for (int i=0; i < nnetcon; ++i) {
    NetCon& nc = nt.netcons[i];
    nc.u.weight_ = nt.weights + iw;
    iw += pnt_receive_size[pnttype[i]];
  }
  assert(iw == nweight);
  delete [] pnttype;

  // delays in netcons order
  double* delay = read_dbl_array(NULL, nnetcon);
  for (int i=0; i < nnetcon; ++i) {
    NetCon& nc = nt.netcons[i];
    nc.delay_ = delay[i];
  }
  delete [] delay;

  // BBCOREPOINTER information
  npnt = read_int();
  for (int i=0; i < npnt; ++i) {
    int type = read_int();
    assert(nrn_bbcore_read_[type]);
    int icnt = read_int();
    int dcnt = read_int();
    int* iArray = NULL;
    double* dArray = NULL;
    if (icnt) 
    {
      iArray = read_int_array(NULL, icnt);
    }
    if (dcnt) 
    {
      dArray = read_dbl_array(NULL, dcnt);
    }
    int ik = 0;
    int dk = 0;
    Memb_list* ml = mlmap[type];
    int dsz = nrn_prop_param_size_[type];
    int pdsz = nrn_prop_dparam_size_[type];
    for (int j=0; j < ml->nodecount; ++j) {
      double* d = ml->data + j*dsz;
      Datum* pd = ml->pdata + j*pdsz;
//      (*nrn_bbcore_read_[type])(vp, k, d, pd, ml->_thread, &nt);
      (*nrn_bbcore_read_[type])(dArray, iArray, &dk, &ik, d, pd, ml->_thread, &nt);
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
  int n = read_int();
  nt.n_vecplay = n;
  if (n) {
    nt._vecplay = new void*[n];
  }else{
    nt._vecplay = NULL;
  }
  for (int i=0; i < n; ++i) {
    int vtype = read_int();
    assert(vtype == VecPlayContinuousType);
    int ix = read_int();
    int sz = read_int();
    IvocVect* yvec = vector_new(sz);
    read_dbl_array(vector_vec(yvec), sz);
    IvocVect* tvec = vector_new(sz);
    read_dbl_array(vector_vec(tvec), sz);
    nt._vecplay[i] = new VecPlayContinuous(nt._data + ix, yvec, tvec, NULL, nt.id);
  }

  fclose(f);
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
