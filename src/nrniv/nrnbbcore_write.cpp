#include <../../nrnconf.h>
// A model built using NEURON is heavyweight in memory usage and that
// prevents maximizing the number of cells that can be simulated on
// a process. On the other hand, a tiny version of NEURON that contains
// only the cache efficient structures, minimal memory usage arrays,
// needed to do a simulation (no interpreter, hoc Objects, Section, etc.)
// lacks the model building flexibility of NEURON.
// Ideally, the only arrays needed for a tiny version simulation are those
// enumerated in the NrnThread structure in src/nrnoc/multicore.h up to,
// but not including, the Node** arrays. Also tiny versions of POINT_PROCESS,
// PreSyn, NetCon, and SUFFIX mechanisms will be stripped down from
// their full NEURON definitions and, it seems certain, many of the
// double fields will be converted to some other, less memory using, types.
// With this in mind, we envision that NEURON will incrementally construct
// cache efficient whole cell structures which can be saved and read with
// minimal processing into the tiny simulator. Note that this is a petabyte
// level of data volume. Consider, for example, 128K cores each
// preparing model data for several thousand cells using full NEURON where
// there is not enough space for the simultaneous existence of
// those several thousand cells --- but there is with the tiny version.

// Several assumptions with regard to the nrnbbcore_read reader.
// Since memory is filled with cells, whole cell
// load balance should be adequate and so there is no provision for
// multisplit. A process gets a list of the gid owned by that process
// and allocates the needed
// memory based on size variables for each gid, i.e.
// number of nodes, number of instances of each mechanism type, and number
// of NetCon instances. Also the offsets are calculated for where the
// cell information is to reside in the cache efficient arrays.
// The rest of the cell information is then copied
// into memory with the proper offsets. Pointers to data, used in the old
// NEURON world are converted to integer indices into a common data array.

// A good deal of conceptual confusion resulted in earlier implementations
// with regard to ordering of synapses and
// artificial cells with and without gids. The ordering of the property
// data for those is defined by the order in the NrnThread.tml list where
// every Memb_list.data has an easily found index relative to its 'nodecount'.
// (For artificial cells, since those are not ordered in a cache efficient
// array, we get the index using int nrncore_art2index(double* param)
// which looks up the index in a hash table. Earlier implementations
// handled 'artificial cells without gids' specially which also
// necessitated special handling of their NetCons and disallowed artificial
// cells with gids. We now handle all artificial cells in a thread
// in the same way as any other synapse (the assumption still holds that
// any artificial cell without a gid in a thread can connect only to
// targets in the same thread. Thus, a single NrnThread.synapses now contains
// all synapses and all artificial cells belonging to that thread. All
// the synapses and artificial cells are in NrnThread.tml order. So there
// are no exceptions in filling Point_process pointers from the data indices
// on the coreneuron side. PreSyn ordering is a bit more delicate.
// From netpar.cpp, the gid2out_ hash table defines an output_gid
// ordering and gives us all the PreSyn
// associated with real and artificial cells having gids. But those are
// randomly ordered and interleaved with 'no gid instances'
// relative to the tml ordering.
// Since the number of output PreSyn >= number of output_gid it makes sense
// to order the PreSyn in the same way as defined by the tml ordering.
// Thus, even though artificial cells with and without gids are mixed,
// at least it is convenient to fill the PreSyn.psrc field.
// Synapses are first but the artificial cells with and without gids are
// mixed. The problem that needs to
// be explicitly overcome is associating output gids with the proper PreSyn
// and that can be done with a list parallel to the acell part of the
// output_gid list that specifies the PreSyn list indices.
// Note that allocation of large arrays allows considerable space savings
// by eliminating overhead involved in allocation of many individual
// instances.
/*
Assumptions regarding the scope of possible models.(Incomplete list)
All real cells have gids.
Artificial cells without gids connect only to cells in the same thread.
No POINTER to data outside of NrnThread.
No POINTER to data in ARTIFICIAL_CELL (that data is not cache_efficient)
nt->tml->pdata is not cache_efficient
*/
// See coreneuron/nrniv/nrn_setup.cpp for a description of
// the file format written by this file.

/*
Support direct transfer of model to dynamically loaded coreneuron library.
To do this we factored all major file writing components into a series
of functions that return data that can be called from the coreneuron
library. The file writing functionality is kept by also calling those
functions here as well.
Direct transfer mode disables error checking with regard to every thread
having a real cell with a gid. Of course real and artificial cells without
gids do not have spike information in the output raster file. Trajectory
correctness has not been validated for cells without gids.
*/

#include <stdio.h>
#include <stdlib.h>

// for idDirExist and makePath
#include <ocfile.h>

#include <nrnran123.h> // globalindex written to globals.dat
#include <section.h>
#include <parse.h>
#include <nrnmpi.h>
#include <netcon.h>
#include <nrndae_c.h>
#include <algorithm>
#include <nrnhash_alt.h>
#include <nrnbbcore_write.h>
#include <netcvode.h> // for nrnbbcore_vecplay_write
#include <vrecitem.h> // for nrnbbcore_vecplay_write
#include <nrnsection_mapping.h>
#include <fstream>
#include <sstream>

#if defined(HAVE_DLFCN_H)
#include <dlfcn.h>
#endif

extern NetCvode* net_cvode_instance;

