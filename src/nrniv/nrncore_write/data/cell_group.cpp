#include "cell_group.h"
#include "nrncore_write/utils/nrncore_utils.h"
#include "nrnran123.h"  // globalindex written to globals.dat
#include "section.h"
#include "parse.hpp"
#include "nrnmpi.h"
#include "netcon.h"
#include "netcvode.h"
extern NetCvode* net_cvode_instance;

#include <limits>
#include <sstream>

extern short* nrn_is_artificial_;
extern bool corenrn_direct;
extern int* bbcore_dparam_size;
extern int nrn_has_net_event_cnt_;
extern int* nrn_has_net_event_;
extern short* nrn_is_artificial_;

Deferred_Type2ArtMl CellGroup::deferred_type2artml_;
int* CellGroup::has_net_event_;

CellGroup::CellGroup() {
    n_output = n_real_output = n_presyn = n_netcon = n_mech = ntype = 0;
    group_id = -1;
    ndiam = 0;
    netcon_srcgid = netcon_pnttype = netcon_pntindex = 0;
    datumindices = 0;
    type2ml = new Memb_list*[n_memb_func];
    for (int i = 0; i < n_memb_func; ++i) {
        type2ml[i] = 0;
    }
    ml_vdata_offset = NULL;
}

CellGroup::~CellGroup() {
    if (netcon_srcgid)
        delete[] netcon_srcgid;
    if (netcon_pnttype)
        delete[] netcon_pnttype;
    if (netcon_pntindex)
        delete[] netcon_pntindex;
    if (datumindices)
        delete[] datumindices;
    if (ml_vdata_offset)
        delete[] ml_vdata_offset;
    delete[] type2ml;
}


