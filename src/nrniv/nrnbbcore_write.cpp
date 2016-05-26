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
// on the corebluron side. PreSyn ordering is a bit more delicate.
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
// See corebluron/src/simcore/nrniv/nrn_setup.cpp for a description of
// the file format written by this file.

#include <stdio.h>
#include <stdlib.h>
#include <section.h>
#include <parse.h>
#include <nrnmpi.h>
#include <netcon.h>
#include <nrnhash_alt.h>
#include <nrnbbcore_write.h>
#include <netcvode.h> // for nrnbbcore_vecplay_write
#include <vrecitem.h> // for nrnbbcore_vecplay_write
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

/* not NULL, need to write gap information */
extern void (*nrnthread_v_transfer_)(NrnThread*);
extern size_t nrnbbcore_gap_write(const char* path, int* group_ids);

typedef void (*bbcore_write_t)(double*, int*, int*, int*, double*, Datum*, Datum*, NrnThread*);
extern bbcore_write_t* nrn_bbcore_write_;

static CellGroup* mk_cellgroups(); // gid, PreSyn, NetCon, Point_process relation.
static void datumtransform(CellGroup*); // Datum.pval to int
static void datumindex_fill(int, CellGroup&, DatumIndices&, Memb_list*); //helper
static void write_memb_mech_types(const char* fname);
static void write_globals(const char* fname);
static void write_nrnthread(const char* fname, NrnThread& nt, CellGroup& cg);
static void write_nrnthread_task(const char*, CellGroup* cgs);
static int* datum2int(int type, Memb_list* ml, NrnThread& nt, CellGroup& cg, DatumIndices& di, int* ml_data_offset, int* ml_vdata_offset);
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
static NrnThreadMembList** tml_with_art; // nrn_nthread of them
static void mk_tml_with_art(void);

declareNrnHash(PVoid2Int, void*, int)
implementNrnHash(PVoid2Int, void*, int)
PVoid2Int* artdata2index_;