extern "C" { // to end of file

extern void hoc_execerror(const char*, const char*);
extern int* nrn_prop_param_size_;
extern int* nrn_prop_dparam_size_;
static int* bbcore_dparam_size; // cvodeieq not present
extern char* pnt_map;
extern short* nrn_is_artificial_;
extern int nrn_is_ion(int type);
extern double nrn_ion_charge(Symbol* sym);
extern Symbol* hoc_lookup(const char*);
extern int secondorder, diam_changed, v_structure_change, tree_changed;

/* not NULL, need to write gap information */
extern void (*nrnthread_v_transfer_)(NrnThread*);
extern size_t nrnbbcore_gap_write(const char* path, int* group_ids);

typedef void (*bbcore_write_t)(double*, int*, int*, int*, double*, Datum*, Datum*, NrnThread*);
extern bbcore_write_t* nrn_bbcore_write_;

static CellGroup* cellgroups_;
static CellGroup* mk_cellgroups(); // gid, PreSyn, NetCon, Point_process relation.
static void datumtransform(CellGroup*); // Datum.pval to int
static void datumindex_fill(int, CellGroup&, DatumIndices&, Memb_list*); //helper
static void write_byteswap1(const char* fname);
static void write_memb_mech_types(const char* fname);
static void write_memb_mech_types_direct(std::ostream& s);
static void write_globals(const char* fname);
static int get_global_int_item(const char* name);
static void* get_global_dbl_item(void* p, const char* & name, int& size, double*& val);
static void write_nrnthread(const char* fname, NrnThread& nt, CellGroup& cg);

static void nrnthread_group_ids(int* groupids);
static int nrnthread_dat1(int tid, int& n_presyn, int& n_netcon,
  int*& output_gid, int*& netcon_srcgid);
static int nrnthread_dat2_1(int tid, int& ngid, int& n_real_gid, int& nnode, int& ndiam,
  int& nmech, int*& tml_index, int*& ml_nodecount, int& nidata,
  int& nvdata, int& nweight);
static int nrnthread_dat2_2(int tid, int*& v_parent_index, double*& a, double*& b,
  double*& area, double*& v, double*& diamvec);
static int nrnthread_dat2_mech(int tid, size_t i, int dsz_inst, int*& nodeindices,
  double*& data, int*& pdata);
static int nrnthread_dat2_3(int tid, int nweight, int*& output_vindex, double*& output_threshold,
  int*& netcon_pnttype, int*& netcon_pntindex, double*& weights, double*& delays);
static int nrnthread_dat2_corepointer(int tid, int& n);
static int nrnthread_dat2_corepointer_mech(int tid, int type,
  int& icnt, int& dcnt, int*& iarray, double*& darray);
static int nrnthread_dat2_vecplay(int tid, int& n);
static int nrnthread_dat2_vecplay_inst(int tid, int i, int& vptype, int& mtype,
  int& ix, int& sz, double*& yvec, double*& tvec);

static void write_nrnthread_task(const char*, CellGroup* cgs);
static int* datum2int(int type, Memb_list* ml, NrnThread& nt, CellGroup& cg, DatumIndices& di, int ml_vdata_offset);
static void setup_nrn_has_net_event();
static int chkpnt;

// Up to now all the artificial cells have been left out of the processing.
// Since most processing is in the context of iteration over nt.tml it
// might be easiest to transform the loops using a
// copy of nt.tml with artificial cell types belonging to nt at the end.
// Treat these artificial cell memb_list as much as possible like the others.
// The only issue is that data for artificial cells is not in cache order
// (after all there is no BREAKPOINT or SOLVE block for ARTIFICIAL_CELLs)
// so we assume there will be no POINTER usage into that data.
// Also, note that ml.nodecount for artificial cell does not refer to
// a list of voltage nodes but just to the count of instances.
static void mk_tml_with_art(void); // set up MlWithArt CellGroup.mlwithart

declareNrnHash(PVoid2Int, void*, int)
implementNrnHash(PVoid2Int, void*, int)
PVoid2Int* artdata2index_;

/** mapping information */
static NrnMappingInfo mapinfo;

// to avoid incompatible dataset between neuron and coreneuron
// add version string to the dataset files
const char *bbcore_write_version = "1.2";

// direct transfer or via files? The latter makes use of group_gid for
// filename construction.
static bool corenrn_direct;

static size_t part1();
static void part2(const char*);

// prerequisites for a NEURON model to be transferred to CoreNEURON.
static void model_ready() {
  // Do the model type checks first as some of them prevent the success
  // of cvode.cache_efficient(1) and the error message associated with
  // !use_cachevec would be misleading. 
  if (!nrndae_list_is_empty()) {
    hoc_execerror("CoreNEURON cannot simulate a model that contains extra LinearMechanism or RxD equations", NULL);
  }
  if (nrn_threads[0]._ecell_memb_list) {
    hoc_execerror("CoreNEURON cannot simulate a model that contains the extracellular mechanism", NULL);
  }
  if (corenrn_direct) {
    if (cvode_active_) {
      hoc_execerror("CoreNEURON can only use fixed step method.", NULL);
    }
  }

  if (!use_cachevec) {
    hoc_execerror("NEURON model for CoreNEURON requires cvode.cache_efficient(1)", NULL);
  }
  if (tree_changed || v_structure_change || diam_changed) {
    hoc_execerror("NEURON model internal structures for CoreNEURON are out of date. Make sure call to finitialize(...) is after cvode.cache_efficient(1))", NULL);
  }
}

// accessible from ParallelContext.total_bytes()
size_t nrnbbcore_write() {
  corenrn_direct = false;
  model_ready();
  char fname[1024];
  std::string path(".");
  if (ifarg(1)) {
    path = hoc_gargstr(1);
    if (nrnmpi_myid == 0) {
      if (!isDirExist(path)) {
        if (!makePath(path)) {
          hoc_execerror(path.c_str(), "directory did not exist and makePath for it failed");
        }
      }
    }
#ifdef NRNMPI
    nrnmpi_barrier();
#endif
  }

  size_t rankbytes = part1(); // can arrange to be just before part2

  nrn_assert(snprintf(fname, 1024, "%s/%s", path.c_str(), "byteswap1.dat") < 1024);
  write_byteswap1(fname);

  nrn_assert(snprintf(fname, 1024, "%s/%s", path.c_str(), "bbcore_mech.dat") < 1024);
  write_memb_mech_types(fname);

  nrn_assert(snprintf(fname, 1024, "%s/%s", path.c_str(), "globals.dat") < 1024);
  write_globals(fname);

  part2(path.c_str());
  return rankbytes;
}

static size_t part1() {
  size_t rankbytes = 0;
  size_t nbytes;
  NrnThread* nt;
  NrnThreadMembList* tml;
  if (!bbcore_dparam_size) {
    bbcore_dparam_size = new int[n_memb_func];
  }
  for (int i=0; i < n_memb_func; ++i) {
    int sz = nrn_prop_dparam_size_[i];
    bbcore_dparam_size[i] = sz;
    Memb_func* mf = memb_func + i;
    if (mf && mf->dparam_semantics && sz && mf->dparam_semantics[sz-1] == -3) {
        // cvode_ieq in NEURON but not CoreNEURON
        bbcore_dparam_size[i] = sz - 1;
    }
  }
  setup_nrn_has_net_event();
  cellgroups_ = new CellGroup[nrn_nthread]; // here because following needs mlwithart
  mk_tml_with_art();

  FOR_THREADS(nt) {
    size_t threadbytes = 0;
    size_t npnt = 0;
    size_t nart = 0;
    int ith = nt->id;
    //printf("rank %d thread %d\n", nrnmpi_myid, ith);
    //printf("  ncell=%d nnode=%d\n", nt->ncell, nt->end);
    //v_parent_index, _actual_a, _actual_b, _actual_area
    nbytes = nt->end * (1*sizeof(int) + 3*sizeof(double));
    threadbytes += nbytes;

    int mechcnt = 0;
    size_t mechcnt_instances = 0;
    MlWithArt& mla = cellgroups_[ith].mlwithart;
    for (size_t i = 0; i < mla.size(); ++i) {
      int type = mla[i].first;
      Memb_list* ml = mla[i].second;
      ++mechcnt;
      mechcnt_instances += ml->nodecount;
      npnt += (memb_func[type].is_point ? ml->nodecount : 0);
      int psize = nrn_prop_param_size_[type];
      int dpsize = nrn_prop_dparam_size_[type]; // includes cvodeieq if present
      //printf("%d %s ispnt %d  cnt %d  psize %d  dpsize %d\n",tml->index, memb_func[type].sym->name,
      //memb_func[type].is_point, ml->nodecount, psize, dpsize);
      // nodeindices, data, pdata + pnt with prop
      int notart = nrn_is_artificial_[type] ? 0 : 1;
      if (nrn_is_artificial_[type]) {
        nart += ml->nodecount;
      }
      nbytes = ml->nodecount * (notart*sizeof(int) + 1*sizeof(double*) +
        1*sizeof(Datum*) + psize*sizeof(double) + dpsize*sizeof(Datum));
      threadbytes += nbytes;
    }
    nbytes += npnt * (sizeof(Point_process) + sizeof(Prop));
    //printf("  mech in use %d  Point instances %ld  artcells %ld  total instances %ld\n",
    //mechcnt, npnt, nart, mechcnt_instances);
    //printf("  thread bytes %ld\n", threadbytes);
    rankbytes += threadbytes;
  }
  
  rankbytes += nrncore_netpar_bytes();
  //printf("%d bytes %ld\n", nrnmpi_myid, rankbytes);
  CellGroup* cgs = mk_cellgroups();

  datumtransform(cgs);
  return rankbytes;
}

static void part2_clean() {
  if (artdata2index_) {
    delete artdata2index_;
    artdata2index_ = NULL;
  }
  delete [] cellgroups_;
  cellgroups_ = NULL;
}

static void part2(const char* path) {
  CellGroup* cgs = cellgroups_;
  for (int i=0; i < nrn_nthread; ++i) {
    chkpnt = 0;
    write_nrnthread(path, nrn_threads[i], cgs[i]);
  }

  /** write mapping information */
  if(mapinfo.size()) {
    int gid = cgs[0].group_id;
    nrn_write_mapping_info(path, gid, mapinfo);
    mapinfo.clear();
  }

  if (nrnthread_v_transfer_) {
    // see partrans.cpp. nrn_nthread files of path/icg_gap.dat
    int* group_ids = new int[nrn_nthread];
    for (int i=0; i < nrn_nthread; ++i) {
      group_ids[i] = cgs[i].group_id;
    }
    nrnbbcore_gap_write(path, group_ids);
    delete [] group_ids;
  }

  // filename data might have to be collected at hoc level since
  // pc.nrnbbcore_write might be called
  // many times per rank since model may be built as series of submodels.
  if (ifarg(2)) {
    Vect* cgidvec = vector_arg(2);
    vector_resize(cgidvec, nrn_nthread);
    double* px = vector_vec(cgidvec);
    for (int i=0; i < nrn_nthread; ++i) {
      px[i] = double(cgs[i].group_id);
    }
  }else{
    write_nrnthread_task(path, cgs);
  }

  // clean up the art Memb_list of CellGroup[].mlwithart
  for (int ith=0; ith < nrn_nthread; ++ith) {
    MlWithArt& mla = cgs[ith].mlwithart;
    for (size_t i = 0; i < mla.size(); ++i) {
      int type = mla[i].first;
      Memb_list* ml = mla[i].second;
      if (nrn_is_artificial_[type]) {
        delete [] ml->data;
        delete [] ml->pdata;
        delete ml;
      }
    }
  }

  part2_clean();
}

int nrncore_art2index(double* d) {
  int i;
  int result = artdata2index_->find(d, i);
  assert(result);
  return i;
}

extern "C" {
extern int nrn_has_net_event_cnt_;
extern int* nrn_has_net_event_;
}

static int* has_net_event_;
static void setup_nrn_has_net_event() {
  if (has_net_event_) { return; }
  has_net_event_ = new int[n_memb_func];
  for (int i=0; i < n_memb_func; ++i) {
    has_net_event_[i] = 0;
  }
  for(int i=0; i < nrn_has_net_event_cnt_; ++i) {
    has_net_event_[nrn_has_net_event_[i]] = 1;
  }
}

static int nrn_has_net_event(int type) {
  return has_net_event_[type];
}

void mk_tml_with_art() {
  // copy NrnThread tml list and append ARTIFICIAL cell types 
  // but do not include PatternStim
  // Now using cgs[tid].mlwithart instead of
  // tml_with_art = new NrnThreadMembList*[nrn_nthread];
  // to allow fast retrieval of type and Memb_list* given index into the vector.
  CellGroup* cgs = cellgroups_;
  // copy from NrnThread
  for (int id = 0; id < nrn_nthread; ++id) {
    MlWithArt& mla = cgs[id].mlwithart;
    for (NrnThreadMembList* tml = nrn_threads[id].tml; tml; tml = tml->next) {
      mla.push_back(MlWithArtItem(tml->index, tml->ml));
    }
  }
  int *acnt = new int[nrn_nthread];
  artdata2index_ = new PVoid2Int(1000);

  for (int i = 0; i < n_memb_func; ++i) {
    if (nrn_is_artificial_[i] && memb_list[i].nodecount) {
      // skip PatternStim
      if (strcmp(memb_func[i].sym->name, "PatternStim") == 0) { continue; }
      if (strcmp(memb_func[i].sym->name, "HDF5Reader") == 0) { continue; }
      Memb_list* ml = memb_list + i;
      // how many artificial in each thread
      for (int id = 0; id < nrn_nthread; ++id) {acnt[id] = 0;}
      for (int j = 0; j < memb_list[i].nodecount; ++j) {
        Point_process* pnt = (Point_process*)memb_list[i].pdata[j][1]._pvoid;
        int id = ((NrnThread*)pnt->_vnt)->id;
        ++acnt[id];
      }

      // allocate
      for (int id = 0; id < nrn_nthread; ++id) {
        if (acnt[id]) {
          MlWithArt& mla = cgs[id].mlwithart;
          ml = new Memb_list;
          mla.push_back(MlWithArtItem(i, ml)); // need to delete ml when mla destroyed.
          ml->nodecount = acnt[id];
          ml->nodelist = NULL;
          ml->nodeindices = NULL;
          ml->prop = NULL;
          ml->_thread = NULL;
          ml->data = new double*[acnt[id]];
          ml->pdata = new Datum*[acnt[id]];
        }
      }
      // fill data and pdata pointers
      // and fill the artdata2index hash table
      for (int id = 0; id < nrn_nthread; ++id) {acnt[id] = 0;}
      for (int j = 0; j < memb_list[i].nodecount; ++j) {
        Point_process* pnt = (Point_process*)memb_list[i].pdata[j][1]._pvoid;
        int id = ((NrnThread*)pnt->_vnt)->id;
        Memb_list* ml = cgs[id].mlwithart.back().second;
        ml->data[acnt[id]] = memb_list[i].data[j];
        ml->pdata[acnt[id]] = memb_list[i].pdata[j];
        artdata2index_->insert(ml->data[acnt[id]], acnt[id]);
        ++acnt[id];
      }
    }
  }
  delete [] acnt;
}

CellGroup::CellGroup() {
  n_output = n_real_output = n_presyn = n_netcon = n_mech = ntype = 0;
  group_id = -1;
  output_gid = output_vindex = 0;
  netcons = 0; output_ps = 0;
  ndiam = 0;
  netcon_srcgid = netcon_pnttype = netcon_pntindex = 0;
  datumindices = 0;
  type2ml = new Memb_list*[n_memb_func];
  for (int i=0; i < n_memb_func; ++i) {
    type2ml[i] = 0;
  }
  ml_vdata_offset = NULL;
}

CellGroup::~CellGroup() {
  if (output_gid) delete [] output_gid;
  if (output_vindex) delete [] output_vindex;
  if (netcon_srcgid) delete [] netcon_srcgid;
  if (netcon_pnttype) delete [] netcon_pnttype;
  if (netcon_pntindex) delete [] netcon_pntindex;
  if (datumindices) delete [] datumindices;
  if (netcons) delete [] netcons;
  if (output_ps) delete [] output_ps;
  if (ml_vdata_offset) delete [] ml_vdata_offset;
  delete [] type2ml;
}

DatumIndices::DatumIndices() {
  type = -1;
  ion_type = ion_index = 0;
}

DatumIndices::~DatumIndices() {
  if (ion_type) delete [] ion_type;
  if (ion_index) delete [] ion_index;
}

// use the Hoc NetCon object list to segregate according to threads
// and fill the CellGroup netcons, netcon_srcgid, netcon_pnttype, and
// netcon_pntindex (called at end of mk_cellgroups);
static void mk_cgs_netcon_info(CellGroup* cgs) {
  // count the netcons for each thread
  int* nccnt = new int[nrn_nthread];
  for (int i=0; i < nrn_nthread; ++i) {
    nccnt[i] = 0;
  }
  Symbol* ncsym = hoc_lookup("NetCon");
  hoc_List* ncl = ncsym->u.ctemplate->olist;
  hoc_Item* q;
  ITERATE(q, ncl) {
    Object* ho = (Object*)VOIDITM(q);
    NetCon* nc = (NetCon*)ho->u.this_pointer;
    int ith = 0; // if no _vnt, put in thread 0
    if (nc->target_ && nc->target_->_vnt) {
      ith = ((NrnThread*)(nc->target_->_vnt))->id;
    }
    ++nccnt[ith];
  }

  // allocate
  for (int i=0; i < nrn_nthread; ++i) {
    cgs[i].n_netcon = nccnt[i];
    cgs[i].netcons = new NetCon*[nccnt[i]+1];
    cgs[i].netcon_srcgid = new int[nccnt[i]+1];
    cgs[i].netcon_pnttype = new int[nccnt[i]+1];
    cgs[i].netcon_pntindex = new int[nccnt[i]+1];
  }

  // reset counts and fill
  for (int i=0; i < nrn_nthread; ++i) {
    nccnt[i] = 0;
  }
  ITERATE(q, ncl) {
    Object* ho = (Object*)VOIDITM(q);
    NetCon* nc = (NetCon*)ho->u.this_pointer;
    int ith = 0; // if no _vnt, put in thread 0
    if (nc->target_ && nc->target_->_vnt) {
      ith = ((NrnThread*)(nc->target_->_vnt))->id;
    }
    int i = nccnt[ith];
    cgs[ith].netcons[i] = nc;

    if (nc->target_) {
      int type = nc->target_->prop->type;
      cgs[ith].netcon_pnttype[i] = type;
      if (nrn_is_artificial_[type]) {
        cgs[ith].netcon_pntindex[i] = nrncore_art2index(nc->target_->prop->param);
      }else{
        // cache efficient so can calculate index from pointer
        Memb_list* ml = cgs[ith].type2ml[type];
        int sz = nrn_prop_param_size_[type];
        double* d1 = ml->data[0];
        double* d2 = nc->target_->prop->param;
        assert(d2 >= d1 && d2 < (d1 + (sz*ml->nodecount)));
        int ix = (d2 - d1)/sz;
        cgs[ith].netcon_pntindex[i] = ix;
      }
    }else{
      cgs[ith].netcon_pnttype[i] = 0;
      cgs[ith].netcon_pntindex[i] = -1;
    }

    if (nc->src_) {
      PreSyn* ps = nc->src_;
      if (ps->gid_ >= 0) {
        cgs[ith].netcon_srcgid[i] = ps->gid_;
      }else{
        if (ps->osrc_) {
          assert(ps->thvar_ == NULL);
          Point_process* pnt = (Point_process*)ps->osrc_->u.this_pointer;
          int type = pnt->prop->type;
          if (nrn_is_artificial_[type]) {
            int ix = nrncore_art2index(pnt->prop->param);
            cgs[ith].netcon_srcgid[i] = -(type + 1000*ix);
          }else{
            assert(nrn_has_net_event(type));
            Memb_list* ml = cgs[ith].type2ml[type];
            int sz = nrn_prop_param_size_[type];
            double* d1 = ml->data[0];
            double* d2 = pnt->prop->param;
            assert(d2 >= d1 && d2 < (d1 + (sz*ml->nodecount)));
            int ix = (d2 - d1)/sz;
            cgs[ith].netcon_srcgid[i] = -(type + 1000*ix);
          }
        }else{
          cgs[ith].netcon_srcgid[i] = -1;
        }
      }
    }else{
      cgs[ith].netcon_srcgid[i] = -1;
    }
    ++nccnt[ith];
  }
  delete [] nccnt;
}

CellGroup* mk_cellgroups() {
  CellGroup* cgs = cellgroups_;
  for (int i=0; i < nrn_nthread; ++i) {
    int ncell = nrn_threads[i].ncell; // real cell count
    int npre = ncell;
    MlWithArt& mla = cgs[i].mlwithart;
    for (size_t j = 0; j < mla.size(); ++j) {
      int type = mla[j].first;
      Memb_list* ml = mla[j].second;
      cgs[i].type2ml[type] = ml;
      if (nrn_has_net_event(type)) {
        npre += ml->nodecount;
      }
    }
    cgs[i].n_presyn = npre;
    cgs[i].n_real_output = ncell;
    cgs[i].output_ps = new PreSyn*[npre];
    cgs[i].output_gid = new int[npre];
    cgs[i].output_vindex = new int[npre];
    // in case some cells do not have voltage presyns (eg threshold detection
    // computed from a POINT_PROCESS NET_RECEIVE with WATCH and net_event)
    // initialize as unused.
    for (int j=0; j < npre; ++j) {
      cgs[i].output_ps[j] = NULL;
      cgs[i].output_gid[j] = -1;
      cgs[i].output_vindex[j] = -1;
    }

    // fill in the artcell info
    npre = ncell;
    cgs[i].n_output = ncell; // add artcell (and PP with net_event) with gid in following loop
    for (size_t j = 0; j < mla.size(); ++j) {
      int type = mla[j].first;
      Memb_list* ml = mla[j].second;
      if (nrn_has_net_event(type)) {
        for (int j=0; j < ml->nodecount; ++j) {
          Point_process* pnt = (Point_process*)ml->pdata[j][1]._pvoid;
          PreSyn* ps = (PreSyn*)pnt->presyn_;
          cgs[i].output_ps[npre] = ps;
          int agid = -1;
          if (nrn_is_artificial_[type]) {
            agid = -(type + 1000*nrncore_art2index(pnt->prop->param));
          }else{ // POINT_PROCESS with net_event
            int sz = nrn_prop_param_size_[type];
            double* d1 = ml->data[0];
            double* d2 = pnt->prop->param;
            assert(d2 >= d1 && d2 < (d1 + (sz*ml->nodecount)));
            int ix = (d2 - d1)/sz;
            agid = -(type + 1000*ix);
          }
          if (ps) {
            if (ps->output_index_ >= 0) { // has gid
              cgs[i].output_gid[npre] = ps->output_index_;
              if (cgs[i].group_id < 0) {
                cgs[i].group_id = ps->output_index_;
              }
              ++cgs[i].n_output;
            }else{
              cgs[i].output_gid[npre] = agid;
            }
          }else{ // if an acell is never a source, it will not have a presyn
            cgs[i].output_gid[npre] = -1;
          }
          // the way we associate an acell PreSyn with the Point_process.
          cgs[i].output_vindex[npre] = agid;
          ++npre;
        }
      }
    }
  }
  // work at netpar.cpp because we don't have the output gid hash tables here.
  // fill in the output_ps, output_gid, and output_vindex for the real cells.
  nrncore_netpar_cellgroups_helper(cgs);

  // use first real cell gid, if it exists, as the group_id
  if (corenrn_direct == false) for (int i=0; i < nrn_nthread; ++i) {
    if (cgs[i].n_real_output && cgs[i].output_gid[0] >= 0) {
      cgs[i].group_id = cgs[i].output_gid[0];
    }else if (cgs[i].group_id >= 0) {
      // set above to first artificial cell with a ps->output_index >= 0
    }else{
      hoc_execerror("A thread has no real cell or ARTIFICIAL_CELL with a gid", NULL);
    }
  }

  // use the Hoc NetCon object list to segregate according to threads
  // and fill the CellGroup netcons, netcon_srcgid, netcon_pnttype, and
  // netcon_pntindex
  mk_cgs_netcon_info(cgs);

  return cgs;
}

void datumtransform(CellGroup* cgs) {
  // ions, area, and POINTER to v.
  for (int ith=0; ith < nrn_nthread; ++ith) {
    NrnThread& nt = nrn_threads[ith];
    CellGroup& cg = cgs[ith];
    // how many mechanisms in use and how many DatumIndices do we need.
    MlWithArt& mla = cgs[ith].mlwithart;
    for (size_t j = 0; j < mla.size(); ++j) {
      Memb_list* ml = mla[j].second;
      ++cg.n_mech;
      if (ml->pdata[0]) {
        ++cg.ntype;
      }
    }
    cg.datumindices = new DatumIndices[cg.ntype];
    // specify type, allocate the space, and fill the indices
    int i=0;
    for (size_t j = 0; j < mla.size(); ++j) {
      int type = mla[j].first;
      Memb_list* ml = mla[j].second;
      int sz = bbcore_dparam_size[type];
      if (sz) {
        DatumIndices& di = cg.datumindices[i++];
        di.type = type;
        int n = ml->nodecount * sz;
        di.ion_type = new int[n];
        di.ion_index = new int[n];
        // fill the indices.
        // had tointroduce a memb_func[i].dparam_semantics registered by each mod file.
        datumindex_fill(ith, cg, di, ml);
      }
    }
  }
}

// This function is related to stdindex2ptr in CoreNeuron to determine which values should
// be transferred from CoreNeuron. Types correspond to the value to be transferred based on
// mech_type enum or non-artificial cell mechanisms.
// Limited to pointers to voltage, nt._nrn_fast_imem->_nrn_sav_rhs (fast_imem value) or
// data of non-artificial cell mechanisms.
// Requires cache_efficient mode.
// Input double* and NrnThread. Output type and index.
// type == 0 means could not determine index.
int nrn_dblpntr2nrncore(double* pd, NrnThread& nt, int& type, int& index) {
  assert(use_cachevec);
  int nnode = nt.end;
  type = 0;
  if (pd >= nt._actual_v && pd < (nt._actual_v + nnode)) {
    type = voltage; // signifies an index into voltage array portion of _data
    index = pd - nt._actual_v;
  } else if (pd >= nt._nrn_fast_imem->_nrn_sav_rhs && pd < (nt._nrn_fast_imem->_nrn_sav_rhs + nnode)) {
    type = i_membrane_; // signifies an index into i_membrane_ array portion of _data
    index = pd - nt._nrn_fast_imem->_nrn_sav_rhs;
  }else{
    for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
      if (nrn_is_artificial_[tml->index]) { continue; }
      Memb_list* ml1 = tml->ml;
      int nn = nrn_prop_param_size_[tml->index] * ml1->nodecount;
      if (pd >= ml1->data[0] && pd < (ml1->data[0] + nn)) {
        type = tml->index;
        index = pd - ml1->data[0];
        break;
      }
    }
  }
  return type == 0 ? 1 : 0;
}

