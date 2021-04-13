#include "nrncore_callbacks.h"
#include "nrnconf.h"
#include "nrnmpi.h"
#include "section.h"
#include "netcon.h"
#include "hocdec.h"
#include "nrncore_write/data/cell_group.h"
#include "nrncore_write/io/nrncore_io.h"
#include "parse.h"
#include "nrnran123.h" // globalindex written to globals.
#include "netcvode.h" // for nrnbbcore_vecplay_write
#include "vrecitem.h" // for nrnbbcore_vecplay_write

#if defined(HAVE_DLFCN_H)
#include <dlfcn.h>
#endif

extern "C" {
extern bbcore_write_t* nrn_bbcore_write_;
extern short* nrn_is_artificial_;
extern bool corenrn_direct;
extern int* bbcore_dparam_size;
extern double nrn_ion_charge(Symbol*);
extern CellGroup* cellgroups_;
extern NetCvode* net_cvode_instance;
extern char* pnt_map;
};

/** Populate function pointers by mapping function pointers for callback */
void map_coreneuron_callbacks(void* handle) {
    for (int i=0; cnbs[i].name; ++i) {
        void* sym = NULL;
#if defined(HAVE_DLFCN_H)
        sym = dlsym(handle, cnbs[i].name);
#endif
        if (!sym) {
            fprintf(stderr, "Could not get symbol %s from CoreNEURON\n", cnbs[i].name);
            hoc_execerror("dlsym returned NULL", NULL);
        }
        void** c = (void**)sym;
        *c = (void*)(cnbs[i].f);
    }
}