// accessible from ParallelContext.total_bytes()
size_t nrnbbcore_write() {
  if (!use_cachevec) {
    hoc_execerror("nrnbbcore_write requires cvode.cache_efficient(1)", NULL);
  }
  NrnThread* nt;
  NrnThreadMembList* tml;
  if (!bbcore_dparam_size) {
    bbcore_dparam_size = new int[n_memb_func];
  }
  char fname[1024];
  char path[1024];
  sprintf(path, ".");
  if (ifarg(1)) {
    strcpy(path, hoc_gargstr(1));
  }
  for (int i=0; i < n_memb_func; ++i) {
    int sz = nrn_prop_dparam_size_[i];
    bbcore_dparam_size[i] = sz;
    Memb_func* mf = memb_func + i;
    if (mf && mf->dparam_semantics && sz && mf->dparam_semantics[sz-1] == -3) {
        bbcore_dparam_size[i] = sz - 1;
    }
  }
  mk_tml_with_art();
  sprintf(fname, "%s/%s", path, "bbcore_mech.dat");
  write_memb_mech_types(fname);

  sprintf(fname, "%s/%s", path, "globals.dat");
  write_globals(fname);

  size_t rankbytes = 0;
  size_t nbytes;
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
    for (tml = tml_with_art[ith]; tml; tml = tml->next) {
      ++mechcnt;
      Memb_list* ml = tml->ml;
      mechcnt_instances += ml->nodecount;
      npnt += (memb_func[tml->index].is_point ? ml->nodecount : 0);
      int psize = nrn_prop_param_size_[tml->index];
      int dpsize = nrn_prop_dparam_size_[tml->index]; // includes cvodeieq if present
      //printf("%d %s ispnt %d  cnt %d  psize %d  dpsize %d\n",tml->index, memb_func[tml->index].sym->name,
      //memb_func[tml->index].is_point, ml->nodecount, psize, dpsize);
      // nodeindices, data, pdata + pnt with prop
      int notart = nrn_is_artificial_[tml->index] ? 0 : 1;
      if (nrn_is_artificial_[tml->index]) {
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
  for (int i=0; i < nrn_nthread; ++i) {
    chkpnt = 0;
    write_nrnthread(path, nrn_threads[i], cgs[i]);
  }
  if (nrnthread_v_transfer_) {
    // see partrans.cpp. nrn_nthread files of path/icg_gap.dat
    int* group_ids = new int[nrn_nthread];
    for (int i=0; i < nrn_nthread; ++i) {
      group_ids[i] = (cgs[i].n_output > 0) ? cgs[i].output_gid[0] : -1;
    }
    nrnbbcore_gap_write(path, group_ids);
    delete [] group_ids;
  }
  if (artdata2index_) {
    delete artdata2index_;
    artdata2index_ = NULL;
  }

  // filename data might have to be collected at hoc level since
  // pc.nrnbbcore_write might be called
  // many times per rank since model may be built as series of submodels.
  if (ifarg(2)) {
    Vect* cgidvec = vector_arg(2);
    vector_resize(cgidvec, nrn_nthread);
    double* px = vector_vec(cgidvec);
    for (int i=0; i < nrn_nthread; ++i) {
      px[i] = (cgs[i].n_output > 0) ? double(cgs[i].output_gid[0]) : -1;
    }
  }else{
    write_nrnthread_task(path, cgs);
  }

  if (tml_with_art) {
    for (int ith=0; ith < nrn_nthread; ++ith) {
      NrnThreadMembList* tmlnext;
      for (tml = tml_with_art[ith]; tml; tml = tmlnext) {
        tmlnext = tml->next;
        delete tml;
      }
    }
    tml_with_art = NULL;
  }

  return rankbytes;
}

int nrncore_art2index(double* d) {
  int i;
  assert(artdata2index_->find(d, i));
  return i;
}

void mk_tml_with_art() {
  // copy NrnThread tml list and append ARTIFICIAL cell types 
  // but do not include PatternStim
  tml_with_art = new NrnThreadMembList*[nrn_nthread];
  // copy from NrnThread
  NrnThreadMembList *tml1, *tml2;
  NrnThreadMembList** tail = new NrnThreadMembList*[nrn_nthread];
  for (int id = 0; id < nrn_nthread; ++id) {
    tml2 = tml_with_art[id] = NULL;
    for (tml1 = nrn_threads[id].tml; tml1; tml1 = tml1->next) {
      if (tml_with_art[id] == NULL) {
        tml2 = new NrnThreadMembList;
        tml_with_art[id] = tml2;
      }else{
        tml2->next = new NrnThreadMembList;
        tml2 = tml2->next;
      }
      tml2->index = tml1->index;
      tml2->ml = tml1->ml;
      tml2->next = NULL;
    }
    tail[id] = tml2;
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
          NrnThreadMembList* tml3 = new NrnThreadMembList;
          tml3->next = NULL;
          tml3->index = i;
          tml3->ml = new Memb_list;
          tml3->ml->nodecount = acnt[id];
          tml3->ml->nodelist = NULL;
          tml3->ml->nodeindices = NULL;
          tml3->ml->prop = NULL;
          tml3->ml->_thread = NULL;
          tml3->ml->data = new double*[acnt[id]];
          tml3->ml->pdata = new Datum*[acnt[id]];
          // link at tail
          if (tml_with_art[id] == NULL) {
            tml_with_art[id] = tail[id] = tml3;
          }else{
            tail[id]->next = tml3;
            tail[id] = tml3;
          }
        }
      }
      // fill data and pdata pointers
      // and fill the artdata2index hash table
      for (int id = 0; id < nrn_nthread; ++id) {acnt[id] = 0;}
      for (int j = 0; j < memb_list[i].nodecount; ++j) {
        Point_process* pnt = (Point_process*)memb_list[i].pdata[j][1]._pvoid;
        int id = ((NrnThread*)pnt->_vnt)->id;
        assert(tail[id]->index == i);
        Memb_list* ml = tail[id]->ml;
        ml->data[acnt[id]] = memb_list[i].data[j];
        ml->pdata[acnt[id]] = memb_list[i].pdata[j];
        artdata2index_->insert(ml->data[acnt[id]], acnt[id]);
        ++acnt[id];
      }
    }
  }
  delete [] tail;
  delete [] acnt;
}