void datumindex_fill(int ith, CellGroup& cg, DatumIndices& di, Memb_list* ml) {
  NrnThread& nt = nrn_threads[ith];
  double* a = nt._actual_area;
  int nnode = nt.end;
  int mcnt = ml->nodecount;
  int dsize = bbcore_dparam_size[di.type];
  if (dsize == 0) { return; }
  int* dmap = memb_func[di.type].dparam_semantics;
  assert(dmap);
  // what is the size of the nt._vdata portion needed for a single ml->dparam[i]
  int vdata_size = 0;
  for (int i=0; i < dsize; ++i) {
    int* ds = memb_func[di.type].dparam_semantics;
    if (ds[i] == -4 || ds[i] == -6 || ds[i] == -7 || ds[i] == 0) {
      ++vdata_size;
    }
  }

  int isart = nrn_is_artificial_[di.type];
  for (int i=0; i < mcnt; ++i) {
    // Prop* datum instance arrays are not in cache efficient order
    // ie. ml->pdata[i] are not laid out end to end in memory.
    // Also, ml->data for artificial cells is not in cache efficient order
    // but in the artcell case there are no pointers to doubles and
    // the _actual_area pointer should be left unfilled.
    Datum* dparam = ml->pdata[i];
    int offset = i*dsize;
    int vdata_offset = i*vdata_size;
    for (int j=0; j < dsize; ++j) {
      int etype = -100; // uninterpreted
      int eindex = -1;
      if (dmap[j] == -1) { // double* into _actual_area
        if (isart) {
          etype = -1;
          eindex = -1; // the signal to ignore in bbcore.
        }else{
          if (dparam[j].pval == &ml->nodelist[i]->_area) {
            // possibility it points directly into Node._area instead of
            // _actual_area. For our purposes we need to figure out the
            // _actual_area index.
            etype = -1;
            eindex = ml->nodeindices[i];
            assert(a[ml->nodeindices[i]] == *dparam[j].pval);
          }else{
            if (dparam[j].pval < a || dparam[j].pval >= (a + nnode)){
              printf("%s dparam=%p a=%p a+nnode=%p j=%d\n",
                  memb_func[di.type].sym->name, dparam[j].pval, a, a+nnode, j);
              abort();
            }
            assert(dparam[j].pval >= a && dparam[j].pval < (a + nnode));
            etype = -1;
            eindex = dparam[j].pval - a;
          }
        }
      }else if (dmap[j] == -2) { // this is an ion and dparam[j][0].i is the iontype
        etype = -2;
        eindex = dparam[j].i;
      }else if (dmap[j] == -3) { // cvodeieq is always last and never seen
        assert(dmap[j] != -3);
      }else if (dmap[j] == -4) { // netsend (_tqitem pointer)
        // eventually index into nt->_vdata
        etype = -4;
        eindex = vdata_offset++;
      }else if (dmap[j] == -6) { // pntproc
        // eventually index into nt->_vdata
        etype = -6;
        eindex = vdata_offset++;
      }else if (dmap[j] == -7) { // bbcorepointer
        // eventually index into nt->_vdata
        etype = -6;
        eindex = vdata_offset++;
      }else if (dmap[j] == -8) { // watch
        etype = -8;
        eindex = 0;
      }else if (dmap[j] == -9) { // diam
        cg.ndiam = nt.end;
        etype = -9;
        // Rare for a mechanism to use dparam pointing to diam.
        // MORPHOLOGY was never made cache efficient. And
        // is not in the tml_with_art. 
        // Need to determine this node and then simple to search its
        // mechanism list for MORPHOLOGY and then know the diam.
        Node* nd = ml->nodelist[i];
        double* pdiam = NULL;
        for (Prop* p = nd->prop; p; p = p->next) {
          if (p->type == MORPHOLOGY) {
            pdiam = p->param;
            break;
          }
        }
        assert(dparam[j].pval == pdiam);
        eindex = ml->nodeindices[i];
      }else if (dmap[j] == -5) { // POINTER
        // must be a pointer into nt->_data. Handling is similar to eion so
        // give proper index into the type.
        double* pd = dparam[j].pval;
	nrn_dblpntr2nrncore(pd, nt, etype, eindex);
        if (etype == 0) {
          fprintf(stderr, "POINTER is not pointing to voltage or mechanism data. Perhaps it should be a BBCOREPOINTER\n");
        }
        assert(etype != 0);
        // pointer into one of the tml types?
      }else if (dmap[j] > 0 && dmap[j] < 1000) { // double* into eion type data
        Memb_list* eml = cg.type2ml[dmap[j]];
        assert(eml);
        if(dparam[j].pval < eml->data[0]){
          printf("%s dparam=%p data=%p j=%d etype=%d %s\n",
              memb_func[di.type].sym->name, dparam[j].pval, eml->data[0], j,
              dmap[j], memb_func[dmap[j]].sym->name);
          abort();
        }
        assert(dparam[j].pval >= eml->data[0]);
        etype = dmap[j];
        if (dparam[j].pval >= (eml->data[0] +
              (nrn_prop_param_size_[etype] * eml->nodecount))) {
          printf("%s dparam=%p data=%p j=%d psize=%d nodecount=%d etype=%d %s\n",
              memb_func[di.type].sym->name, dparam[j].pval, eml->data[0], j,
              nrn_prop_param_size_[etype],
              eml->nodecount, etype, memb_func[etype].sym->name);
        }
        assert(dparam[j].pval < (eml->data[0] +
              (nrn_prop_param_size_[etype] * eml->nodecount)));
        eindex = dparam[j].pval - eml->data[0];
      }else if (dmap[j] > 1000) {//int* into ion dparam[xxx][0]
        //store the actual ionstyle
        etype = dmap[j];
        eindex = *((int*)dparam[j]._pvoid);
      } else {
        char errmes[100];
        sprintf(errmes, "Unknown semantics type %d for dparam item %d of", dmap[j], j);
        hoc_execerror(errmes, memb_func[di.type].sym->name);
      }
      di.ion_type[offset + j] = etype;
      di.ion_index[offset + j] = eindex;
    }
  }
}