void write_memb_mech_types_direct(std::ostream& s) {
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




// just for secondorder and Random123_globalindex and legacy units flag
int get_global_int_item(const char* name) {
    if (strcmp(name, "secondorder") == 0) {
        return secondorder;
    }else if(strcmp(name, "Random123_global_index") == 0) {
        return nrnran123_get_globalindex();
    }else if (strcmp(name, "_nrnunit_use_legacy_") == 0) {
        return _nrnunit_use_legacy_;
    }
    return 0;
}

// successively return global double info. Begin with p==NULL.
// Done when return NULL.
void* get_global_dbl_item(void* p, const char* & name, int& size, double*& val) {
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



/**
   Copy weights from all coreneuron::NrnThread to NetCon instances.
   This depends on the CoreNEURON weight order for each thread to be
   the same as was originally sent from NEURON.  See how that order
   was constructed in CellGroup::mk_cgs_netcon_info.
**/

void nrnthreads_all_weights_return(std::vector<double*>& weights) {
    std::vector<int> iw(nrn_nthread); // index for each thread
    Symbol* ncsym = hoc_lookup("NetCon");
    hoc_List* ncl = ncsym->u.ctemplate->olist;
    hoc_Item* q;
    ITERATE(q, ncl) {
        Object* ho = (Object*)VOIDITM(q);
        NetCon* nc = (NetCon*)ho->u.this_pointer;
        std::size_t ith = 0; // if no _vnt, put in thread 0
        if (nc->target_ && nc->target_->_vnt) {
            ith = std::size_t(((NrnThread*)(nc->target_->_vnt))->id);
        }
        for (int i = 0; i < nc->cnt_; ++i) {
            nc->weight_[i] = weights[ith][iw[ith]++];
        }
    }
}

/** @brief Return location for CoreNEURON to copy data into.
 *  The type is mechanism type or special negative type for voltage,
 *  i_membrane_, or time. See coreneuron/io/nrn_setup.cpp:stdindex2ptr.
 *  We allow coreneuron to copy to NEURON's AoS data as CoreNEURON knows
 *  how its data is arranged (SoA and possibly permuted).
 *  This function figures out the size (just for sanity check)
 *  and data pointer to be returned based on type and thread id.
 *  The ARTIFICIAL_CELL type case is special as there is no thread specific
 *  Memb_list for those.
 */
size_t nrnthreads_type_return(int type, int tid, double*& data, double**& mdata) {
    size_t n = 0;
    data = NULL;
    mdata = NULL;
    if (tid >= nrn_nthread) {
        return n;
    }
    NrnThread& nt = nrn_threads[tid];
    if (type == voltage) {
        data = nt._actual_v;
        n = size_t(nt.end);
    } else if (type == i_membrane_) { // i_membrane_
        data = nt._nrn_fast_imem->_nrn_sav_rhs;
        n = size_t(nt.end);
    } else if (type == 0) { // time
        data = &nt._t;
        n = 1;
    } else if (type > 0 && type < n_memb_func) {
        Memb_list* ml = nt._ml_list[type];
        if (ml) {
            mdata = ml->data;
            n = ml->nodecount;
        }else{
            // The single thread case is easy
            if (nrn_nthread == 1) {
                ml = memb_list + type;
                mdata = ml->data;
                n = ml->nodecount;
            }else{
                // mk_tml_with_art() created a cgs[id].mlwithart which appended
                // artificial cells to the end. Turns out that
                // cellgroups_[tid].type2ml[type]
                // is the Memb_list we need. Sadly, by the time we get here, cellgroups_
                // has already been deleted.  So we defer deletion of the necessary
                // cellgroups_ portion (deleting it on return from nrncore_run).
                auto& p = CellGroup::deferred_type2artdata_[tid][type];
                n = size_t(p.first);
                mdata = p.second;
            }
        }
    }
    return n;
}


void nrnthread_group_ids(int* grp) {
    for (int i = 0; i < nrn_nthread; ++i) {
        grp[i] = cellgroups_[i].group_id;
    }
}



int nrnthread_dat1(int tid, int& n_presyn, int& n_netcon,
                          int*& output_gid, int*& netcon_srcgid,
                          std::vector<int>& netcon_negsrcgid_tid) {

    if (tid >= nrn_nthread) { return 0; }
    CellGroup& cg = cellgroups_[tid];
    n_presyn = cg.n_presyn;
    n_netcon = cg.n_netcon;
    output_gid = cg.output_gid;  cg.output_gid = NULL;
    netcon_srcgid = cg.netcon_srcgid;  cg.netcon_srcgid = NULL;
    netcon_negsrcgid_tid = cg.netcon_negsrcgid_tid;
    return 1;
}

// sizes and total data count
int nrnthread_dat2_1(int tid, int& ngid, int& n_real_gid, int& nnode, int& ndiam,
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

int nrnthread_dat2_2(int tid, int*& v_parent_index, double*& a, double*& b,
                            double*& area, double*& v, double*& diamvec) {

    if (tid >= nrn_nthread) { return 0; }
    CellGroup& cg = cellgroups_[tid];
    NrnThread& nt = nrn_threads[tid];

    assert(cg.n_real_output == nt.ncell);

    // If direct transfer, copy, because target space already allocated
    bool copy = corenrn_direct;
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

int nrnthread_dat2_mech(int tid, size_t i, int dsz_inst, int*& nodeindices,
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

int nrnthread_dat2_3(int tid, int nweight, int*& output_vindex, double*& output_threshold,
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

int nrnthread_dat2_corepointer(int tid, int& n) {

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

int nrnthread_dat2_corepointer_mech(int tid, int type,
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

void part2_clean() {
    CellGroup::clear_artdata2index();

    CellGroup::clean_art(cellgroups_);

    delete [] cellgroups_;
    cellgroups_ = NULL;
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

int nrnthread_dat2_vecplay(int tid, std::vector<int>& indices) {
    if (tid >= nrn_nthread) { return 0; }
    NrnThread& nt = nrn_threads[tid];

    // add the index of each instance in fixed_play_ for thread tid.
    // error if not a VecPlayContinuous with no discon vector
    PlayRecList* fp = net_cvode_instance->fixed_play_;
    for (int i=0; i < fp->count(); ++i){
        if (fp->item(i)->type() == VecPlayContinuousType) {
            VecPlayContinuous* vp = (VecPlayContinuous*)fp->item(i);
            if (vp->discon_indices_ == NULL) {
                if (vp->ith_ == nt.id) {
                    assert(vp->y_ && vp->t_);
                    indices.push_back(i);
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

int nrnthread_dat2_vecplay_inst(int tid, int i, int& vptype, int& mtype,
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