void CellGroup::mk_cellgroups(neuron::model_sorted_token const& cache_token, CellGroup* cgs) {
    for (int i = 0; i < nrn_nthread; ++i) {
        auto& nt = nrn_threads[i];
        cgs[i].n_real_cell = nt.ncell;  // real cell count

        // Count PreSyn watching voltage (raise error if watching something else)
        // Allows possibility of multiple outputs for a cell.
        int npre = 0;
        NetCvodeThreadData& nctd = net_cvode_instance->p[i];
        hoc_Item* pth = nctd.psl_thr_;
        if (pth) {
            hoc_Item* q;
            ITERATE(q, pth) {
                auto* ps = static_cast<PreSyn*>(VOIDITM(q));
                // The PreSyn should refer to a valid Node
                assert(ps->thvar_);
                // The old code says this should always be a voltage, and
                // voltage is the thing we are moving to a new data structure,
                // so we should not be hitting the backwards-compatibility layer
                if (!ps->thvar_.refers_to<neuron::container::Node::field::Voltage>(
                        neuron::model().node_data())) {
                    hoc_execerr_ext("NetCon range variable reference source not a voltage");
                }
                if (ps->gid_ < 0) {
                    bool b1 = !ps->dil_.empty();
                    bool b2 = b1 && bool(ps->dil_[0]->target_);
                    std::string ncob(b1 ? hoc_object_name(ps->dil_[0]->obj_) : "");
                    hoc_execerr_ext(
                        "%s with voltage source has no gid."
                        " (The source is %s(x)._ref_v"
                        " and is the source for %zd NetCons. %s%s)",
                        (b1 ? ncob.c_str() : "NetCon"),
                        secname(ps->ssrc_),
                        ps->dil_.size(),
                        (b1 ? (ncob + " has target ").c_str() : ""),
                        (b1 ? (b2 ? std::string(hoc_object_name(ps->dil_[0]->target_->ob)).c_str()
                                  : "None")
                            : ""));
                }
                ++npre;
            }
        }
        cgs[i].n_real_output = npre;

        // Final count of n_presyn of thread
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

        // in case some cells do not have voltage presyns (eg threshold detection
        // computed from a POINT_PROCESS NET_RECEIVE with WATCH and net_event)
        // initialize as unused.
        cgs[i].output_ps.resize(npre);
        cgs[i].output_gid.resize(npre, -1);
        cgs[i].output_vindex.resize(npre, -1);

        // fill in the output_ps, output_gid, and output_vindex for the real cells.
        npre = 0;
        if (pth) {
            hoc_Item* q;
            ITERATE(q, pth) {
                auto* ps = static_cast<PreSyn*>(VOIDITM(q));
                assert(ps->thvar_);
                assert(ps->thvar_.refers_to_a_modern_data_structure());
                assert(ps->thvar_.refers_to<neuron::container::Node::field::Voltage>(
                    neuron::model().node_data()));
                cgs[i].output_ps.at(npre) = ps;
                cgs[i].output_gid.at(npre) = ps->output_index_;
                // Convert back to an old-style index, i.e. the index of the
                // voltage within this NrnThread after sorting
                cgs[i].output_vindex.at(npre) = ps->thvar_.current_row() -
                                                cache_token.thread_cache(i).node_data_offset;
                ++npre;
            }
        }
        assert(npre == cgs[i].n_real_output);

        // fill in the artcell info
        npre = cgs[i].n_real_output;
        cgs[i].n_output = npre;  // add artcell (and PP with net_event) with gid in following loop
        for (size_t j = 0; j < mla.size(); ++j) {
            int type = mla[j].first;
            Memb_list* ml = mla[j].second;
            if (nrn_has_net_event(type)) {
                for (int instance = 0; instance < ml->nodecount; ++instance) {
                    auto* const pnt = ml->pdata[instance][1].get<Point_process*>();
                    auto* const ps = static_cast<PreSyn*>(pnt->presyn_);
                    auto const other_thread = static_cast<NrnThread*>(pnt->_vnt)->id;
                    assert(other_thread == i);
                    cgs[i].output_ps.at(npre) = ps;
                    auto const offset = cache_token.thread_cache(i).mechanism_offset.at(type);
                    auto const global_row = pnt->prop->id().current_row();
                    assert(global_row >= offset);
                    long const agid = -(type + 1000 * static_cast<long>(global_row - offset));
                    if (ps) {
                        if (ps->output_index_ >= 0) {  // has gid
                            cgs[i].output_gid[npre] = ps->output_index_;
                            if (cgs[i].group_id < 0) {
                                cgs[i].group_id = ps->output_index_;
                            }
                            ++cgs[i].n_output;
                        } else {
                            cgs[i].output_gid[npre] = agid;
                        }
                    } else {  // if an acell is never a source, it will not have a presyn
                        cgs[i].output_gid[npre] = -1;
                    }
                    // the way we associate an acell PreSyn with the
                    // Point_process.
                    if (agid < std::numeric_limits<int>::min() || agid >= -1) {
                        std::ostringstream oss;
                        oss << "maximum of ~" << std::numeric_limits<int>::max() / 1000
                            << " artificial cells of a given type can be created per NrnThread, "
                               "this model has "
                            << ml->nodecount << " instances of " << memb_func[type].sym->name
                            << " (cannot store cgs[" << i << "].output_vindex[" << npre
                            << "]=" << agid << ')';
                        hoc_execerror("integer overflow", oss.str().c_str());
                    }
                    cgs[i].output_vindex[npre] = agid;
                    ++npre;
                }
            }
        }
    }

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
    CellGroup::mk_cgs_netcon_info(cache_token, cgs);
}