static void write_byteswap1(const char* fname) {
  if (nrnmpi_myid > 0) { return; } // only rank 0 writes this file
  FILE* f = fopen(fname, "wb");
  if (!f) {
    hoc_execerror("nrnbbcore_write write_byteswap1 could not open for writing: %s\n", fname);
  }
  // write an endian sentinal value so reader can determine if byteswap needed.
  int32_t x = 1;
  fwrite(&x, sizeof(int32_t), 1, f);
  fclose(f);
}

static void write_memb_mech_types(const char* fname) {
  if (nrnmpi_myid > 0) { return; } // only rank 0 writes this file
  std::ofstream fs(fname);
  if (!fs.good()) {
    hoc_execerror("nrnbbcore_write write_mem_mech_types could not open for writing: %s\n", fname);
  }
  write_memb_mech_types_direct(fs);
}

static void write_memb_mech_types_direct(std::ostream& s) {
  // list of Memb_func names, types, point type info, is_ion
  // and data, pdata instance sizes. If the mechanism is an eion type,
  // the following line is the charge.
  // Not all Memb_func are necessarily used in the model.
  s << bbcore_write_version << std::endl;
  s << n_memb_func << std::endl;
  for (int type=2; type < n_memb_func; ++type) {
    const char* w = " ";
    Memb_func& mf = memb_func[type];
    s << mf.sym->name << w << type << w
      << int(pnt_map[type]) << w // the pointtype, 0 means not a POINT_PROCESS
      << nrn_is_artificial_[type] << w
      << nrn_is_ion(type) << w
      << nrn_prop_param_size_[type] << w << bbcore_dparam_size[type] << std::endl;

    if (nrn_is_ion(type)) {
        s << nrn_ion_charge(mf.sym) << std::endl;
    }
  }
}

