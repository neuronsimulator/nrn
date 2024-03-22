#include <vector>
#include <unordered_map>
#include "nrncore_callbacks.h"
#include "nrnconf.h"
#include "nrnmpi.h"
#include "section.h"
#include "netcon.h"
#include "nrncvode.h"
#include "nrniv_mf.h"
#include "hocdec.h"
#include "nrncore_write/utils/nrncore_utils.h"
#include "nrncore_write/data/cell_group.h"
#include "nrncore_write/io/nrncore_io.h"
#include "parse.hpp"
#include "nrnran123.h"  // globalindex written to globals.
#include "netcvode.h"   // for nrnbbcore_vecplay_write and PreSyn.flag_
extern TQueue* net_cvode_instance_event_queue(NrnThread*);
#include "vrecitem.h"  // for nrnbbcore_vecplay_write

#include "nrnwrap_dlfcn.h"

extern bbcore_write_t* nrn_bbcore_write_;
extern bbcore_write_t* nrn_bbcore_read_;
extern short* nrn_is_artificial_;
extern bool corenrn_direct;
extern int* bbcore_dparam_size;
extern double nrn_ion_charge(Symbol*);
extern CellGroup* cellgroups_;
extern NetCvode* net_cvode_instance;
extern char* pnt_map;
extern void* nrn_interthread_enqueue(NrnThread*);

/** Populate function pointers by mapping function pointers for callback */
void map_coreneuron_callbacks(void* handle) {
    for (int i = 0; cnbs[i].name; ++i) {
        void* sym = NULL;
#if defined(HAVE_DLFCN_H)
        sym = dlsym(handle, cnbs[i].name);
#endif
        if (!sym) {
            fprintf(stderr, "Could not get symbol %s from CoreNEURON\n", cnbs[i].name);
            hoc_execerror("dlsym returned NULL", NULL);
        }
        void** c = (void**) sym;
        *c = (void*) (cnbs[i].f);
    }
}

void write_memb_mech_types_direct(std::ostream& s) {
    // list of Memb_func names, types, point type info, is_ion
    // and data, pdata instance sizes. If the mechanism is an eion type,
    // the following line is the charge.
    // Not all Memb_func are necessarily used in the model.
    s << bbcore_write_version << std::endl;
    s << n_memb_func << std::endl;
    for (int type = 2; type < n_memb_func; ++type) {
        const char* w = " ";
        Memb_func& mf = memb_func[type];
        s << mf.sym->name << w << type << w << int(pnt_map[type])
          << w  // the pointtype, 0 means not a POINT_PROCESS
          << nrn_is_artificial_[type] << w << nrn_is_ion(type) << w << nrn_prop_param_size_[type]
          << w << bbcore_dparam_size[type] << std::endl;

        if (nrn_is_ion(type)) {
            s << nrn_ion_charge(mf.sym) << std::endl;
        }
    }
}


// just for secondorder and Random123_globalindex and legacy units flag
int get_global_int_item(const char* name) {
    if (strcmp(name, "secondorder") == 0) {
        return secondorder;
    } else if (strcmp(name, "Random123_global_index") == 0) {
        return nrnran123_get_globalindex();
    }
    return 0;
}