CellGroup::CellGroup() {
  n_output = n_real_output = n_presyn = n_netcon = n_mech = ntype = 0;
  output_gid = output_vindex = 0;
  netcons = 0; output_ps = 0;
  netcon_srcgid = netcon_pnttype = netcon_pntindex = 0;
  datumindices = 0;
  type2ml = new Memb_list*[n_memb_func];
  for (int i=0; i < n_memb_func; ++i) {
    type2ml[i] = 0;
  }
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
          assert(nrn_is_artificial_[type]);
          int ix = nrncore_art2index(pnt->prop->param);
          cgs[ith].netcon_srcgid[i] = -(type + 1000*ix);
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
  CellGroup* cgs = new CellGroup[nrn_nthread];
  for (int i=0; i < nrn_nthread; ++i) {
    int ncell = nrn_threads[i].ncell; // real cell count
    int npre = ncell;
    for (NrnThreadMembList* tml = tml_with_art[i]; tml; tml = tml->next) {
      cgs[i].type2ml[tml->index] = tml->ml;
      if (nrn_is_artificial_[tml->index]) {
        npre += tml->ml->nodecount;
      }
    }
    cgs[i].n_presyn = npre;
    cgs[i].n_real_output = ncell;
    cgs[i].output_ps = new PreSyn*[npre];
    cgs[i].output_gid = new int[npre];
    cgs[i].output_vindex = new int[npre];
    // fill in the artcell info
    npre = ncell;
    cgs[i].n_output = ncell; // add artcell with gid in following loop
    for (NrnThreadMembList* tml = tml_with_art[i]; tml; tml = tml->next) {
      if (nrn_is_artificial_[tml->index]) {
        for (int j=0; j < tml->ml->nodecount; ++j) {
          Point_process* pnt = (Point_process*)tml->ml->pdata[j][1]._pvoid;
          PreSyn* ps = (PreSyn*)pnt->presyn_;
          cgs[i].output_ps[npre] = ps;
          int agid = -(tml->index + 1000*nrncore_art2index(pnt->prop->param));
          if (ps) {
            if (ps->output_index_ >= 0) { // has gid
              cgs[i].output_gid[npre] = ps->output_index_;
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
    for (NrnThreadMembList* tml = tml_with_art[ith]; tml; tml = tml->next) {
      ++cg.n_mech;
      if (tml->ml->pdata[0]) {
        ++cg.ntype;
      }
    }
    cg.datumindices = new DatumIndices[cg.ntype];
    // specify type, allocate the space, and fill the indices
    int i=0;
    for (NrnThreadMembList* tml = tml_with_art[ith]; tml; tml = tml->next) {
      int sz = bbcore_dparam_size[tml->index];
      if (sz) {
        DatumIndices& di = cg.datumindices[i++];
        di.type = tml->index;
        Memb_list* ml = tml->ml;
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
      }else if (dmap[j] == -5) { // POINTER
        // must be a pointer into nt->_data. Handling is similar to eion so
        // give proper index into the type.
        double* pd = dparam[j].pval;
        etype = 0;
        if (pd >= nt._actual_v && pd < (nt._actual_v + nnode)) {
          etype = -5; // signifies an index into voltage array portion of _data 
          eindex = pd - nt._actual_v;
        }else{
          for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
            if (nrn_is_artificial_[tml->index]) { continue; }
            Memb_list* ml1 = tml->ml;
            int nn = nrn_prop_param_size_[tml->index] * ml1->nodecount;
            if (pd >= ml1->data[0] && pd < (ml1->data[0] + nn)) {
              etype = tml->index;
              eindex = pd - ml1->data[0];
              break;
            }
          }
          fprintf(stderr, "POINTER is not pointing to voltage or mechanism data. Perhaps it should be a BBCOREPOINTER\n");
          assert(etype != 0);
        }
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
      }
      di.ion_type[offset + j] = etype;
      di.ion_index[offset + j] = eindex;
    }
  }
}

static void write_memb_mech_types(const char* fname) {
  // list of Memb_func names, types, point type info, is_ion
  // and data, pdata instance sizes. If the mechanism is an eion type,
  // the following line is the charge.
  // Not all Memb_func are necessarily used in the model.
  if (nrnmpi_myid > 0) { return; } // only rank 0 writes this file
  FILE* f = fopen(fname, "wb");
  if (!f) {
    hoc_execerror("nrnbbcore_write write_mem_mech_types could not open for writing: %s\n", fname);
  }
  fprintf(f, "%d\n", n_memb_func);
  for (int type=2; type < n_memb_func; ++type) {
    Memb_func& mf = memb_func[type];
    fprintf(f, "%s %d %d %d %d %d %d\n", mf.sym->name, type,
      pnt_map[type], // the pointtype, 0 means not a POINT_PROCESS
      nrn_is_artificial_[type],
      nrn_is_ion(type),
      nrn_prop_param_size_[type], bbcore_dparam_size[type]
    );
    if (nrn_is_ion(type)) {
        fprintf(f, "%g\n", nrn_ion_charge(mf.sym));
    }
  }

  // tack on an endian sentinal value so reader can determine if byteswap needed.
  int32_t x = 1.0;
  fwrite(&x, sizeof(int32_t), 1, f);

  fclose(f);
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

  for (Symbol* sp = hoc_built_in_symlist->first; sp; sp = sp->next) {
    if (sp->type == VAR && sp->subtype == USERDOUBLE) {
      if (ISARRAY(sp)) {
        Arrayinfo* a = sp->arayinfo;
        if (a->nsub == 1) {
          fprintf(f, "%s[%d]\n", sp->name, a->sub[0]);
          for (int i=0; i < a->sub[0]; ++i) {
            char n[256];
            sprintf(n, "%s[%d]", sp->name, i);
            fprintf(f, "%.20g\n", *hoc_val_pointer(n));
          }
        }
      }else{
        fprintf(f, "%s %.20g\n", sp->name, *sp->u.pval);
      }
    }
  }
  fprintf(f, "0 0\n"); 

  fclose(f);
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

// Vector.play information.
// Must play into a data element in this thread
// File format is # of play instances in this thread (generally VecPlayContinuous)
// For each Play instance
// VecPlayContinuousType (4), pd (index), y.size, yvec, tvec
// Other VecPlay instance types are possible, such as VecPlayContinuous with
// a discon vector or VecPlayStep with a DT or tvec, but are not implemented
// at present. Assertion errors are generated if not type 0 of if we
// cannot determine the index into the NrnThread._data .
static void nrnbbcore_vecplay_write(FILE* f, NrnThread& nt, int* ml_data_offset) {
  // count the instances for this thread
  // error if not a VecPlayContinuous with no discon vector
  int n = 0;
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
  fprintf(f, "%d VecPlay instances\n", n);
  for (int i=0; i < fp->count(); ++i) {
    if (fp->item(i)->type() == VecPlayContinuousType) {
      VecPlayContinuous* vp = (VecPlayContinuous*)fp->item(i);
      if (vp->discon_indices_ == NULL) {
        if (vp->ith_ == nt.id) {
          double* pd = vp->pd_;
          int found = 0;
          fprintf(f, "%d\n", vp->type());
          for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
            if (nrn_is_artificial_[tml->index]) { continue; }
            Memb_list* ml = tml->ml;
            int nn = nrn_prop_param_size_[tml->index] * ml->nodecount;
            if (pd >= ml->data[0] && pd < (ml->data[0] + nn)) {
              int ix = (pd - ml->data[0]);
              int sz = vector_capacity(vp->y_);
              fprintf(f, "%d\n", tml->index);
              fprintf(f, "%d\n", ix);
              fprintf(f, "%d\n", sz);
              writedbl(vector_vec(vp->y_), sz);
              writedbl(vector_vec(vp->t_), sz);
              found = 1;
              break;
            }
          }
          assert(found);
        }
      }
    }
  }
}

void write_nrnthread(const char* path, NrnThread& nt, CellGroup& cg) {
  char fname[1000];
  if (cg.n_output <= 0) { return; }
  assert(cg.output_gid[0] >= 0);
  sprintf(fname, "%s/%d_1.dat", path, cg.output_gid[0]);
  FILE* f = fopen(fname, "wb");
  if (!f) {
    hoc_execerror("nrnbbcore_write write_nrnthread could not open for writing:", fname);
  }
  fprintf(f, "%d npresyn\n", cg.n_presyn);
  fprintf(f, "%d nnetcon\n", cg.n_netcon);
  writeint(cg.output_gid, cg.n_presyn);
  writeint(cg.netcon_srcgid, cg.n_netcon);
  fclose(f);

  sprintf(fname, "%s/%d_2.dat", path, cg.output_gid[0]);
  f = fopen(fname, "w");
  if (!f) {
    hoc_execerror("nrnbbcore_write write_nrnthread could not open for writing:", fname);
  }

  NrnThreadMembList* tml;
  // sizes and data offset
  int* ml_data_offset = new int[n_memb_func];
  int* ml_vdata_offset = new int[n_memb_func];
  int data_offset = 6*nt.end;
  int vdata_offset = 0;
  for (int i=0; i < n_memb_func; ++i) {
    ml_data_offset[i] = -1; // for nt._data
    ml_vdata_offset[i] = -1; // for nt._vdata
  }
  fprintf(f, "%d ngid\n", cg.n_output);
  fprintf(f, "%d n_real_gid\n", cg.n_real_output);
  fprintf(f, "%d nnode\n", nt.end);
  fprintf(f, "%d nmech\n", cg.n_mech);

  for (tml=tml_with_art[nt.id]; tml; tml = tml->next) {
    fprintf(f, "%d\n", tml->index);
    fprintf(f, "%d\n", tml->ml->nodecount);
    ml_data_offset[tml->index] = data_offset;
    data_offset += tml->ml->nodecount*nrn_prop_param_size_[tml->index];
    ml_vdata_offset[tml->index] = vdata_offset;
    int* ds = memb_func[tml->index].dparam_semantics;
    for (int psz=0; psz < bbcore_dparam_size[tml->index]; ++psz) {
      if (ds[psz] == -4 || ds[psz] == -6 || ds[psz] == -7 || ds[psz] == 0) {
        //printf("%s ds[%d]=%d vdata_offset=%d\n", memb_func[tml->index].sym->name, psz, ds[psz], vdata_offset);
        vdata_offset += tml->ml->nodecount;
      }
    }
  }
  //  printf("ndata=%d  nidata=%d nvdata=%d nnetcon=%d\n", data_offset, 0, vdata_offset, cg.n_netcon);
  fprintf(f, "%d ndata\n", data_offset);
  fprintf(f, "%d nidata\n", 0);
  fprintf(f, "%d nvdata\n", vdata_offset);
  int nweight = 0;
  for (int i=0; i < cg.n_netcon; ++i) {
    nweight += cg.netcons[i]->cnt_;
  }
  fprintf(f, "%d nweight\n", nweight);
  // data
  assert(cg.n_real_output == nt.ncell);
  writeint(nt._v_parent_index, nt.end);
  writedbl(nt._actual_a, nt.end);
  writedbl(nt._actual_b, nt.end);
  writedbl(nt._actual_area, nt.end);
  writedbl(nt._actual_v, nt.end);
  int id = 0;
  for (tml=tml_with_art[nt.id]; tml; tml=tml->next) {
    Memb_list* ml = tml->ml;
    int isart = nrn_is_artificial_[tml->index];
    int n = ml->nodecount;
    int sz = nrn_prop_param_size_[tml->index];
    if (isart) { // data may not be contiguous
      write_contiguous_art_data(ml->data, n, sz, f);
    }else{
      writeint(ml->nodeindices, n);
      writedbl(ml->data[0], n * sz);
    }
    sz = bbcore_dparam_size[tml->index];

    if (sz) {
      int* pdata = datum2int(tml->index, ml, nt, cg, cg.datumindices[id], ml_data_offset, ml_vdata_offset);
      writeint(pdata, n * sz);
      ++id;
      delete [] pdata;
    }
  }
  writeint(cg.output_vindex, cg.n_presyn);
  double* output_threshold = new double[cg.n_real_output];
  for (int i=0; i < cg.n_real_output; ++i) {
    output_threshold[i] = cg.output_ps[i]->threshold_;
  }
  writedbl(output_threshold, cg.n_real_output);
  delete [] output_threshold;

  // connections
  int n = cg.n_netcon;
//printf("n_netcon=%d nweight=%d\n", n, nweight);
  writeint(cg.netcon_pnttype, n);
  writeint(cg.netcon_pntindex, n);
  // alloc a weight array and write netcon weights
  double* weights = new double[nweight];
  int iw = 0;
  for (int i=0; i < n; ++ i) {
    NetCon* nc = cg.netcons[i];
    for (int j=0; j < nc->cnt_; ++j) {
      weights[iw++] = nc->weight_[j];
    }
  }
  writedbl(weights, nweight);
  delete [] weights;
  // alloc a delay array and write netcon delays
  double* delays = new double[n];
  for (int i=0; i < n; ++ i) {
    NetCon* nc = cg.netcons[i];
    delays[i] = nc->delay_;
  }
  writedbl(delays, n);
  delete [] delays;

  // special handling for BBCOREPOINTER
  // how many mechanisms require it
  n = 0;
  for (tml=tml_with_art[nt.id]; tml; tml = tml->next) {
    if (nrn_bbcore_write_[tml->index]) {
      ++n;
    }
  }
  fprintf(f, "%d bbcorepointer\n", n);
  // for each of those, what is the mech type and data size
  // and what is the data
  for (tml=tml_with_art[nt.id]; tml; tml = tml->next) {
    if (nrn_bbcore_write_[tml->index]) {
      fprintf(f, "%d\n", tml->index);
      Memb_list* ml = tml->ml;
      int dcnt = 0;
      int icnt = 0;
      // data size and allocate
      for (int i = 0; i < ml->nodecount; ++i) {
        (*nrn_bbcore_write_[tml->index])(NULL, NULL, &dcnt, &icnt, ml->data[i], ml->pdata[i], ml->_thread, &nt);
      }
      fprintf(f, "%d\n%d\n", icnt, dcnt);
      double* dArray = NULL;
      int* iArray = NULL;
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
        (*nrn_bbcore_write_[tml->index])(dArray, iArray, &dcnt, &icnt, ml->data[i], ml->pdata[i], ml->_thread, &nt);
      }
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

  nrnbbcore_vecplay_write(f, nt, ml_data_offset);

  fclose(f);
  delete [] ml_data_offset;
  delete [] ml_vdata_offset;
}


/// Writing the number of threads per MPI task
void write_nrnthread_task(const char* path, CellGroup* cgs)
{
  int *iRecv, *iRecvVec, *iDispl;
  if (nrnmpi_myid == 0)
  {
    iRecv = new int[nrnmpi_numprocs];
    iDispl = new int[nrnmpi_numprocs];
  }

  int iSumThread = 0;
  int *iSend = new int[nrn_nthread];
#ifdef NRNMPI
  /// Getting the number of threads for each task
  if (nrnmpi_numprocs > 1) {
    nrnmpi_int_gather(&nrn_nthread, iRecv, 1, 0);
  }else{
    iRecv[0] = nrn_nthread;
  }
#else
  iRecv[0] = nrn_nthread;
#endif

  if (nrnmpi_myid == 0)
  {
    for (int iInt = 0; iInt < nrnmpi_numprocs; ++iInt)
    {
      iDispl[iInt] = iSumThread;
      iSumThread += iRecv[iInt];
    }
    iRecvVec = new int[iSumThread];
  }

  for (int iInt = 0; iInt < nrn_nthread; ++iInt) 
  {  
    iSend[iInt] = cgs[iInt].output_gid[0];
  }

#ifdef NRNMPI
  /// Getting all the first gids (per thread) into the array with correspondent offsets
  if (nrnmpi_numprocs > 1) {
    nrnmpi_int_gatherv(iSend, nrn_nthread, iRecvVec, iRecv, iDispl, 0);
  }else{
    for (int iInt = 0; iInt < nrn_nthread; ++iInt)
    {
      iRecvVec[iInt] = iSend[iInt];
    }
  }
#else
  for (int iInt = 0; iInt < nrn_nthread; ++iInt)
  {
    iRecvVec[iInt] = iSend[iInt];
  }
#endif

  /// Writing the file with task, correspondent number of threads and list of correspondent first gids
  if (nrnmpi_myid == 0)
  {
    char fname[1000];
    sprintf(fname, "%s/%s", path, "files.dat");
    FILE *fp = fopen(fname, "w");
    if (!fp) {
      hoc_execerror("nrnbbcore_write write_nrnthread_task could not open for writing:", fname);
    }
    // temporary? expedient to notify coreneuron that this model involves
    // gap junctions
    if (nrnthread_v_transfer_) {
      fprintf(fp, "-1\n");
    }
    fprintf(fp, "%d\n", iSumThread);
    for (int iInt = 0; iInt < nrnmpi_numprocs; ++iInt)
    {
      //fprintf(fp, "%d\t%d\n", iInt, iRecv[iInt]);
      for (int jInt = 0; jInt < iRecv[iInt]; ++jInt)
      {
        fprintf(fp, "%d\n", iRecvVec[jInt+iDispl[iInt]]);
      }
    }
    fclose(fp);
  }

  delete [] iSend;
  if (nrnmpi_myid == 0)
  {
    delete [] iRecv;
    delete [] iDispl;
    delete [] iRecvVec;
  }
}


int* datum2int(int type, Memb_list* ml, NrnThread& nt, CellGroup& cg, DatumIndices& di, int* ml_data_offset, int* ml_vdata_offset) {
  int isart = nrn_is_artificial_[di.type];
  int sz = bbcore_dparam_size[type];
  int* pdata = new int[ml->nodecount * sz];
  int area_offset = 5*nt.end;
  int volt_offset = 4*nt.end;
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
          pdata[jj] = area_offset + eindex;
        }
      }else if (etype > 0 && etype < 1000){//ion pointer and also POINTER
        pdata[jj] = ml_data_offset[etype] + eindex;
      }else if (etype > 1000 && etype < 2000) { //ionstyle can be explicit instead of pointer to int*
        pdata[jj] = eindex;
      }else if (etype == -2) { // an ion and this is the iontype
        pdata[jj] = eindex;
      }else if (etype == -4) { // netsend (_tqitem)
        pdata[jj] = ml_vdata_offset[type] + eindex;
        //printf("etype %d jj=%d eindex=%d pdata=%d\n", etype, jj, eindex, pdata[jj]);
      }else if (etype == -6) { // pntproc
        pdata[jj] = ml_vdata_offset[type] + eindex;
        //printf("etype %d jj=%d eindex=%d pdata=%d\n", etype, jj, eindex, pdata[jj]);
      }else if (etype == -7) { // bbcorepointer
        pdata[jj] = ml_vdata_offset[type] + eindex;
        //printf("etype %d jj=%d eindex=%d pdata=%d\n", etype, jj, eindex, pdata[jj]);
      }else if (etype == -5) { // POINTER to voltage
        pdata[jj] = volt_offset + eindex;
        //printf("etype %d\n", etype);
      }else{ //uninterpreted
        assert(eindex != -3); // avoided if last
        pdata[jj] = 0;
      }
    }
  }
  return pdata;
}

} // end of extern "C"