// format is name value
// with last line of 0 0
//In case of an array, the line is name[num] with num lines following with
// one value per line.  Values are %.20g format.
static void write_globals(const char* fname) {

  if (nrnmpi_myid > 0) { return; } // only rank 0 writes this file

  FILE* f = fopen(fname, "w");
  if (!f) {
    hoc_execerror("nrnbbcore_write write_globals could not open for writing: %s\n", fname);
  }

  fprintf(f, "%s\n", bbcore_write_version);
  const char* name;
  int size; // 0 means scalar, is 0 will still allocated one element for val.
  double* val; // Allocated by new in get_global_item, must be delete [] here.
  for (void* sp = NULL;
        (sp = get_global_dbl_item(sp, name, size, val)) != NULL;) {
    if (size) {
      fprintf(f, "%s[%d]\n", name, size);
      for (int i=0; i < size; ++i) {
        fprintf(f, "%.20g\n", val[i]);
      }
    }else{
      fprintf(f, "%s %.20g\n", name, val[0]);
    }
    delete [] val;
  }
  fprintf(f, "0 0\n"); 
  fprintf(f, "secondorder %d\n", secondorder);
  fprintf(f, "Random123_globalindex %d\n", nrnran123_get_globalindex());

  fclose(f);
}

// just for secondorder and Random123_globalindex
static int get_global_int_item(const char* name) {
  if (strcmp(name, "secondorder") == 0) {
    return secondorder;
  }else if(strcmp(name, "Random123_global_index") == 0) {
    return nrnran123_get_globalindex();
  }
  return 0;
}

// successively return global double info. Begin with p==NULL.
// Done when return NULL.
static void* get_global_dbl_item(void* p, const char* & name, int& size, double*& val) {
  Symbol* sp = (Symbol*)p;
  if (sp == NULL) {
    sp = hoc_built_in_symlist->first;
  }
  for (; sp; sp = sp->next) {
    if (sp->type == VAR && sp->subtype == USERDOUBLE) {
      name = sp->name;
      if (ISARRAY(sp)) {
        Arrayinfo* a = sp->arayinfo;
        if (a->nsub == 1) {
          size = a->sub[0];
          val = new double[size];
          for (int i=0; i < a->sub[0]; ++i) {
            char n[256];
            sprintf(n, "%s[%d]", sp->name, i);
            val[i] =  *hoc_val_pointer(n);
          }
        }
      }else{
        size = 0;
        val = new double[1];
        val[0] = *sp->u.pval;
      }
      return sp->next;
    }
  }
  return NULL;
}

void writeint_(int* p, size_t size, FILE* f) {
  fprintf(f, "chkpnt %d\n", chkpnt++);
  size_t n = fwrite(p, sizeof(int), size, f);
  assert(n == size);
}

void writedbl_(double* p, size_t size, FILE* f) {
  fprintf(f, "chkpnt %d\n", chkpnt++);
  size_t n = fwrite(p, sizeof(double), size, f);
  assert(n == size);
}

#define writeint(p,size) writeint_(p, size, f)
#define writedbl(p,size) writedbl_(p, size, f)

static void write_contiguous_art_data(double** data, int nitem, int szitem, FILE* f) {
  fprintf(f, "chkpnt %d\n", chkpnt++);
  // the assumption is that an fwrite of nitem groups of szitem doubles can be
  // fread as a single group of nitem*szitem doubles.
  for (int i = 0; i < nitem; ++i) {
    size_t n = fwrite(data[i], sizeof(double), szitem, f);
    assert(n == szitem);
  }
}

static double* contiguous_art_data(double** data, int nitem, int szitem) {
  double* d1 = new double[nitem*szitem];
  int k = 0;
  for (int i = 0; i < nitem; ++i) {
    for (int j=0; j < szitem; ++j) {
      d1[k++] = data[i][j];
    }
  }
  return d1;
}

// Vector.play information.
// Must play into a data element in this thread
// File format is # of play instances in this thread (generally VecPlayContinuous)
// For each Play instance
// VecPlayContinuousType (4), pd (index), y.size, yvec, tvec
// Other VecPlay instance types are possible, such as VecPlayContinuous with
// a discon vector or VecPlayStep with a DT or tvec, but are not implemented
// at present. Assertion errors are generated if not type 0 of if we
// cannot determine the index into the NrnThread._data .

static int nrnthread_dat2_vecplay(int tid, int& n) {
  if (tid >= nrn_nthread) { return 0; }
  NrnThread& nt = nrn_threads[tid];

  // count the instances for this thread
  // error if not a VecPlayContinuous with no discon vector
  n = 0;
  PlayRecList* fp = net_cvode_instance->fixed_play_;
  for (int i=0; i < fp->count(); ++i){
    if (fp->item(i)->type() == VecPlayContinuousType) {
      VecPlayContinuous* vp = (VecPlayContinuous*)fp->item(i);
      if (vp->discon_indices_ == NULL) {
        if (vp->ith_ == nt.id) {
          assert(vp->y_ && vp->t_);
          ++n;
        }
      }else{
        assert(0);
      }
    }else{
      assert(0);
    }
  }

  return 1;
}

static int nrnthread_dat2_vecplay_inst(int tid, int i, int& vptype, int& mtype,
  int& ix, int& sz, double*& yvec, double*& tvec) {

  if (tid >= nrn_nthread) { return 0; }
  NrnThread& nt = nrn_threads[tid];

    PlayRecList* fp = net_cvode_instance->fixed_play_;
    if (fp->item(i)->type() == VecPlayContinuousType) {
      VecPlayContinuous* vp = (VecPlayContinuous*)fp->item(i);
      if (vp->discon_indices_ == NULL) {
        if (vp->ith_ == nt.id) {
          double* pd = vp->pd_;
          int found = 0;
          vptype = vp->type();
          for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
            if (nrn_is_artificial_[tml->index]) { continue; }
            Memb_list* ml = tml->ml;
            int nn = nrn_prop_param_size_[tml->index] * ml->nodecount;
            if (pd >= ml->data[0] && pd < (ml->data[0] + nn)) {
              mtype = tml->index;
              ix = (pd - ml->data[0]);
              sz = vector_capacity(vp->y_);
              yvec = vector_vec(vp->y_);
              tvec = vector_vec(vp->t_);
              found = 1;
              break;
            }
          }
          assert(found);
          return 1;
        }
      }
    }

  return 0;
}

static void nrnbbcore_vecplay_write(FILE* f, NrnThread& nt) {
  // count the instances for this thread
  // error if not a VecPlayContinuous with no discon vector
  int n;
  nrnthread_dat2_vecplay(nt.id, n);
  fprintf(f, "%d VecPlay instances\n", n);
  PlayRecList* fp = net_cvode_instance->fixed_play_;
  for (int i=0; i < fp->count(); ++i) {
    int vptype, mtype, ix, sz; double *yvec, *tvec;
    if (nrnthread_dat2_vecplay_inst(nt.id, i, vptype, mtype, ix, sz, yvec, tvec)) {
      fprintf(f, "%d\n", vptype);
      fprintf(f, "%d\n", mtype);
      fprintf(f, "%d\n", ix);
      fprintf(f, "%d\n", sz);
      writedbl(yvec, sz);
      writedbl(tvec, sz);
    }
  }
}

static int nrnthread_dat1(int tid, int& n_presyn, int& n_netcon,
  int*& output_gid, int*& netcon_srcgid) {

  if (tid >= nrn_nthread) { return 0; }
  CellGroup& cg = cellgroups_[tid];
  n_presyn = cg.n_presyn;
  n_netcon = cg.n_netcon;
  output_gid = cg.output_gid;  cg.output_gid = NULL;
  netcon_srcgid = cg.netcon_srcgid;  cg.netcon_srcgid = NULL;
  return 1;
}