void CellGroup::datumtransform(CellGroup* cgs) {
    // ions, area, and POINTER to v or mechanism data.
    for (int ith = 0; ith < nrn_nthread; ++ith) {
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
        int i = 0;
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
    int nnode = nt.end;
    int mcnt = ml->nodecount;
    int dsize = bbcore_dparam_size[di.type];
    if (dsize == 0) {
        return;
    }
    int* dmap = memb_func[di.type].dparam_semantics;
    assert(dmap);
    // what is the size of the nt._vdata portion needed for a single ml->dparam[i]
    int vdata_size = 0;
    for (int i = 0; i < dsize; ++i) {
        int* ds = memb_func[di.type].dparam_semantics;
        if (ds[i] == -4 || ds[i] == -6 || ds[i] == -7 || ds[i] == -11 || ds[i] == 0) {
            ++vdata_size;
        }
    }

    int isart = nrn_is_artificial_[di.type];
    for (int i = 0; i < mcnt; ++i) {
        // Prop* datum instance arrays are not in cache efficient order
        // ie. ml->pdata[i] are not laid out end to end in memory.
        // Also, ml->_data for artificial cells is not in cache efficient order
        // but in the artcell case there are no pointers to doubles
        Datum* dparam = ml->pdata[i];
        int offset = i * dsize;
        int vdata_offset = i * vdata_size;
        for (int j = 0; j < dsize; ++j) {
            int etype = -100;  // uninterpreted
            int eindex = -1;
            if (dmap[j] == -1) {  // used to be a double* into _actual_area, now handled by soa<...>
                if (isart) {
                    etype = -1;
                    eindex = -1;  // the signal to ignore in bbcore.
                } else {
                    auto area = static_cast<neuron::container::data_handle<double>>(dparam[j]);
                    assert(area.refers_to_a_modern_data_structure());
                    auto const cache_token = nrn_ensure_model_data_are_sorted();
                    etype = -1;
                    // current_row() refers to the global Node data, but we need
                    // to set eindex to something local to the NrnThread
                    eindex = area.current_row() - cache_token.thread_cache(ith).node_data_offset;
                }
            } else if (dmap[j] == -2) {  // this is an ion and dparam[j][0].i is the iontype
                etype = -2;
                eindex = dparam[j].get<int>();
            } else if (dmap[j] == -3) {  // cvodeieq is always last and never seen
                assert(dmap[j] != -3);
            } else if (dmap[j] == -4) {  // netsend (_tqitem pointer)
                // eventually index into nt->_vdata
                etype = -4;
                eindex = vdata_offset++;
            } else if (dmap[j] == -6) {  // pntproc
                // eventually index into nt->_vdata
                etype = -6;
                eindex = vdata_offset++;
            } else if (dmap[j] == -7) {  // bbcorepointer
                // eventually index into nt->_vdata
                etype = -6;
                eindex = vdata_offset++;
            } else if (dmap[j] == -8) {  // watch
                etype = -8;
                eindex = 0;
            } else if (dmap[j] == -10) {  // fornetcon
                etype = -10;
                eindex = 0;
            } else if (dmap[j] == -11) {  // random
                etype = -11;
                eindex = vdata_offset++;
            } else if (dmap[j] == -9) {  // diam
                cg.ndiam = nt.end;
                etype = -9;
                // Rare for a mechanism to use dparam pointing to diam.
                // MORPHOLOGY was never made cache efficient. And
                // is not in the tml_with_art.
                // Need to determine this node and then simple to search its
                // mechanism list for MORPHOLOGY and then know the diam.
                Node* nd = ml->nodelist[i];
                neuron::container::data_handle<double> pdiam{};
                for (Prop* p = nd->prop; p; p = p->next) {
                    if (p->_type == MORPHOLOGY) {
                        pdiam = p->param_handle(0);
                        break;
                    }
                }
                assert(static_cast<neuron::container::data_handle<double>>(dparam[j]) == pdiam);
                eindex = ml->nodeindices[i];
            } else if (dmap[j] == -5) {  // POINTER
                // must be a pointer into nt->_data. Handling is similar to eion so
                // give proper index into the type.
                auto const pd = static_cast<neuron::container::data_handle<double>>(dparam[j]);
                nrn_dblpntr2nrncore(pd, nt, etype, eindex);
                if (etype == 0) {
                    fprintf(stderr,
                            "POINTER is not pointing to voltage or mechanism data. Perhaps it "
                            "should be a BBCOREPOINTER\n");
                }
                assert(etype != 0);
                // pointer into one of the tml types?
            } else if (dmap[j] > 0 && dmap[j] < 1000) {  // double* into eion type data
                etype = dmap[j];
                Memb_list* eml = cg.type2ml[etype];
                assert(eml);
                auto* const pval = dparam[j].get<double*>();
                auto const legacy_index = eml->legacy_index(pval);
                assert(legacy_index >= 0);
                eindex = legacy_index;
            } else if (dmap[j] > 1000) {  // int* into ion dparam[xxx][0]
                // store the actual ionstyle
                etype = dmap[j];
                eindex = *dparam[j].get<int*>();
            } else {
                char errmes[100];
                Sprintf(errmes, "Unknown semantics type %d for dparam item %d of", dmap[j], j);
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
void CellGroup::mk_cgs_netcon_info(neuron::model_sorted_token const& cache_token, CellGroup* cgs) {
    // count the netcons for each thread
    int* nccnt = new int[nrn_nthread];
    for (int i = 0; i < nrn_nthread; ++i) {
        nccnt[i] = 0;
    }
    Symbol* ncsym = hoc_lookup("NetCon");
    hoc_List* ncl = ncsym->u.ctemplate->olist;
    hoc_Item* q;
    ITERATE(q, ncl) {
        Object* ho = (Object*) VOIDITM(q);
        NetCon* nc = (NetCon*) ho->u.this_pointer;
        int ith = 0;  // if no _vnt, put in thread 0
        if (nc->target_ && nc->target_->_vnt) {
            ith = ((NrnThread*) (nc->target_->_vnt))->id;
        }
        ++nccnt[ith];
    }

    // allocate
    for (int i = 0; i < nrn_nthread; ++i) {
        cgs[i].n_netcon = nccnt[i];
        cgs[i].netcons.resize(nccnt[i] + 1);
        cgs[i].netcon_srcgid = new int[nccnt[i] + 1];
        cgs[i].netcon_pnttype = new int[nccnt[i] + 1];
        cgs[i].netcon_pntindex = new int[nccnt[i] + 1];
    }

    // reset counts and fill
    for (int i = 0; i < nrn_nthread; ++i) {
        nccnt[i] = 0;
    }
    ITERATE(q, ncl) {
        Object* ho = (Object*) VOIDITM(q);
        NetCon* nc = (NetCon*) ho->u.this_pointer;
        int ith = 0;  // if no _vnt, put in thread 0
        if (nc->target_ && nc->target_->_vnt) {
            ith = ((NrnThread*) (nc->target_->_vnt))->id;
        }
        int i = nccnt[ith];
        cgs[ith].netcons[i] = nc;

        if (nc->target_) {
            int type = nc->target_->prop->_type;
            auto const target_thread = static_cast<NrnThread*>(nc->target_->_vnt)->id;
            assert(target_thread == ith);
            cgs[ith].netcon_pnttype[i] = type;
            cgs[ith].netcon_pntindex[i] = nc->target_->prop->id().current_row() -
                                          cache_token.thread_cache(ith).mechanism_offset.at(type);
        } else {
            cgs[ith].netcon_pnttype[i] = 0;
            cgs[ith].netcon_pntindex[i] = -1;
        }

        if (nc->src_) {
            PreSyn* ps = nc->src_;
            if (ps->gid_ >= 0) {
                cgs[ith].netcon_srcgid[i] = ps->gid_;
            } else {
                if (ps->osrc_) {
                    assert(!ps->thvar_);
                    if (nrn_nthread > 1) {  // negative gid and multiple threads.
                        cgs[ith].netcon_negsrcgid_tid.push_back(ps->nt_->id);
                        // Raise error if file mode transfer and nc and ps not
                        // in same thread. In that case we cannot guarantee that
                        // the PreSyn will end up in the same coreneuron process.
                        if (!corenrn_direct && ith != ps->nt_->id) {
                            hoc_execerror(
                                "NetCon and NetCon source with no gid are not in the same thread",
                                NULL);
                        }
                    }
                    auto* const pnt = static_cast<Point_process*>(ps->osrc_->u.this_pointer);
                    int type = pnt->prop->_type;
                    auto const src_thread = static_cast<NrnThread*>(pnt->_vnt)->id;
                    auto const current = pnt->prop->id().current_row();
                    auto const offset =
                        cache_token.thread_cache(src_thread).mechanism_offset.at(type);
                    // the resulting GID is different for "the same" pnt/source
                    // if the number of threads changes, because it encodes the
                    // offset of the source process into the thread that it
                    // lives in
                    cgs[ith].netcon_srcgid[i] = -(type +
                                                  1000 * static_cast<long>(current - offset));
                } else {
                    cgs[ith].netcon_srcgid[i] = -1;
                }
            }
        } else {
            cgs[ith].netcon_srcgid[i] = -1;
        }
        ++nccnt[ith];
    }
    delete[] nccnt;
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
void CellGroup::mk_tml_with_art(neuron::model_sorted_token const& cache_token, CellGroup* cgs) {
    // copy NrnThread tml list and append ARTIFICIAL cell types
    // but do not include PatternStim if file mode.
    //    For direct mode PatternStim is not treated specially except that
    //    the Info struct is shared.
    //    For file mode transfer PatternStim has always been treated
    //    specially by CoreNEURON as it is not conceptually a part of
    //    the model but is invoked via an argument when launching
    //    CoreNEURON from the shell.
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
    int* acnt = new int[nrn_nthread];
    for (int i = 0; i < n_memb_func; ++i) {
        if (nrn_is_artificial_[i] && memb_list[i].nodecount) {
            // skip PatternStim if file mode transfer.
            if (!corenrn_direct && strcmp(memb_func[i].sym->name, "PatternStim") == 0) {
                continue;
            }
            if (strcmp(memb_func[i].sym->name, "HDF5Reader") == 0) {
                continue;
            }
            Memb_list* ml = &memb_list[i];
            // how many artificial in each thread
            for (int id = 0; id < nrn_nthread; ++id) {
                acnt[id] = 0;
            }
            for (int j = 0; j < memb_list[i].nodecount; ++j) {
                auto* pnt = memb_list[i].pdata[j][1].get<Point_process*>();
                int id = ((NrnThread*) pnt->_vnt)->id;
                ++acnt[id];
            }

            // allocate
            for (int id = 0; id < nrn_nthread; ++id) {
                if (acnt[id]) {
                    MlWithArt& mla = cgs[id].mlwithart;
                    ml = new Memb_list{i};
                    mla.push_back(MlWithArtItem(i, ml));  // need to delete ml when mla destroyed.
                    ml->nodecount = acnt[id];
                    ml->nodelist = NULL;
                    ml->nodeindices = NULL;
                    ml->prop = NULL;
                    ml->_thread = NULL;
                    // ml->_data = new double*[acnt[id]];
                    ml->pdata = new Datum*[acnt[id]];
                }
            }
            // fill data and pdata pointers
            for (int id = 0; id < nrn_nthread; ++id) {
                acnt[id] = 0;
            }
            for (int j = 0; j < memb_list[i].nodecount; ++j) {
                auto* pnt = memb_list[i].pdata[j][1].get<Point_process*>();
                int id = ((NrnThread*) pnt->_vnt)->id;
                Memb_list* ml = cgs[id].mlwithart.back().second;
                ml->set_storage_offset(cache_token.thread_cache(id).mechanism_offset.at(i));
                ml->pdata[acnt[id]] = memb_list[i].pdata[j];
                ++acnt[id];
            }
        }
    }
    delete[] acnt;
}

size_t CellGroup::get_mla_rankbytes(CellGroup* cellgroups_) {
    size_t mla_rankbytes = 0;
    size_t nbytes;
    NrnThread* nt;
    NrnThreadMembList* tml;
    FOR_THREADS(nt) {
        size_t threadbytes = 0;
        size_t npnt = 0;
        size_t nart = 0;
        int ith = nt->id;
        nbytes = nt->end * (1 * sizeof(int) + 3 * sizeof(double));
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
            int dpsize = nrn_prop_dparam_size_[type];  // includes cvodeieq if present
            // printf("%d %s ispnt %d  cnt %d  psize %d  dpsize %d\n",tml->index,
            // memb_func[type].sym->name, memb_func[type].is_point, ml->nodecount, psize, dpsize);
            // nodeindices, data, pdata + pnt with prop
            int notart = nrn_is_artificial_[type] ? 0 : 1;
            if (nrn_is_artificial_[type]) {
                nart += ml->nodecount;
            }
            nbytes = ml->nodecount *
                     (notart * sizeof(int) + 1 * sizeof(double*) + 1 * sizeof(Datum*) +
                      psize * sizeof(double) + dpsize * sizeof(Datum));
            threadbytes += nbytes;
        }
        nbytes += npnt * (sizeof(Point_process) + sizeof(Prop));
        // printf("  mech in use %d  Point instances %ld  artcells %ld  total instances %ld\n",
        // mechcnt, npnt, nart, mechcnt_instances);
        // printf("  thread bytes %ld\n", threadbytes);
        mla_rankbytes += threadbytes;
    }
    return mla_rankbytes;
}

void CellGroup::clean_art(CellGroup* cgs) {
    // clean up the art Memb_list of CellGroup[].mlwithart
    // But if multithread and direct transfer mode, defer deletion of
    // data for artificial cells, so that the artificial cell ml->_data
    // can be used when nrnthreads_type_return is called.
    if (corenrn_direct && nrn_nthread > 0) {
        deferred_type2artml_.resize(nrn_nthread);
    }
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        MlWithArt& mla = cgs[ith].mlwithart;
        for (size_t i = 0; i < mla.size(); ++i) {
            int type = mla[i].first;
            Memb_list* ml = mla[i].second;
            if (nrn_is_artificial_[type]) {
                if (!deferred_type2artml_.empty()) {
                    deferred_type2artml_[ith][type] = ml;
                } else {
                    // delete[] ml->_data;
                    delete[] ml->pdata;
                    delete ml;
                }
            }
        }
    }
}

void CellGroup::setup_nrn_has_net_event() {
    if (has_net_event_) {
        return;
    }

    has_net_event_ = new int[n_memb_func];
    for (int i = 0; i < n_memb_func; ++i) {
        has_net_event_[i] = 0;
    }
    for (int i = 0; i < nrn_has_net_event_cnt_; ++i) {
        has_net_event_[nrn_has_net_event_[i]] = 1;
    }
}