// successively return global double info. Begin with p==NULL.
// Done when return NULL.
void* get_global_dbl_item(void* p, const char*& name, int& size, double*& val) {
    Symbol* sp = (Symbol*) p;
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
                    for (int i = 0; i < a->sub[0]; ++i) {
                        char n[256];
                        Sprintf(n, "%s[%d]", sp->name, i);
                        val[i] = *hoc_val_pointer(n);
                    }
                }
            } else {
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
    std::vector<int> iw(nrn_nthread);  // index for each thread
    Symbol* ncsym = hoc_lookup("NetCon");
    hoc_List* ncl = ncsym->u.ctemplate->olist;
    hoc_Item* q;
    ITERATE(q, ncl) {
        Object* ho = (Object*) VOIDITM(q);
        NetCon* nc = (NetCon*) ho->u.this_pointer;
        std::size_t ith = 0;  // if no _vnt, put in thread 0
        if (nc->target_ && nc->target_->_vnt) {
            ith = std::size_t(((NrnThread*) (nc->target_->_vnt))->id);
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
size_t nrnthreads_type_return(int type, int tid, double*& data, std::vector<double*>& mdata) {
    size_t n = 0;
    data = NULL;
    mdata.clear();
    if (tid >= nrn_nthread) {
        return n;
    }
    NrnThread& nt = nrn_threads[tid];
    if (type == voltage) {
        auto const cache_token = nrn_ensure_model_data_are_sorted();
        data = nt.node_voltage_storage();
        n = size_t(nt.end);
    } else if (type == i_membrane_) {  // i_membrane_
        auto const cache_token = nrn_ensure_model_data_are_sorted();
        data = nt.node_sav_rhs_storage();
        n = size_t(nt.end);
    } else if (type == 0) {  // time
        data = &nt._t;
        n = 1;
    } else if (type > 0 && type < n_memb_func) {
        Memb_list* ml = nt._ml_list[type];
        if (ml) {
            mdata = ml->data();
            n = ml->nodecount;
        } else {
            // The single thread case is easy
            if (nrn_nthread == 1) {
                ml = &memb_list[type];
                mdata = ml->data();
                n = ml->nodecount;
            } else {
                // mk_tml_with_art() created a cgs[id].mlwithart which appended
                // artificial cells to the end. Turns out that
                // cellgroups_[tid].type2ml[type]
                // is the Memb_list we need. Sadly, by the time we get here, cellgroups_
                // has already been deleted.  So we defer deletion of the necessary
                // cellgroups_ portion (deleting it on return from nrncore_run).
                auto& ml = CellGroup::deferred_type2artml_[tid][type];
                n = size_t(ml->nodecount);
                mdata = ml->data();
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


int nrnthread_dat1(int tid,
                   int& n_presyn,
                   int& n_netcon,
                   std::vector<int>& output_gid,
                   int*& netcon_srcgid,
                   std::vector<int>& netcon_negsrcgid_tid) {
    if (tid >= nrn_nthread) {
        return 0;
    }
    CellGroup& cg = cellgroups_[tid];
    n_presyn = cg.n_presyn;
    n_netcon = cg.n_netcon;
    output_gid = std::move(cg.output_gid);
    netcon_srcgid = cg.netcon_srcgid;
    cg.netcon_srcgid = NULL;
    netcon_negsrcgid_tid = cg.netcon_negsrcgid_tid;
    return 1;
}

// sizes and total data count
int nrnthread_dat2_1(int tid,
                     int& ncell,
                     int& ngid,
                     int& n_real_gid,
                     int& nnode,
                     int& ndiam,
                     int& nmech,
                     int*& tml_index,
                     int*& ml_nodecount,
                     int& nidata,
                     int& nvdata,
                     int& nweight) {
    if (tid >= nrn_nthread) {
        return 0;
    }
    CellGroup& cg = cellgroups_[tid];
    NrnThread& nt = nrn_threads[tid];

    ncell = cg.n_real_cell;
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
        for (int psz = 0; psz < bbcore_dparam_size[type]; ++psz) {
            if (ds[psz] == -4 || ds[psz] == -6 || ds[psz] == -7 || ds[psz] == -11 || ds[psz] == 0) {
                // printf("%s ds[%d]=%d vdata_offset=%d\n", memb_func[type].sym->name, psz, ds[psz],
                // vdata_offset);
                vdata_offset += ml->nodecount;
            }
        }
    }
    nvdata = vdata_offset;
    nidata = 0;
    //  printf("nidata=%d nvdata=%d nnetcon=%d\n", nidata, nvdata, cg.n_netcon);
    nweight = 0;
    for (int i = 0; i < cg.n_netcon; ++i) {
        nweight += cg.netcons[i]->cnt_;
    }

    return 1;
}

int nrnthread_dat2_2(int tid,
                     int*& v_parent_index,
                     double*& a,
                     double*& b,
                     double*& area,
                     double*& v,
                     double*& diamvec) {
    if (tid >= nrn_nthread) {
        return 0;
    }
    CellGroup& cg = cellgroups_[tid];
    NrnThread& nt = nrn_threads[tid];

    assert(cg.n_real_cell == nt.ncell);

    // If direct transfer, copy, because target space already allocated
    bool copy = corenrn_direct;
    if (copy) {
        std::copy_n(nt.node_a_storage(), nt.end, a);
        std::copy_n(nt.node_b_storage(), nt.end, b);
        std::copy_n(nt.node_area_storage(), nt.end, area);
        std::copy_n(nt.node_voltage_storage(), nt.end, v);
        std::copy_n(nt._v_parent_index, nt.end, v_parent_index);
    } else {
        v_parent_index = nt._v_parent_index;
        auto const cache_token = nrn_ensure_model_data_are_sorted();
        a = nt.node_a_storage();
        area = nt.node_area_storage();
        b = nt.node_b_storage();
        v = nt.node_voltage_storage();
    }
    if (cg.ndiam) {
        if (!copy) {
            diamvec = new double[nt.end];
        }
        for (int i = 0; i < nt.end; ++i) {
            Node* nd = nt._v_node[i];
            double diam = 0.0;
            for (Prop* p = nd->prop; p; p = p->next) {
                if (p->_type == MORPHOLOGY) {
                    diam = p->param(0);
                    break;
                }
            }
            diamvec[i] = diam;
        }
    }
    return 1;
}

int nrnthread_dat2_mech(int tid,
                        size_t i,
                        int dsz_inst,
                        int*& nodeindices,
                        double*& data,
                        int*& pdata,
                        std::vector<uint32_t>& nmodlrandom,  // 5 uint32_t per var per instance
                        std::vector<int>& pointer2type) {
    if (tid >= nrn_nthread) {
        return 0;
    }
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

    // As the NEURON data is now transposed then for now always create a new
    // copy in the format expected by CoreNEURON.
    // TODO remove the need for this entirely
    if (!copy) {
        data = new double[n * sz];
    }
    for (auto instance = 0, k = 0; instance < n; ++instance) {
        for (auto variable = 0; variable < sz; ++variable) {
            data[k++] = ml->data(instance, variable);
        }
    }

    if (isart) {  // data may not be contiguous
        nodeindices = NULL;
    } else {
        nodeindices = ml->nodeindices;  // allocated below if copy
    }
    if (copy) {
        if (!isart) {
            nodeindices = (int*) emalloc(n * sizeof(int));
            for (int i = 0; i < n; ++i) {
                nodeindices[i] = ml->nodeindices[i];
            }
        }
    }

    sz = bbcore_dparam_size[type];  // nrn_prop_dparam_size off by 1 if cvode_ieq.
    if (sz) {
        int* pdata1;
        pdata1 = datum2int(type, ml, nt, cg, cg.datumindices[dsz_inst], vdata_offset, pointer2type);
        if (copy) {
            int nn = n * sz;
            for (int i = 0; i < nn; ++i) {
                pdata[i] = pdata1[i];
            }
            delete[] pdata1;
        } else {
            pdata = pdata1;
        }
    } else {
        pdata = NULL;
    }

    // nmodlrandom: reserve 5 uint32 for each var of each instance
    // id1, id2, id3, seq, uint32_t(which)
    // Header is number of random variables followed by dparam indices
    // if no destructor, skip. There are no random variables.
    if (nrn_mech_inst_destruct.count(type)) {
        auto& indices = nrn_mech_random_indices(type);
        nmodlrandom.reserve(1 + indices.size() + 5 * n * indices.size());
        nmodlrandom.push_back(indices.size());
        for (int ix: indices) {
            nmodlrandom.push_back((uint32_t) ix);
        }
        for (int ix: indices) {
            uint32_t data[5];
            char which;
            for (int i = 0; i < n; ++i) {
                auto& datum = ml->pdata[i][ix];
                nrnran123_State* r = (nrnran123_State*) datum.get<void*>();
                nrnran123_getids3(r, &data[0], &data[1], &data[2]);
                nrnran123_getseq(r, &data[3], &which);
                data[4] = uint32_t(which);
                for (auto j: data) {
                    nmodlrandom.push_back(j);
                }
            }
        }
    }
    return 1;
}

int nrnthread_dat2_3(int tid,
                     int nweight,
                     int*& output_vindex,
                     double*& output_threshold,
                     int*& netcon_pnttype,
                     int*& netcon_pntindex,
                     double*& weights,
                     double*& delays) {
    if (tid >= nrn_nthread) {
        return 0;
    }
    CellGroup& cg = cellgroups_[tid];

    output_vindex = new int[cg.n_presyn];
    output_threshold = new double[cg.n_real_output];
    for (int i = 0; i < cg.n_presyn; ++i) {
        output_vindex[i] = cg.output_vindex[i];
    }
    for (int i = 0; i < cg.n_real_output; ++i) {
        output_threshold[i] = cg.output_ps[i] ? cg.output_ps[i]->threshold_ : 0.0;
    }

    // connections
    int n = cg.n_netcon;
    // printf("n_netcon=%d nweight=%d\n", n, nweight);
    netcon_pnttype = cg.netcon_pnttype;
    cg.netcon_pnttype = NULL;
    netcon_pntindex = cg.netcon_pntindex;
    cg.netcon_pntindex = NULL;
    // alloc a weight array and write netcon weights
    weights = new double[nweight];
    int iw = 0;
    for (int i = 0; i < n; ++i) {
        NetCon* nc = cg.netcons[i];
        for (int j = 0; j < nc->cnt_; ++j) {
            weights[iw++] = nc->weight_[j];
        }
    }
    // alloc a delay array and write netcon delays
    delays = new double[n];
    for (int i = 0; i < n; ++i) {
        NetCon* nc = cg.netcons[i];
        delays[i] = nc->delay_;
    }

    return 1;
}

int nrnthread_dat2_corepointer(int tid, int& n) {
    if (tid >= nrn_nthread) {
        return 0;
    }

    n = 0;
    MlWithArt& mla = cellgroups_[tid].mlwithart;
    for (size_t i = 0; i < mla.size(); ++i) {
        if (nrn_bbcore_write_[mla[i].first]) {
            ++n;
        }
    }

    return 1;
}

int nrnthread_dat2_corepointer_mech(int tid,
                                    int type,
                                    int& icnt,
                                    int& dcnt,
                                    int*& iArray,
                                    double*& dArray) {
    if (tid >= nrn_nthread) {
        return 0;
    }
    NrnThread& nt = nrn_threads[tid];
    CellGroup& cg = cellgroups_[tid];
    Memb_list* ml = cg.type2ml[type];

    dcnt = 0;
    icnt = 0;
    // data size and allocate
    for (int i = 0; i < ml->nodecount; ++i) {
        (*nrn_bbcore_write_[type])(NULL, NULL, &dcnt, &icnt, ml, i, ml->pdata[i], ml->_thread, &nt);
    }
    dArray = NULL;
    iArray = NULL;
    if (icnt) {
        iArray = new int[icnt];
    }
    if (dcnt) {
        dArray = new double[dcnt];
    }
    icnt = dcnt = 0;
    // data values
    for (int i = 0; i < ml->nodecount; ++i) {
        (*nrn_bbcore_write_[type])(
            dArray, iArray, &dcnt, &icnt, ml, i, ml->pdata[i], ml->_thread, &nt);
    }

    return 1;
}


// primarily to return nrnran123 sequence info when psolve on the coreneuron
// side is finished so can either do another coreneuron psolve or
// continue on neuron side.
int core2nrn_corepointer_mech(int tid, int type, int icnt, int dcnt, int* iArray, double* dArray) {
    if (tid >= nrn_nthread) {
        return 0;
    }
    NrnThread& nt = nrn_threads[tid];
    Memb_list* ml = nt._ml_list[type];
    // ARTIFICIAL_CELL are not in nt.
    if (!ml) {
        ml = CellGroup::deferred_type2artml_[tid][type];
        assert(ml);
    }

    int ik = 0;
    int dk = 0;
    // data values
    for (int i = 0; i < ml->nodecount; ++i) {
        (*nrn_bbcore_read_[type])(dArray, iArray, &dk, &ik, ml, i, ml->pdata[i], ml->_thread, &nt);
    }
    assert(dk == dcnt);
    assert(ik == icnt);
    return 1;
}

// NMODL RANDOM seq34 data return from coreneuron
int core2nrn_nmodlrandom(int tid,
                         int type,
                         const std::vector<int>& indices,
                         const std::vector<double>& nmodlrandom) {
    if (tid >= nrn_nthread) {
        return 0;
    }
    NrnThread& nt = nrn_threads[tid];
    Memb_list* ml = nt._ml_list[type];
    // ARTIFICIAL_CELL are not in nt.
    if (!ml) {
        ml = CellGroup::deferred_type2artml_[tid][type];
        assert(ml);
    }

    auto& nrnindices = nrn_mech_random_indices(type);  // for sanity checking
    assert(nrnindices == indices);
    assert(nmodlrandom.size() == indices.size() * ml->nodecount);

    int ir = 0;  // into nmodlrandom
    for (const auto ix: nrnindices) {
        for (int i = 0; i < ml->nodecount; ++i) {
            auto& datum = ml->pdata[i][ix];
            nrnran123_State* state = (nrnran123_State*) datum.get<void*>();
            nrnran123_setseq(state, nmodlrandom[ir++]);
        }
    }
    return 1;
}

int* datum2int(int type,
               Memb_list* ml,
               NrnThread& nt,
               CellGroup& cg,
               DatumIndices& di,
               int ml_vdata_offset,
               std::vector<int>& pointer2type) {
    int isart = nrn_is_artificial_[di.type];
    int sz = bbcore_dparam_size[type];
    int* pdata = new int[ml->nodecount * sz];
    int* semantics = memb_func[type].dparam_semantics;
    for (int i = 0; i < ml->nodecount; ++i) {
        int ioff = i * sz;
        for (int j = 0; j < sz; ++j) {
            int jj = ioff + j;
            int etype = di.ion_type[jj];
            int eindex = di.ion_index[jj];
            const int seman = semantics[j];
            // Would probably be more clear if use seman for as many as
            // possible of the cases
            // below and within each case deal with etype appropriately.
            // ion_type and ion_index have become misnomers as they no longer
            // refer to ions specificially but the mechanism type where the
            // range variable lives (and otherwise is generally the same as
            // seman). And ion_index refers to the index of the range variable
            // within the mechanism (or voltage, area, etc.)
            if (seman == -5) {  // POINTER to range variable (e.g. voltage)
                pdata[jj] = eindex;
                pointer2type.push_back(etype);
            } else if (etype == -1) {
                if (isart) {
                    pdata[jj] = -1;  // maybe save this space eventually. but not many of these in
                                     // bb models
                } else {
                    pdata[jj] = eindex;
                }
            } else if (etype == -9) {
                pdata[jj] = eindex;
            } else if (etype > 0 && etype < 1000) {  // ion pointer
                pdata[jj] = eindex;
            } else if (etype > 1000 && etype < 2000) {  // ionstyle can be explicit instead of
                                                        // pointer to int*
                pdata[jj] = eindex;
            } else if (etype == -2) {  // an ion and this is the iontype
                pdata[jj] = eindex;
            } else if (etype == -4) {  // netsend (_tqitem)
                pdata[jj] = ml_vdata_offset + eindex;
                // printf("etype %d jj=%d eindex=%d pdata=%d\n", etype, jj, eindex, pdata[jj]);
            } else if (etype == -6) {  // pntproc
                pdata[jj] = ml_vdata_offset + eindex;
                // printf("etype %d jj=%d eindex=%d pdata=%d\n", etype, jj, eindex, pdata[jj]);
            } else if (etype == -7) {  // bbcorepointer
                pdata[jj] = ml_vdata_offset + eindex;
                // printf("etype %d jj=%d eindex=%d pdata=%d\n", etype, jj, eindex, pdata[jj]);
            } else if (etype == -11) {  // random
                pdata[jj] = ml_vdata_offset + eindex;
            } else {                   // uninterpreted
                assert(eindex != -3);  // avoided if last
                pdata[jj] = 0;
            }
        }
    }
    return pdata;
}

void part2_clean() {
    CellGroup::clean_art(cellgroups_);

    if (corenrn_direct) {
        CellGroup::defer_clean_netcons(cellgroups_);
    }

    delete[] cellgroups_;
    cellgroups_ = NULL;
}

std::vector<std::vector<NetCon*>> CellGroup::deferred_netcons;

void CellGroup::defer_clean_netcons(CellGroup* cgs) {
    clean_deferred_netcons();
    for (int tid = 0; tid < nrn_nthread; ++tid) {
        deferred_netcons.push_back(std::move(cgs[tid].netcons));
    }
}

void CellGroup::clean_deferred_netcons() {
    deferred_netcons.clear();
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
    if (tid >= nrn_nthread) {
        return 0;
    }
    NrnThread& nt = nrn_threads[tid];

    // add the index of each instance in fixed_play_ for thread tid.
    // error if not a VecPlayContinuous with no discon vector
    int i = 0;
    for (auto& item: *net_cvode_instance->fixed_play_) {
        if (item->type() == VecPlayContinuousType) {
            auto* vp = static_cast<VecPlayContinuous*>(item);
            if (vp->discon_indices_ == NULL) {
                if (vp->ith_ == nt.id) {
                    assert(vp->y_ && vp->t_);
                    indices.push_back(i);
                }
            } else {
                assert(0);
            }
        } else {
            assert(0);
        }
        ++i;
    }

    return 1;
}

int nrnthread_dat2_vecplay_inst(int tid,
                                int i,
                                int& vptype,
                                int& mtype,
                                int& ix,
                                int& sz,
                                double*& yvec,
                                double*& tvec,
                                int& last_index,
                                int& discon_index,
                                int& ubound_index) {
    if (tid >= nrn_nthread) {
        return 0;
    }
    NrnThread& nt = nrn_threads[tid];

    auto* fp = net_cvode_instance->fixed_play_;
    if (fp->at(i)->type() == VecPlayContinuousType) {
        auto* const vp = static_cast<VecPlayContinuous*>(fp->at(i));
        if (!vp->discon_indices_) {
            if (vp->ith_ == nt.id) {
                auto* pd = static_cast<double*>(vp->pd_);
                int found = 0;
                vptype = vp->type();
                for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
                    if (nrn_is_artificial_[tml->index]) {
                        continue;
                    }
                    Memb_list* ml = tml->ml;
                    int nn = nrn_prop_param_size_[tml->index] * ml->nodecount;
                    auto const legacy_index = ml->legacy_index(pd);
                    if (legacy_index >= 0) {
                        mtype = tml->index;
                        ix = legacy_index;
                        sz = vector_capacity(vp->y_);
                        yvec = vector_vec(vp->y_);
                        tvec = vector_vec(vp->t_);
                        found = 1;
                        break;
                    }
                }
                assert(found);
                // following 3 used for direct-mode.
                last_index = vp->last_index_;
                discon_index = vp->discon_index_;
                ubound_index = vp->ubound_index_;
                return 1;
            }
        }
    }

    return 0;
}

/** getting one item at a time from CoreNEURON **/
void core2nrn_vecplay(int tid, int i, int last_index, int discon_index, int ubound_index) {
    if (tid >= nrn_nthread) {
        return;
    }
    auto* fp = net_cvode_instance->fixed_play_;
    assert(fp->at(i)->type() == VecPlayContinuousType);
    VecPlayContinuous* vp = (VecPlayContinuous*) fp->at(i);
    vp->last_index_ = last_index;
    vp->discon_index_ = discon_index;
    vp->ubound_index_ = ubound_index;
}

/** start the vecplay events **/
void core2nrn_vecplay_events() {
    for (auto& item: *net_cvode_instance->fixed_play_) {
        if (item->type() == VecPlayContinuousType) {
            auto* vp = static_cast<VecPlayContinuous*>(item);
            NrnThread* nt = nrn_threads + vp->ith_;
            vp->e_->send(vp->t_->elem(vp->ubound_index_), net_cvode_instance, nt);
        }
    }
}

/** getting one item at a time from nrn2core_transfer_WATCH **/
void nrn2core_transfer_WatchCondition(WatchCondition* wc, void (*cb)(int, int, int, int, int)) {
    Point_process* pnt = wc->pnt_;
    assert(pnt);
    int tid = ((NrnThread*) (pnt->_vnt))->id;
    int pnttype = pnt->prop->_type;
    int watch_index = wc->watch_index_;
    int triggered = wc->flag_ ? 1 : 0;
    int pntindex = CellGroup::nrncore_pntindex_for_queue(pnt->prop, tid, pnttype);
    (*cb)(tid, pnttype, pntindex, watch_index, triggered);

    // This transfers CvodeThreadData activated WatchCondition
    // information. All WatchCondition stuff is implemented in netcvode.cpp.
    // cvodeobj.h: HTList* CvodeThreadData.watch_list_
    // netcon.h: WatchCondition
    // On the NEURON side, WatchCondition is activated within a
    // NET_RECEIVE block with the NMODL WATCH statement translated into a
    // call to _nrn_watch_activate implmented as a function in netcvode.cpp.
    // Note that on the CoreNEURON side, all the WATCH functionality is
    // implemented within the mod file translation, and the info from this side
    // is used to assign a value in the location specified by the
    // _watch_array(flag) macro.
    // The return from psolve must transfer back the correct conditions
    // so that NEURON can continue with a classical psolve, or, if CoreNEURON
    // continues, receive the correct transfer of conditions back from NEURON
    // again.
    // Note: the reason CoreNEURON does not already have the correct watch
    // condition from phase2 setup is because, on the NEURON side,
    // _nrn_watch_activate fills in the _watch_array[0] with a pointer to
    // WatchList and _watch_array[i] with a pointer to WatchCondition.
    // Activation consists of removing all conditions from a HTList (HeadTailList)
    // and _watch_array[0] (only on the first _nrn_watch_activate call from a
    // NET_RECEIVE delivery event). And appending to _watch_array[0] and
    // Append to the HTList which is the CvodeThreadData.watch_list_;
    // But on the CoreNEURON side, _watch_array[0] is unused and _watch_array[i]
    // is a two bit integer. Bit 2 on means the WATCH is activated Bit 1
    // is used to determine the transition from false to true for doing a
    // net_send (immediate deliver).
}

// for faster determination of the movable index given the type
static std::map<int, int> type2movable;
static void setup_type2semantics() {
    if (type2movable.empty()) {
        for (int type = 0; type < n_memb_func; ++type) {
            int* ds = memb_func[type].dparam_semantics;
            if (ds) {
                for (int psz = 0; psz < bbcore_dparam_size[type]; ++psz) {
                    if (ds[psz] == -4) {  // netsend semantics
                        type2movable[type] = psz;
                    }
                }
            }
        }
    }
}

// Copying TQItem information for transfer to CoreNEURON has been factored
// out of nrn2core_transfer_tqueue since, if BinQ is being used, it involves
// iterating over the BinQ as well as the normal TQueue.
static void set_info(TQItem* tqi,
                     int tid,
                     NrnCoreTransferEvents* core_te,
                     std::unordered_map<NetCon*, std::vector<size_t>>& netcon2intdata,
                     std::unordered_map<PreSyn*, std::vector<size_t>>& presyn2intdata,
                     std::unordered_map<double*, std::vector<size_t>>& weight2intdata) {
    DiscreteEvent* de = (DiscreteEvent*) (tqi->data_);
    int type = de->type();
    double tdeliver = tqi->t_;
    core_te->type.push_back(type);
    core_te->td.push_back(tdeliver);

    switch (type) {
    case DiscreteEventType: {  // 0
    } break;
    case NetConType: {  // 2
        NetCon* nc = (NetCon*) de;
        // To find the i for cg.netcons[i] == nc
        // and since there are generally very many fewer nc on the queue
        // than netcons. Begin a nc2i map that we can fill in for i
        // later in one sweep through the cg.netcons.
        // Here is where it goes.
        size_t iloc = core_te->intdata.size();
        core_te->intdata.push_back(-1);
        // But must take into account the rare situation where the same
        // NetCon is on the queue more than once. Hence the std::vector
        netcon2intdata[nc].push_back(iloc);
    } break;
    case SelfEventType: {  // 3
        SelfEvent* se = (SelfEvent*) de;
        Point_process* pnt = se->target_;
        int type = pnt->prop->_type;
        int movable_index = type2movable[type];
        double* wt = se->weight_;

        core_te->intdata.push_back(type);
        core_te->dbldata.push_back(se->flag_);

        // All SelfEvent have a target. A SelfEvent only has a weight if
        // it was issued in response to a NetCon event to the target
        // NET_RECEIVE block. Determination of Point_process* target_ on the
        // CoreNEURON side uses mechanism type and instance index from here
        // on the NEURON side. And the latter can be determined now from pnt.
        // On the other hand, if there is a non-null weight pointer, its index
        // can only be determined by sweeping over all NetCon.

        // Introduced the public static method below because ARTIFICIAL_CELL
        // are not located in NrnThread and are not cache efficient.
        int index = CellGroup::nrncore_pntindex_for_queue(pnt->prop, tid, type);
        core_te->intdata.push_back(index);

        size_t iloc_wt = core_te->intdata.size();
        if (wt) {  // don't bother with NULL weights
            weight2intdata[wt].push_back(iloc_wt);
        }
        core_te->intdata.push_back(-1);  // If NULL weight this is the indicator
        // Each of these holds a TQItem*
        Datum* const movable = se->movable_;
        Datum* const pnt_movable = pnt->prop->dparam + movable_index;
        // Only one SelfEvent on the queue for a given point process can be
        // movable
        bool const condition = movable && (*movable).get<TQItem*>() == tqi;
        core_te->intdata.push_back(condition);
        if (condition) {
            assert(pnt_movable && (*pnt_movable).get<TQItem*>() == tqi);
        }

    } break;
    case PreSynType: {  // 4
        PreSyn* ps = (PreSyn*) de;

        // NEURON puts PreSyn on every thread queue
        // Skip if PreSyn not associated with this thread.
        bool skip = (ps->nt_ && (ps->nt_->id != tid)) ? true : false;
        // Skip if effectively an InputPresyn (ps->nt_ == NULL)
        //     and this is not thread 0.
        skip = (!ps->nt_ && tid != 0) ? true : skip;
        if (skip) {
            // erase what was already added
            core_te->type.pop_back();
            core_te->td.pop_back();
            break;
        }
        // Output PreSyn similar to NetCon but more data.
        // Input PreSyn (ps->output_index = -1 and ps->gid >= 0)
        // is distinquished from PreSyn (ps->output_index == ps->gid
        // or both negative) by the first item of 0 or 1 respectively followed
        // by gid or presyn index respectively.
        // That is:
        // Output PreSyn format is 0, presyn index
        // initialized to -1 and figured out from presyn2intdata, and
        // ps->delay_
        // Input PreSyn format is 1, gid, and ps->delay_
        if (ps->output_index_ < 0 && ps->gid_ >= 0) {
            // InputPreSyn on the CoreNEURON side
            core_te->intdata.push_back(1);
            core_te->intdata.push_back(ps->gid_);
        } else {
            // PreSyn on the NEURON side
            core_te->intdata.push_back(0);
            size_t iloc = core_te->intdata.size();
            core_te->intdata.push_back(-1);
            presyn2intdata[ps].push_back(iloc);
        }
        // CoreNEURON PreSyn has no notion of use_min_delay_ so if that
        // is in effect, then the send time is actually tt - nc->delay_
        // (Note there is no core2nrn inverse as PreSyn does not appear on
        //  the CoreNEURON event queue).
        if (ps->use_min_delay_) {
            core_te->td.back() -= ps->delay_;
        }
    } break;
    case HocEventType: {  // 5
        // Not supported in CoreNEURON, discard and print a warning.
        core_te->td.pop_back();
        core_te->type.pop_back();
        // Delivery time was often reduced by a quarter step to avoid
        // fixed step roundoff problems.
        Fprintf(stderr,
                "WARNING: CVode.event(...) for delivery at time step nearest %g discarded. "
                "CoreNEURON cannot presently handle interpreter events (rank %d, thread %d).\n",
                nrnmpi_myid,
                tdeliver,
                nrnmpi_myid,
                tid);
    } break;
    case PlayRecordEventType: {  // 6
    } break;
    case NetParEventType: {  // 7
    } break;
    default: {
    } break;
    }
}

NrnCoreTransferEvents* nrn2core_transfer_tqueue(int tid) {
    if (tid >= nrn_nthread) {
        return NULL;
    }

    setup_type2semantics();

    NrnCoreTransferEvents* core_te = new NrnCoreTransferEvents;

    // see comments below about same object multiple times on queue
    // and single sweep fill
    std::unordered_map<NetCon*, std::vector<size_t>> netcon2intdata;
    std::unordered_map<PreSyn*, std::vector<size_t>> presyn2intdata;
    std::unordered_map<double*, std::vector<size_t>> weight2intdata;

    NrnThread& nt = nrn_threads[tid];
    TQueue* tq = net_cvode_instance_event_queue(&nt);
    TQItem* tqi;
    auto& cg = cellgroups_[tid];
    // make sure all buffered interthread events are on the queue
    nrn_interthread_enqueue(&nt);

    // Iterate over all tqueue items to record info needed for transfer to
    // coreneuron. The atomic_dq removes items from the queue but misses
    // BinQ items if present. So need separate iteration for that (hence the
    // factoring out of the loop bodies into set_info.)
    while ((tqi = tq->atomic_dq(1e15)) != NULL) {
        set_info(tqi, tid, core_te, netcon2intdata, presyn2intdata, weight2intdata);
    }
    if (nrn_use_bin_queue_) {
        // does not remove items but the entire queue will be cleared
        // before using again.
        for (tqi = tq->binq()->first(); tqi; tqi = tq->binq()->next(tqi)) {
            set_info(tqi, tid, core_te, netcon2intdata, presyn2intdata, weight2intdata);
        }
    }

    // fill in the integers for the pointer translation

    // NEURON NetCon* to CoreNEURON index into nt.netcons
    for (int i = 0; i < cg.n_netcon; ++i) {
        NetCon* nc = cg.netcons[i];
        auto iter = netcon2intdata.find(nc);
        if (iter != netcon2intdata.end()) {
            for (auto iloc: iter->second) {
                core_te->intdata[iloc] = i;
            }
        }
    }

    // NEURON PreSyn* to CoreNEURON index into nt.presyns
#define NRN_SENTINAL 100000000000
    for (int i = 0; i < cg.n_presyn; ++i) {
        PreSyn* ps = cg.output_ps[i];
        auto iter = presyn2intdata.find(ps);
        if (iter != presyn2intdata.end()) {
            // not visited twice
            assert(iter->second[0] < NRN_SENTINAL);
            for (auto iloc: iter->second) {
                core_te->intdata[iloc] = i;
            }
            presyn2intdata[ps][0] = i + NRN_SENTINAL;
        }
    }
    // all presyn2intdata should have been visited so all
    // presyn2intdata[ps][0] must be >= NRN_SENTINAL
    for (auto& iter: presyn2intdata) {
        assert(iter.second[0] >= NRN_SENTINAL);
    }

    // NEURON SelfEvent weight* into CoreNEURON index into nt.netcons
    //    On the CoreNEURON side we find the NetCon and then the
    //    nc.u.weight_index_
    for (int i = 0; i < cg.n_netcon; ++i) {
        NetCon* nc = cg.netcons[i];
        double* wt = nc->weight_;
        auto iter = weight2intdata.find(wt);
        if (iter != weight2intdata.end()) {
            for (auto iloc: iter->second) {
                core_te->intdata[iloc] = i;
            }
        }
    }

    return core_te;
}

/** @brief Initialize queues before transfer
    Probably aleady clear, but if binq then must be initialized to time.
 */
void core2nrn_clear_queues(double time) {
    nrn_threads[0]._t = time;  // used by clear_event_queue
    clear_event_queue();
}

/** @brief Called from CoreNEURON core2nrn_tqueue_item.
 */
void core2nrn_NetCon_event(int tid, double td, size_t nc_index) {
    assert(tid < nrn_nthread);
    NrnThread& nt = nrn_threads[tid];
    // cellgroups_ has been deleted but deletion of cg.netcons was deferred
    // (and will be deleted on return from nrncore_run).
    // This is tragic for memory usage. There are more NetCon's than anything.
    // Would be better to save the memory at a cost of single iteration over
    // NetCon.
    NetCon* nc = CellGroup::deferred_netcons[tid][nc_index];
    nc->send(td, net_cvode_instance, &nt);
}

static void core2nrn_SelfEvent_helper(int tid,
                                      double td,
                                      int tar_type,
                                      int tar_index,
                                      double flag,
                                      double* weight,
                                      int is_movable) {
    if (type2movable.empty()) {
        setup_type2semantics();
    }
    Memb_list* ml = nrn_threads[tid]._ml_list[tar_type];
    Point_process* pnt;
    if (ml) {
        pnt = ml->pdata[tar_index][1].get<Point_process*>();
    } else {
        // In NEURON world, ARTIFICIAL_CELLs do not live in NrnThread.
        // And the old deferred_type2artdata_ only gave us data, not pdata.
        // So this is where I decided to replace the more
        // expensive deferred_type2artml_.
        ml = CellGroup::deferred_type2artml_[tid][tar_type];
        pnt = ml->pdata[tar_index][1].get<Point_process*>();
    }

    // Needs to be tested when permuted on CoreNEURON side.
    assert(tar_type == pnt->prop->_type);
    assert(tar_index == CellGroup::nrncore_pntindex_for_queue(pnt->prop, tid, tar_type));

    int const movable_index = type2movable[tar_type];
    auto* const movable_arg = pnt->prop->dparam + movable_index;
    auto* const old_movable_arg = (*movable_arg).get<TQItem*>();
    nrn_net_send(movable_arg, weight, pnt, td, flag);
    if (!is_movable) {
        *movable_arg = old_movable_arg;
    }
}

void core2nrn_SelfEvent_event(int tid,
                              double td,
                              int tar_type,
                              int tar_index,
                              double flag,
                              size_t nc_index,
                              int is_movable) {
    assert(tid < nrn_nthread);
    NetCon* nc = CellGroup::deferred_netcons[tid][nc_index];

#if 1
    // verify nc->target_ consistent with tar_type, tar_index.
    Memb_list* ml = nrn_threads[tid]._ml_list[tar_type];
    auto* pnt = ml->pdata[tar_index][1].get<Point_process*>();
    assert(nc->target_ == pnt);
#endif

    double* weight = nc->weight_;
    core2nrn_SelfEvent_helper(tid, td, tar_type, tar_index, flag, weight, is_movable);
}

void core2nrn_SelfEvent_event_noweight(int tid,
                                       double td,
                                       int tar_type,
                                       int tar_index,
                                       double flag,
                                       int is_movable) {
    assert(tid < nrn_nthread);
    double* weight = NULL;
    core2nrn_SelfEvent_helper(tid, td, tar_type, tar_index, flag, weight, is_movable);
}

// Set of the voltage indices in which PreSyn.flag_ == true
void core2nrn_PreSyn_flag(int tid, std::set<int> presyns_flag_true) {
    if (tid >= nrn_nthread) {
        return;
    }
    NetCvodeThreadData& nctd = net_cvode_instance->p[tid];
    hoc_Item* pth = nctd.psl_thr_;
    if (pth) {
        hoc_Item* q;
        // turn off all the PreSyn.flag_ as they might have been turned off
        // during the psolve on the coreneuron side.
        ITERATE(q, pth) {
            PreSyn* ps = (PreSyn*) VOIDITM(q);
            ps->flag_ = false;
        }
        if (presyns_flag_true.empty()) {
            return;
        }
        ITERATE(q, pth) {
            PreSyn* ps = (PreSyn*) VOIDITM(q);
            assert(ps->nt_ == (nrn_threads + tid));
            if (ps->thvar_) {
                int type = 0;
                int index_v = -1;
                nrn_dblpntr2nrncore(ps->thvar_, *ps->nt_, type, index_v);
                assert(type == voltage);
                if (presyns_flag_true.erase(index_v)) {
                    ps->flag_ = true;
                    if (presyns_flag_true.empty()) {
                        break;
                    }
                }
            }
        }
    }
}

// Add the voltage indices in which PreSyn.flag_ == true to the set.
void nrn2core_PreSyn_flag(int tid, std::set<int>& presyns_flag_true) {
    if (tid >= nrn_nthread) {
        return;
    }
    NetCvodeThreadData& nctd = net_cvode_instance->p[tid];
    hoc_Item* pth = nctd.psl_thr_;
    if (pth) {
        hoc_Item* q;
        ITERATE(q, pth) {
            auto* ps = static_cast<PreSyn*>(VOIDITM(q));
            assert(ps->nt_ == (nrn_threads + tid));
            if (ps->flag_ && ps->thvar_) {
                int type = 0;
                int index_v = -1;
                nrn_dblpntr2nrncore(ps->thvar_, *ps->nt_, type, index_v);
                assert(type == voltage);
                presyns_flag_true.insert(index_v);
            }
        }
    }
}

// For each watch index, activate the WatchCondition
void core2nrn_watch_activate(int tid, int type, int watch_begin, Core2NrnWatchInfo& wi) {
    if (tid >= nrn_nthread) {
        return;
    }
    NrnThread& nt = nrn_threads[tid];
    Memb_list* ml = nt._ml_list[type];
    for (size_t i = 0; i < wi.size(); ++i) {
        Core2NrnWatchInfoItem& active_watch_items = wi[i];
        Datum* pd = ml->pdata[i];
        int r = 0;  // first activate removes formerly active from pd.
        for (auto watch_item: active_watch_items) {
            int watch_index = watch_item.first;
            bool above_thresh = watch_item.second;
            auto* wc = pd[watch_index].get<WatchCondition*>();
            if (!wc) {  // if any do not exist in this instance, create them all
                        // with proper callback and flag.
                (*(nrn_watch_allocate_[type]))(pd);
                wc = pd[watch_index].get<WatchCondition*>();
            }
            _nrn_watch_activate(
                pd + watch_begin, wc->c_, watch_index - watch_begin, wc->pnt_, r++, wc->nrflag_);
            wc->flag_ = above_thresh ? 1 : 0;
            // If flag_ is 1
            // there will not be a (immediate) transition event
            // til the value() becomes negative again and then goes positive.
        }
    }
}

// nrn<->corenrn PatternStim

void* nrn_patternstim_info_ref(Datum*);
static int patternstim_type;

// Info from NEURON PatternStim at beginning of psolve.
void nrn2core_patternstim(void** info) {
    if (!patternstim_type) {
        for (int i = 3; i < n_memb_func; ++i) {
            if (strcmp(memb_func[i].sym->name, "PatternStim") == 0) {
                patternstim_type = i;
                break;
            }
        }
    }

    Memb_list& ml = memb_list[patternstim_type];
    assert(ml.nodecount == 1);
    *info = nrn_patternstim_info_ref(ml.pdata[0]);
}


// Info from NEURON subworlds at beginning of psolve.
void nrn2core_subworld_info(int& cnt,
                            int& subworld_index,
                            int& subworld_rank,
                            int& numprocs_subworld,
                            int& numprocs_world) {
#ifdef NRNMPI
    nrnmpi_get_subworld_info(
        &cnt, &subworld_index, &subworld_rank, &numprocs_subworld, &numprocs_world);
#else
    cnt = 0;
    subworld_index = -1;
    subworld_rank = 0;
    numprocs_subworld = 1;
    numprocs_world = 1;
#endif
}