// sizes and total data count
static int nrnthread_dat2_1(int tid, int& ngid, int& n_real_gid, int& nnode, int& ndiam,
  int& nmech, int*& tml_index, int*& ml_nodecount, int& nidata, int& nvdata, int& nweight) {
  
  if (tid >= nrn_nthread) { return 0; }
  CellGroup& cg = cellgroups_[tid];
  NrnThread& nt = nrn_threads[tid];

  ngid = cg.n_output;
  n_real_gid = cg.n_real_output;
  nnode = nt.end;
  ndiam = cg.ndiam;
  nmech = cg.n_mech;

  cg.ml_vdata_offset = new int[nmech];
  int vdata_offset = 0;
  tml_index = new int[nmech];
  ml_nodecount = new int[nmech];
  MlWithArt& mla = cg.mlwithart;
  for (size_t j = 0; j < mla.size(); ++j) {
    int type = mla[j].first;
    Memb_list* ml = mla[j].second;
    tml_index[j] = type;
    ml_nodecount[j] = ml->nodecount;
    cg.ml_vdata_offset[j] = vdata_offset;
    int* ds = memb_func[type].dparam_semantics;
    for (int psz=0; psz < bbcore_dparam_size[type]; ++psz) {
      if (ds[psz] == -4 || ds[psz] == -6 || ds[psz] == -7 || ds[psz] == 0) {
        //printf("%s ds[%d]=%d vdata_offset=%d\n", memb_func[type].sym->name, psz, ds[psz], vdata_offset);
        vdata_offset += ml->nodecount;
      }
    }
  }
  nvdata = vdata_offset;
  nidata = 0;
  //  printf("nidata=%d nvdata=%d nnetcon=%d\n", nidata, nvdata, cg.n_netcon);
  nweight = 0;
  for (int i=0; i < cg.n_netcon; ++i) {
    nweight += cg.netcons[i]->cnt_;
  }

  return 1;
}

static int nrnthread_dat2_2(int tid, int*& v_parent_index, double*& a, double*& b,
  double*& area, double*& v, double*& diamvec) {

  if (tid >= nrn_nthread) { return 0; }
  CellGroup& cg = cellgroups_[tid];
  NrnThread& nt = nrn_threads[tid];

  assert(cg.n_real_output == nt.ncell);

  // if not NULL then copy (for direct transfer target space already allocated)
  bool copy = v_parent_index ? true : false;
  int n = nt.end;
  if (copy) {
    for (int i=0; i < nt.end; ++i) {
      v_parent_index[i] = nt._v_parent_index[i];
      a[i] = nt._actual_a[i];
      b[i] = nt._actual_b[i];
      area[i] = nt._actual_area[i];
      v[i] = nt._actual_v[i];
    }
  }else{
    v_parent_index = nt._v_parent_index;
    a = nt._actual_a;
    b = nt._actual_b;
    area = nt._actual_area;
    v = nt._actual_v;
  }
  if (cg.ndiam) {
    if (!copy) {
      diamvec = new double[nt.end];
    }
    for (int i=0; i < nt.end; ++i) {
      Node* nd = nt._v_node[i];
      double diam = 0.0;
      for (Prop* p = nd->prop; p; p = p->next) {
        if (p->type == MORPHOLOGY) {
          diam = p->param[0];
          break;
        }
      }
      diamvec[i] = diam;
    }
  }
  return 1;
}

static int nrnthread_dat2_mech(int tid, size_t i, int dsz_inst, int*& nodeindices,
  double*& data, int*& pdata) {

  if (tid >= nrn_nthread) { return 0; }
  CellGroup& cg = cellgroups_[tid];
  NrnThread& nt = nrn_threads[tid];
  MlWithArtItem& mlai = cg.mlwithart[i];
  int type = mlai.first;
  Memb_list* ml = mlai.second;
  // for direct transfer, data=NULL means copy into passed space for nodeindices, data, and pdata
  bool copy = data ? true : false;

    int vdata_offset = cg.ml_vdata_offset[i];
    int isart = nrn_is_artificial_[type];
    int n = ml->nodecount;
    int sz = nrn_prop_param_size_[type];
    double* data1;
    if (isart) { // data may not be contiguous
      data1 = contiguous_art_data(ml->data, n, sz); // delete after use
      nodeindices = NULL;
    }else{
      nodeindices = ml->nodeindices; // allocated below if copy
      data1 = ml->data[0]; // do not delete after use
    }
    if (copy) {
      if (!isart) {
        nodeindices = (int*)emalloc(n*sizeof(int));
        for (int i=0; i < n; ++i) {
          nodeindices[i] = ml->nodeindices[i];
        }
      }
      int nn = n*sz;
      for (int i = 0; i < nn; ++i) {
        data[i] = data1[i];
      }
      if (isart) {
        delete [] data1;
      }
    }else{
      data = data1;
    }

    sz = bbcore_dparam_size[type]; // nrn_prop_dparam_size off by 1 if cvode_ieq.
    if (sz) {
      int* pdata1;
      pdata1 = datum2int(type, ml, nt, cg, cg.datumindices[dsz_inst], vdata_offset);
      if (copy) {
        int nn = n*sz;
        for (int i=0; i < nn; ++i) {
          pdata[i] = pdata1[i];
        }
        delete [] pdata1;
      }else{
        pdata = pdata1;
      }
    }else{
      pdata = NULL;
    }

    return 1;
}

static int nrnthread_dat2_3(int tid, int nweight, int*& output_vindex, double*& output_threshold,
  int*& netcon_pnttype, int*& netcon_pntindex, double*& weights, double*& delays) {

  if (tid >= nrn_nthread) { return 0; }
  CellGroup& cg = cellgroups_[tid];
  NrnThread& nt = nrn_threads[tid];

  output_vindex = new int[cg.n_presyn];
  output_threshold = new double[cg.n_real_output];
  for (int i=0; i < cg.n_presyn; ++i) {
    output_vindex[i] = cg.output_vindex[i];
  }
  for (int i=0; i < cg.n_real_output; ++i) {
    output_threshold[i] = cg.output_ps[i] ? cg.output_ps[i]->threshold_ : 0.0;
  }

  // connections
  int n = cg.n_netcon;
  //printf("n_netcon=%d nweight=%d\n", n, nweight);
  netcon_pnttype = cg.netcon_pnttype; cg.netcon_pnttype = NULL;
  netcon_pntindex = cg.netcon_pntindex; cg.netcon_pntindex = NULL;
  // alloc a weight array and write netcon weights
  weights = new double[nweight];
  int iw = 0;
  for (int i=0; i < n; ++ i) {
    NetCon* nc = cg.netcons[i];
    for (int j=0; j < nc->cnt_; ++j) {
      weights[iw++] = nc->weight_[j];
    }
  }
  // alloc a delay array and write netcon delays
  delays = new double[n];
  for (int i=0; i < n; ++ i) {
    NetCon* nc = cg.netcons[i];
    delays[i] = nc->delay_;
  }

  return 1;
}

static int nrnthread_dat2_corepointer(int tid, int& n) {

  if (tid >= nrn_nthread) { return 0; }
  NrnThread& nt = nrn_threads[tid];

  n = 0;
  MlWithArt& mla = cellgroups_[tid].mlwithart;
  for (size_t i = 0; i < mla.size(); ++i) {
    if (nrn_bbcore_write_[mla[i].first]) {
      ++n;
    }
  }

  return 1;
}

static int nrnthread_dat2_corepointer_mech(int tid, int type,
 int& icnt, int& dcnt, int*& iArray, double*& dArray) {

  if (tid >= nrn_nthread) { return 0; }
  NrnThread& nt = nrn_threads[tid];
  CellGroup& cg = cellgroups_[tid];
  Memb_list* ml = cg.type2ml[type];

      dcnt = 0;
      icnt = 0;
      // data size and allocate
      for (int i = 0; i < ml->nodecount; ++i) {
        (*nrn_bbcore_write_[type])(NULL, NULL, &dcnt, &icnt, ml->data[i], ml->pdata[i], ml->_thread, &nt);
      }
      dArray = NULL;
      iArray = NULL;
      if (icnt)
      {
        iArray = new int[icnt];
      }
      if (dcnt)
      {
        dArray = new double[dcnt];
      }
      icnt = dcnt = 0;
      // data values
      for (int i = 0; i < ml->nodecount; ++i) {
        (*nrn_bbcore_write_[type])(dArray, iArray, &dcnt, &icnt, ml->data[i], ml->pdata[i], ml->_thread, &nt);
      }

  return 1;
}

void nrnthread_group_ids(int* grp) {
  for (int i = 0; i < nrn_nthread; ++i) {
    grp[i] = cellgroups_[i].group_id;
  }
}

