#include "cell_group.h"
#include "nrnran123.h" // globalindex written to globals.dat
#include "section.h"
#include "parse.hpp"
#include "nrnmpi.h"
#include "netcon.h"


extern short* nrn_is_artificial_;
extern bool corenrn_direct;
extern int* bbcore_dparam_size;
extern void nrncore_netpar_cellgroups_helper(CellGroup*);
extern int nrn_has_net_event_cnt_;
extern int* nrn_has_net_event_;
extern short* nrn_is_artificial_;

PVoid2Int CellGroup::artdata2index_;
Deferred_Type2ArtData CellGroup::deferred_type2artdata_;
int* CellGroup::has_net_event_;

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



CellGroup* CellGroup::mk_cellgroups(CellGroup* cgs) {
    for (int i = 0; i < nrn_nthread; ++i) {
        int ncell = nrn_threads[i].ncell; // real cell count
        int npre = ncell;
        MlWithArt &mla = cgs[i].mlwithart;
        for (size_t j = 0; j < mla.size(); ++j) {
            int type = mla[j].first;
            Memb_list *ml = mla[j].second;
            cgs[i].type2ml[type] = ml;
            if (nrn_has_net_event(type)) {
                npre += ml->nodecount;
            }
        }
        cgs[i].n_presyn = npre;
        cgs[i].n_real_output = ncell;
        cgs[i].output_ps = new PreSyn *[npre];
        cgs[i].output_gid = new int[npre];
        cgs[i].output_vindex = new int[npre];
        // in case some cells do not have voltage presyns (eg threshold detection
        // computed from a POINT_PROCESS NET_RECEIVE with WATCH and net_event)
        // initialize as unused.
        for (int j = 0; j < npre; ++j) {
            cgs[i].output_ps[j] = NULL;
            cgs[i].output_gid[j] = -1;
            cgs[i].output_vindex[j] = -1;
        }

        // fill in the artcell info
        npre = ncell;
        cgs[i].n_output = ncell; // add artcell (and PP with net_event) with gid in following loop
        for (size_t j = 0; j < mla.size(); ++j) {
            int type = mla[j].first;
            Memb_list *ml = mla[j].second;
            if (nrn_has_net_event(type)) {
                for (int j = 0; j < ml->nodecount; ++j) {
                    Point_process *pnt = (Point_process *) ml->pdata[j][1]._pvoid;
                    PreSyn *ps = (PreSyn *) pnt->presyn_;
                    cgs[i].output_ps[npre] = ps;
                    int agid = -1;
                    if (nrn_is_artificial_[type]) {
                        agid = -(type + 1000 * nrncore_art2index(pnt->prop->param));
                    } else { // POINT_PROCESS with net_event
                        int sz = nrn_prop_param_size_[type];
                        double *d1 = ml->data[0];
                        double *d2 = pnt->prop->param;
                        assert(d2 >= d1 && d2 < (d1 + (sz * ml->nodecount)));
                        int ix = (d2 - d1) / sz;
                        agid = -(type + 1000 * ix);
                    }
                    if (ps) {
                        if (ps->output_index_ >= 0) { // has gid
                            cgs[i].output_gid[npre] = ps->output_index_;
                            if (cgs[i].group_id < 0) {
                                cgs[i].group_id = ps->output_index_;
                            }
                            ++cgs[i].n_output;
                        } else {
                            cgs[i].output_gid[npre] = agid;
                        }
                    } else { // if an acell is never a source, it will not have a presyn
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
    if (corenrn_direct == false)
        for (int i = 0; i < nrn_nthread; ++i) {
            if (cgs[i].n_real_output && cgs[i].output_gid[0] >= 0) {
                cgs[i].group_id = cgs[i].output_gid[0];
            } else if (cgs[i].group_id >= 0) {
                // set above to first artificial cell with a ps->output_index >= 0
            } else {
                // Don't die yet as the thread may be empty. That just means no files
                // output for this thread and no mention in files.dat.
                // Can check for empty near end of datatransform(CellGroup* cgs)
            }
        }

    // use the Hoc NetCon object list to segregate according to threads
    // and fill the CellGroup netcons, netcon_srcgid, netcon_pnttype, and
    // netcon_pntindex (and, if nrn_nthread > 1, netcon_negsrcgid_tid).
    CellGroup::mk_cgs_netcon_info(cgs);

    return cgs;
}

void CellGroup::datumtransform(CellGroup* cgs) {
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
        // if model is being transferred via files, and
        //   if there are no gids in the thread (group_id < 0), and
        //     if the thread is not empty (mechanisms exist, n_mech > 0)
        if (corenrn_direct == false && cg.group_id < 0 && cg.n_mech > 0) {
            hoc_execerror("A nonempty thread has no real cell or ARTIFICIAL_CELL with a gid", NULL);
        }
    }
}



void CellGroup::datumindex_fill(int ith, CellGroup& cg, DatumIndices& di, Memb_list* ml) {
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
            }else if (dmap[j] == -10) { // fornetcon
                etype = -10;
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



// use the Hoc NetCon object list to segregate according to threads
// and fill the CellGroup netcons, netcon_srcgid, netcon_pnttype, and
// netcon_pntindex (called at end of mk_cellgroups);
void CellGroup::mk_cgs_netcon_info(CellGroup* cgs) {
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
                    if (nrn_nthread > 1) { // negative gid and multiple threads.
                        cgs[ith].netcon_negsrcgid_tid.push_back(ps->nt_->id);
                        // Raise error if file mode transfer and nc and ps not
                        // in same thread. In that case we cannot guarantee that
                        // the PreSyn will end up in the same coreneuron process.
                        if (!corenrn_direct && ith != ps->nt_->id) {
                            hoc_execerror("NetCon and NetCon source with no gid are not in the same thread", NULL);
                        }
                    }
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
void CellGroup::mk_tml_with_art(CellGroup* cgs) {
    // copy NrnThread tml list and append ARTIFICIAL cell types
    // but do not include PatternStim
    // Now using cgs[tid].mlwithart instead of
    // tml_with_art = new NrnThreadMembList*[nrn_nthread];
    // to allow fast retrieval of type and Memb_list* given index into the vector.

    // copy from NrnThread
    for (int id = 0; id < nrn_nthread; ++id) {
        MlWithArt& mla = cgs[id].mlwithart;
        for (NrnThreadMembList* tml = nrn_threads[id].tml; tml; tml = tml->next) {
            mla.push_back(MlWithArtItem(tml->index, tml->ml));
        }
    }
    int *acnt = new int[nrn_nthread];

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
                artdata2index_.insert(std::pair<double*, int>(ml->data[acnt[id]], acnt[id]));
                ++acnt[id];
            }
        }
    }
    delete [] acnt;
}

size_t CellGroup::get_mla_rankbytes(CellGroup* cellgroups_) {
    size_t mla_rankbytes=0;
    size_t nbytes;
    NrnThread* nt;
    NrnThreadMembList* tml;
    FOR_THREADS(nt)
    {
        size_t threadbytes = 0;
        size_t npnt = 0;
        size_t nart = 0;
        int ith = nt->id;
        //printf("rank %d thread %d\n", nrnmpi_myid, ith);
        //printf("  ncell=%d nnode=%d\n", nt->ncell, nt->end);
        //v_parent_index, _actual_a, _actual_b, _actual_area
        nbytes = nt->end * (1 * sizeof(int) + 3 * sizeof(double));
        threadbytes += nbytes;

        int mechcnt = 0;
        size_t mechcnt_instances = 0;
        MlWithArt &mla = cellgroups_[ith].mlwithart;
        for (size_t i = 0; i < mla.size(); ++i) {
            int type = mla[i].first;
            Memb_list *ml = mla[i].second;
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
            nbytes = ml->nodecount * (notart * sizeof(int) + 1 * sizeof(double *) +
                                      1 * sizeof(Datum * ) + psize * sizeof(double) + dpsize * sizeof(Datum));
            threadbytes += nbytes;
        }
        nbytes += npnt * (sizeof(Point_process) + sizeof(Prop));
        //printf("  mech in use %d  Point instances %ld  artcells %ld  total instances %ld\n",
        //mechcnt, npnt, nart, mechcnt_instances);
        //printf("  thread bytes %ld\n", threadbytes);
        mla_rankbytes += threadbytes;
    }
    return mla_rankbytes;
}

void CellGroup::clean_art(CellGroup* cgs) {
    // clean up the art Memb_list of CellGroup[].mlwithart
    // But if multithread and direct transfer mode, defer deletion of
    // data for artificial cells, so that the artificial cell ml->data
    // can be used when nrnthreads_type_return is called.
    if (corenrn_direct && nrn_nthread > 0) {
        deferred_type2artdata_.resize(nrn_nthread);
    }
    for (int ith=0; ith < nrn_nthread; ++ith) {
        MlWithArt& mla = cgs[ith].mlwithart;
        for (size_t i = 0; i < mla.size(); ++i) {
            int type = mla[i].first;
            Memb_list* ml = mla[i].second;
            if (nrn_is_artificial_[type]) {
                if (!deferred_type2artdata_.empty()) {
                    deferred_type2artdata_[ith][type] = {ml->nodecount, ml->data};
                }else{
                    delete [] ml->data;
                }
                delete [] ml->pdata;
                delete ml;
            }
        }
    }
}

void CellGroup::setup_nrn_has_net_event() {
    if (has_net_event_) {
        return;
    }

    has_net_event_ = new int[n_memb_func];
    for (int i=0; i < n_memb_func; ++i) {
        has_net_event_[i] = 0;
    }
    for(int i=0; i < nrn_has_net_event_cnt_; ++i) {
        has_net_event_[nrn_has_net_event_[i]] = 1;
    }
}