void write_nrnthread(const char* path, NrnThread& nt, CellGroup& cg) {
  char fname[1000];
  if (cg.n_output <= 0) { return; }
  assert(cg.group_id >= 0);
  nrn_assert(snprintf(fname, 1000, "%s/%d_1.dat", path, cg.group_id) < 1000);
  FILE* f = fopen(fname, "wb");
  if (!f) {
    hoc_execerror("nrnbbcore_write write_nrnthread could not open for writing:", fname);
  }
  fprintf(f, "%s\n", bbcore_write_version);

  //nrnthread_dat1(int tid, int& n_presyn, int& n_netcon, int*& output_gid, int*& netcon_srcgid);
  fprintf(f, "%d npresyn\n", cg.n_presyn);
  fprintf(f, "%d nnetcon\n", cg.n_netcon);
  writeint(cg.output_gid, cg.n_presyn);
  writeint(cg.netcon_srcgid, cg.n_netcon);

  if (cg.output_gid) {delete [] cg.output_gid; cg.output_gid = NULL; }
  if (cg.netcon_srcgid) {delete [] cg.netcon_srcgid; cg.netcon_srcgid = NULL; }
  fclose(f);

  nrn_assert(snprintf(fname, 1000, "%s/%d_2.dat", path, cg.group_id) < 1000);
  f = fopen(fname, "w");
  if (!f) {
    hoc_execerror("nrnbbcore_write write_nrnthread could not open for writing:", fname);
  }

  fprintf(f, "%s\n", bbcore_write_version);

  // sizes and total data count
  int ngid, n_real_gid, nnode, ndiam, nmech, *tml_index, *ml_nodecount, nidata,
    nvdata, nweight;
  nrnthread_dat2_1(nt.id, ngid, n_real_gid, nnode, ndiam,
    nmech, tml_index, ml_nodecount, nidata, nvdata, nweight);

  fprintf(f, "%d ngid\n", ngid);
  fprintf(f, "%d n_real_gid\n", n_real_gid);
  fprintf(f, "%d nnode\n", nnode);
  fprintf(f, "%d ndiam\n", ndiam);
  fprintf(f, "%d nmech\n", nmech);

  for (int i=0; i < nmech; ++i) {
    fprintf(f, "%d\n", tml_index[i]);
    fprintf(f, "%d\n", ml_nodecount[i]);
  }
  delete [] tml_index;
  delete [] ml_nodecount;

  fprintf(f, "%d nidata\n", 0);
  fprintf(f, "%d nvdata\n", nvdata);
  fprintf(f, "%d nweight\n", nweight);

  // data
  int *v_parent_index=NULL; double *a=NULL, *b=NULL, *area=NULL, *v=NULL, *diamvec=NULL;
  nrnthread_dat2_2(nt.id, v_parent_index, a, b, area, v, diamvec);
  assert(cg.n_real_output == nt.ncell);
  writeint(nt._v_parent_index, nt.end);
  writedbl(nt._actual_a, nt.end);
  writedbl(nt._actual_b, nt.end);
  writedbl(nt._actual_area, nt.end);
  writedbl(nt._actual_v, nt.end);
  if (cg.ndiam) {
    writedbl(diamvec, nt.end);
    delete [] diamvec;
  }

  // mechanism data
  int dsz_inst = 0;
  MlWithArt& mla = cg.mlwithart;
  for (size_t i = 0; i < mla.size(); ++i) {
    int type = mla[i].first;
    int *nodeindices=NULL, *pdata=NULL; double* data=NULL;
    nrnthread_dat2_mech(nt.id, i, dsz_inst, nodeindices, data, pdata);
    Memb_list* ml = mla[i].second;
    int n = ml->nodecount;
    int sz = nrn_prop_param_size_[type];
    if (nodeindices) {
      writeint(nodeindices, n);
    }
    writedbl(data, n * sz);
    if (nrn_is_artificial_[type]) {
      delete [] data;
    }
    sz = bbcore_dparam_size[type];
    if (pdata) {
      ++dsz_inst;
      writeint(pdata, n * sz);
      delete [] pdata;
    }
  }

  int *output_vindex, *netcon_pnttype, *netcon_pntindex;
  double *output_threshold, *weights, *delays;
  nrnthread_dat2_3(nt.id, nweight, output_vindex, output_threshold,
    netcon_pnttype, netcon_pntindex, weights, delays);
  writeint(output_vindex, cg.n_presyn);
  writedbl(output_threshold, cg.n_real_output);
  delete [] output_threshold;

  // connections
  int n = cg.n_netcon;
//printf("n_netcon=%d nweight=%d\n", n, nweight);
  writeint(netcon_pnttype, n);
  delete [] netcon_pnttype;
  writeint(netcon_pntindex, n);
  delete [] netcon_pntindex;
  writedbl(weights, nweight);
  delete [] weights;
  writedbl(delays, n);
  delete [] delays;

  // special handling for BBCOREPOINTER
  // how many mechanisms require it
  nrnthread_dat2_corepointer(nt.id, n);
  fprintf(f, "%d bbcorepointer\n", n);
  // for each of those, what is the mech type and data size
  // and what is the data
  for (size_t i = 0; i < mla.size(); ++i) {
    int type = mla[i].first;
    if (nrn_bbcore_write_[type]) {
      int icnt, dcnt, *iArray; double* dArray;
      nrnthread_dat2_corepointer_mech(nt.id, type, icnt, dcnt, iArray, dArray);
      fprintf(f, "%d\n", type);
      fprintf(f, "%d\n%d\n", icnt, dcnt);
      if (icnt) {
        writeint(iArray, icnt);
        delete [] iArray;
      }
      if (dcnt) 
      {
        writedbl(dArray, dcnt);
        delete [] dArray;
      }
    }
  }

  nrnbbcore_vecplay_write(f, nt);

  fclose(f);
}


/** Write all dataset ids to files.dat.
 *
 * Format of the files.dat file is:
 *
 *     version string
 *     -1 (if model uses gap junction)
 *     n (number of datasets)
 *     id1
 *     id2
 *     ...
 *     idN
 */
void write_nrnthread_task(const char* path, CellGroup* cgs)
{
  // ids of datasets that will be created
  std::vector<int> iSend;

  // ignore empty nrnthread (has -1 id)
  for (int iInt = 0; iInt < nrn_nthread; ++iInt)
  {
    if ( cgs[iInt].group_id >= 0) {
        iSend.push_back(cgs[iInt].group_id);
    }
  }

  // receive and displacement buffers for mpi
  std::vector<int> iRecv, iDispl;

  if (nrnmpi_myid == 0)
  {
    iRecv.resize(nrnmpi_numprocs);
    iDispl.resize(nrnmpi_numprocs);
  }

  // number of datasets on the current rank
  int num_datasets = iSend.size();

#ifdef NRNMPI
  // gather number of datasets from each task
  if (nrnmpi_numprocs > 1) {
    nrnmpi_int_gather(&num_datasets, begin_ptr(iRecv), 1, 0);
  }else{
    iRecv[0] = num_datasets;
  }
#else
  iRecv[0] = num_datasets;
#endif

  // total number of datasets across all ranks
  int iSumThread = 0;

  // calculate mpi displacements
  if (nrnmpi_myid == 0)
  {
    for (int iInt = 0; iInt < nrnmpi_numprocs; ++iInt)
    {
      iDispl[iInt] = iSumThread;
      iSumThread += iRecv[iInt];
    }
  }

  // buffer for receiving all dataset ids
  std::vector<int> iRecvVec(iSumThread);

#ifdef NRNMPI
  // gather ids into the array with correspondent offsets
  if (nrnmpi_numprocs > 1) {
    nrnmpi_int_gatherv(begin_ptr(iSend), num_datasets, begin_ptr(iRecvVec), begin_ptr(iRecv), begin_ptr(iDispl), 0);
  }else{
    for (int iInt = 0; iInt < num_datasets; ++iInt)
    {
      iRecvVec[iInt] = iSend[iInt];
    }
  }
#else
  for (int iInt = 0; iInt < num_datasets; ++iInt)
  {
    iRecvVec[iInt] = iSend[iInt];
  }
#endif

  /// Writing the file with task, correspondent number of threads and list of correspondent first gids
  if (nrnmpi_myid == 0)
  {
    std::stringstream ss;
    ss << path << "/files.dat";

    std::string filename = ss.str();

    FILE *fp = fopen(filename.c_str(), "w");
    if (!fp) {
      hoc_execerror("nrnbbcore_write write_nrnthread_task could not open for writing:", filename.c_str());
    }

    fprintf(fp, "%s\n", bbcore_write_version);

    // notify coreneuron that this model involves gap junctions
    if (nrnthread_v_transfer_) {
      fprintf(fp, "-1\n");
    }

    // total number of datasets
    fprintf(fp, "%d\n", iSumThread);

    // write all dataset ids
    for (int i = 0; i < iRecvVec.size(); ++i)
    {
        fprintf(fp, "%d\n", iRecvVec[i]);
    }

    fclose(fp);
  }

}


int* datum2int(int type, Memb_list* ml, NrnThread& nt, CellGroup& cg, DatumIndices& di, int ml_vdata_offset) {
  int isart = nrn_is_artificial_[di.type];
  int sz = bbcore_dparam_size[type];
  int* pdata = new int[ml->nodecount * sz];
  for (int i=0; i < ml->nodecount; ++i) {
    Datum* d = ml->pdata[i];
    int ioff = i*sz;
    for (int j = 0; j < sz; ++j) {
      int jj = ioff + j;
      int etype = di.ion_type[jj];
      int eindex = di.ion_index[jj];
      if (etype == -1) {
        if (isart) {
          pdata[jj] = -1; // maybe save this space eventually. but not many of these in bb models
        }else{
          pdata[jj] = eindex;
        }
      }else if (etype == -9) {
        pdata[jj] = eindex;
      }else if (etype > 0 && etype < 1000){//ion pointer and also POINTER
        pdata[jj] = eindex;
      }else if (etype > 1000 && etype < 2000) { //ionstyle can be explicit instead of pointer to int*
        pdata[jj] = eindex;
      }else if (etype == -2) { // an ion and this is the iontype
        pdata[jj] = eindex;
      }else if (etype == -4) { // netsend (_tqitem)
        pdata[jj] = ml_vdata_offset + eindex;
        //printf("etype %d jj=%d eindex=%d pdata=%d\n", etype, jj, eindex, pdata[jj]);
      }else if (etype == -6) { // pntproc
        pdata[jj] = ml_vdata_offset + eindex;
        //printf("etype %d jj=%d eindex=%d pdata=%d\n", etype, jj, eindex, pdata[jj]);
      }else if (etype == -7) { // bbcorepointer
        pdata[jj] = ml_vdata_offset + eindex;
        //printf("etype %d jj=%d eindex=%d pdata=%d\n", etype, jj, eindex, pdata[jj]);
      }else if (etype == -5) { // POINTER to voltage
        pdata[jj] = eindex;
        //printf("etype %d\n", etype);
      }else{ //uninterpreted
        assert(eindex != -3); // avoided if last
        pdata[jj] = 0;
      }
    }
  }
  return pdata;
}


/** @brief Count number of unique elements in the array.
 *  there is a copy of the vector but we are primarily
 *  using it for small section list vectors.
 */
int count_distinct(double *data, int len) {
    if( len == 0)
        return 0;
    std::vector<double> v;
    v.assign(data, data + len);
    std::sort(v.begin(), v.end());
    return std::unique(v.begin(), v.end()) - v.begin();
}

/** @brief For BBP use case, we want to write section-segment
 *  mapping to gid_3.dat file. This information will be
 *  provided through neurodamus HOC interface with following
 *  format:
 *      gid : number of non-empty neurons in the cellgroup
 *      name : name of section list (like soma, axon, apic)
 *      nsec : number of sections
 *      sections : list of sections
 *      segments : list of segments
 */
void nrnbbcore_register_mapping() {

    // gid of a cell
    int gid = *hoc_getarg(1);

    // name of section list
    std::string name = std::string(hoc_gargstr(2));

    // hoc vectors: sections and segments
    Vect* sec = vector_arg(3);
    Vect* seg = vector_arg(4);

    double* sections  = vector_vec(sec);
    double* segments  = vector_vec(seg);

    int nsec = vector_capacity(sec);
    int nseg = vector_capacity(seg);

    if( nsec != nseg ) {
        std::cout << "Error: Section and Segment mapping vectors should have same size!\n";
        abort();
    }

    // number of unique sections
    nsec = count_distinct(sections, nsec);

    SecMapping *smap = new SecMapping(nsec, name);
    smap->sections.assign(sections, sections+nseg);
    smap->segments.assign(segments, segments+nseg);

    // store mapping information
    mapinfo.add_sec_mapping(gid, smap);
}

/** @brief dump mapping information to gid_3.dat file */
void nrn_write_mapping_info(const char *path, int gid, NrnMappingInfo &minfo) {

    /** full path of mapping file */
    std::stringstream ss;
    ss << path << "/" << gid << "_3.dat";

    std::string fname(ss.str());
    FILE *f = fopen(fname.c_str(), "w");

    if (!f) {
        hoc_execerror("nrnbbcore_write could not open for writing:", fname.c_str());
    }

    fprintf(f, "%s\n", bbcore_write_version);

    /** number of gids in NrnThread */
    fprintf(f, "%zd\n", minfo.size());

    /** all cells mapping information in NrnThread */
    for(size_t i = 0; i < minfo.size(); i++) {
        CellMapping *c = minfo.mapping[i];

        /** gid, #section, #compartments,  #sectionlists */
        fprintf(f, "%d %d %d %zd\n", c->gid, c->num_sections(), c->num_segments(), c->size());

        for(size_t j = 0; j < c->size(); j++) {
            SecMapping* s = c->secmapping[j];
            /** section list name, number of sections, number of segments */
            fprintf(f, "%s %d %zd\n", s->name.c_str(), s->nsec, s->size());

            /** section - segment mapping */
            if(s->size()) {
                writeint(&(s->sections.front()), s->size());
                writeint(&(s->segments.front()), s->size());
            }
        }
    }
    fclose(f);
}

typedef void*(*CNB)(...);
typedef struct core2nrn_callback_t {
  const char* name;
  CNB f;
} core2nrn_callback_t;

// from partrans.cpp
extern "C" {
extern void get_partrans_setup_info(int, int&, int&, int&, int&, int*&, int*&, int*&);
}

extern "C" {
void nrnthread_get_trajectory_requests(int tid, int& bsize, int& ntrajec, void**& vpr, int*& types, int*& indices, double**& pvars, double**& varrays);
void nrnthread_trajectory_values(int tid, int n_pr, void** vpr, double t);
void nrnthread_trajectory_return(int tid, int n_pr, int vecsz, void** vpr, double t);
}

static core2nrn_callback_t cnbs[]  = {
  {"nrn2core_group_ids_", (CNB)nrnthread_group_ids},
  {"nrn2core_mkmech_info_", (CNB)write_memb_mech_types_direct},
  {"nrn2core_get_global_dbl_item_", (CNB)get_global_dbl_item},
  {"nrn2core_get_global_int_item_", (CNB)get_global_int_item},
  {"nrn2core_get_partrans_setup_info_", (CNB)get_partrans_setup_info},
  {"nrn2core_get_dat1_", (CNB)nrnthread_dat1},
  {"nrn2core_get_dat2_1_", (CNB)nrnthread_dat2_1},
  {"nrn2core_get_dat2_2_", (CNB)nrnthread_dat2_2},
  {"nrn2core_get_dat2_mech_", (CNB)nrnthread_dat2_mech},
  {"nrn2core_get_dat2_3_", (CNB)nrnthread_dat2_3},
  {"nrn2core_get_dat2_corepointer_", (CNB)nrnthread_dat2_corepointer},
  {"nrn2core_get_dat2_corepointer_mech_", (CNB)nrnthread_dat2_corepointer_mech},
  {"nrn2core_get_dat2_vecplay_", (CNB)nrnthread_dat2_vecplay},
  {"nrn2core_get_dat2_vecplay_inst_", (CNB)nrnthread_dat2_vecplay_inst},
  {"nrn2core_part2_clean_", (CNB)part2_clean},

  {"nrn2core_get_trajectory_requests_", (CNB)nrnthread_get_trajectory_requests},
  {"nrn2core_trajectory_values_", (CNB)nrnthread_trajectory_values},
  {"nrn2core_trajectory_return_", (CNB)nrnthread_trajectory_return},
  {NULL, NULL}
};

extern int nrn_use_fast_imem;

/** check if file with given path exist */
bool file_exist(const std::string& path) {
  std::ifstream f(path.c_str());
  return f.good();
}

extern "C" {
  extern char* neuron_home;
}

#if defined(HAVE_DLFCN_H)
int nrncore_run(const char* arg) {
  corenrn_direct = true;

  // check that model can be transferred.
  model_ready();

  // name of coreneuron library based on platform
#if defined(MINGW)
  std::string corenrn_mechlib_name("libcorenrnmech.dll");
#elif defined(DARWIN)
  std::string corenrn_mechlib_name("libcorenrnmech.dylib");
#else
  std::string corenrn_mechlib_name("libcorenrnmech.so");
#endif

  // first check if coreneuron specific library exist in <arhc>/.libs
  std::stringstream s_path;
  s_path << NRNHOSTCPU << "/.libs/" << corenrn_mechlib_name;

  std::string path = s_path.str();
  const char* corenrn_lib = path.c_str();

  if (!file_exist(corenrn_lib)) {
    // otherwise, look for CORENEURONLIB env variable
    corenrn_lib = getenv("CORENEURONLIB");
    if (!corenrn_lib) {
      s_path.str("");
      // last fallback is minimal library with internal mechanisms
      s_path << neuron_home << "/../../lib/" << corenrn_mechlib_name;
      path = s_path.str();
      corenrn_lib = path.c_str();
    }
  }

  void* handle = dlopen(corenrn_lib, RTLD_NOW|RTLD_GLOBAL);
  if (!handle) {
    fputs(dlerror(), stderr);
    fputs("\n", stderr);
    hoc_execerror("Could not dlopen CoreNEURON mechanism library : ", corenrn_lib);
  }else{
    void* sym = dlsym(handle, "corenrn_version");
    if (!sym) {
      hoc_execerror("Could not get symbol corenrn_version from", corenrn_lib);
    }
    const char* cnver = (*(const char*(*)())sym)();
    if (strcmp(bbcore_write_version, cnver) != 0) {
      s_path.str("");
      s_path << bbcore_write_version << " and " << cnver;
      path = s_path.str();
      hoc_execerror("Incompatible NEURON and CoreNEURON data versions:", path.c_str());
    }
  }
  for (int i=0; cnbs[i].name; ++i) {
    void* sym = dlsym(handle, cnbs[i].name);
    if (!sym) {
      fprintf(stderr, "Could not get symbol %s in %s\n", cnbs[i].name, corenrn_lib);
      hoc_execerror("dlsym returned NULL", NULL);
    }
    void** c = (void**)sym;
    *c = (void*)(cnbs[i].f);
  }

  void* sym = dlsym(handle, "corenrn_embedded_run");
  if (!sym) {
    hoc_execerror("Could not get symbol corenrn_embedded_run from", corenrn_lib);
  }
  part1();
  int (*r)(int, int, int, int, const char*) = (int (*)(int, int, int, int, const char*))sym;
  int have_gap = nrnthread_v_transfer_ ? 1 : 0;
#if !NRNMPI
#define nrnmpi_use 0
#endif
  return r(nrn_nthread, have_gap, nrnmpi_use, nrn_use_fast_imem, arg);
}
#else
int nrncore_run(const char*) {
  return 0;
}
#endif //HAVE_DLFCN_H

} // end of extern "C"
