/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/
#include <iostream>
#include <sstream>
#include <cassert>
#include <memory>

#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/io/nrn_filehandler.hpp"
#include "coreneuron/io/nrn_checkpoint.hpp"
#include "coreneuron/io/nrn_setup.hpp"
#include "coreneuron/network/netcvode.hpp"
#include "coreneuron/network/netpar.hpp"
#include "coreneuron/utils/vrecitem.h"
#include "coreneuron/mechanism/mech/mod2c_core_thread.hpp"
#include "coreneuron/io/file_utils.hpp"
#include "coreneuron/permute/node_permute.h"
#include "coreneuron/coreneuron.hpp"
#include "coreneuron/utils/nrnoc_aux.hpp"

namespace coreneuron {
bool nrn_checkpoint_arg_exists;
bool _nrn_skip_initmodel;
}  // namespace coreneuron
#define UseFileHandlerWrap 0

#if UseFileHandlerWrap

#include <iomanip>
namespace coreneuron {
/// wrapper class for FileHandler used for debugging checkpointing
class FileHandlerWrap {
  public:
    FileHandler F;
    std::fstream G;
    FileHandlerWrap(){};

    void open(const std::string& filename, std::ios::openmode mode = std::ios::in) {
        F.open(filename, mode);
        std::ostringstream fname;
        fname << filename << ".txt";
        G.open(fname.str(), mode);
    }

    void close() {
        F.close();
        G.close();
    }

    void checkpoint(int c) {
        F.checkpoint(c);
    }

    template <typename T>
    void write_array(T* p, size_t nb_elements) {
        // G first before chkpnt is incremented
        G << "chkpnt " << F.checkpoint() << "\n";
        for (size_t i = 0; i < nb_elements; ++i) {
            G << std::setprecision(8) << p[i] << "\n";
        }
        F.write_array(p, nb_elements);  // chkpnt incremented
    }

    template <typename T>
    FileHandlerWrap& operator<<(const T& scalar) {
        F << scalar;
        G << scalar;
        return *this;
    }
};
}  // namespace coreneuron
#else

#define FileHandlerWrap FileHandler

#endif  // UseFileHandlerWrap

namespace coreneuron {
template <typename T>
T* chkpnt_soa2aos(T* data, int cnt, int sz, int layout, int* permute) {
    // inverse of F -> data. Just a copy if layout=1. If SoA,
    // original file order depends on padding and permutation.
    // Good for a, b, area, v, diam, Memb_list.data, or anywhere values do not change.
    T* d = new T[cnt * sz];
    if (layout == Layout::AoS) {
        for (int i = 0; i < cnt * sz; ++i) {
            d[i] = data[i];
        }
    } else if (layout == Layout::SoA) {
        int align_cnt = nrn_soa_padded_size(cnt, layout);
        for (int i = 0; i < cnt; ++i) {
            int ip = i;
            if (permute) {
                ip = permute[i];
            }
            for (int j = 0; j < sz; ++j) {
                d[i * sz + j] = data[ip + j * align_cnt];
            }
        }
    }
    return d;
}

template <typename T>
void chkpnt_data_write(FileHandlerWrap& F, T* data, int cnt, int sz, int layout, int* permute) {
    T* d = chkpnt_soa2aos(data, cnt, sz, layout, permute);
    F.write_array<T>(d, cnt * sz);
    delete[] d;
}

NrnThreadChkpnt* nrnthread_chkpnt;

int patstimtype;

#ifndef LAYOUT
#define LAYOUT 1
#endif

// output directory to for checkpoint
static const char* output_dir;

static void write_phase2(NrnThread& nt, FileHandlerWrap& file_handle);
static void write_tqueue(NrnThread& nt, FileHandlerWrap& file_handle);
static void write_time(const char* dir);

void write_checkpoint(NrnThread* nt, int nb_threads, const char* dir) {
    // empty directory means the option is not enabled
    if (!strlen(dir)) {
        return;
    }

    output_dir = dir;
    if (nrnmpi_myid == 0) {
        mkdir_p(output_dir);
    }
#if NRNMPI
    nrnmpi_barrier();
#endif

    /**
     * if openmp threading needed:
     *  #pragma omp parallel for private(i) shared(nt, nb_threads) schedule(runtime)
     */
    FileHandlerWrap f;
    for (int i = 0; i < nb_threads; i++) {
        if (nt[i].ncell || nt[i].tml) {
            write_phase2(nt[i], f);
        }
    }

    if (nrnmpi_myid == 0) {
        write_time(output_dir);
    }
#if NRNMPI
    nrnmpi_barrier();
#endif
}

static void write_phase2(NrnThread& nt, FileHandlerWrap& fh) {
    std::ostringstream filename;

    NrnThreadChkpnt& ntc = nrnthread_chkpnt[nt.id];
    filename << output_dir << "/" << ntc.file_id << "_2.dat";

    fh.open(filename.str(), std::ios::out);
    fh.checkpoint(2);

    int n_outputgid = 0;  // calculate PreSyn with gid >= 0
    for (int i = 0; i < nt.n_presyn; ++i) {
        if (nt.presyns[i].gid_ >= 0) {
            ++n_outputgid;
        }
    }

    fh << n_outputgid << " ngid\n";
#if CHKPNTDEBUG
    assert(ntc.n_outputgids == n_outputgid);
#endif

    fh << nt.ncell << " n_real_gid\n";
    fh << nt.end << " nnode\n";
    fh << ((nt._actual_diam == nullptr) ? 0 : nt.end) << " ndiam\n";
    int nmech = 0;
    for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
        if (tml->index != patstimtype) {  // skip PatternStim
            ++nmech;
        }
    }

    fh << nmech << " nmech\n";
#if CHKPNTDEBUG
    assert(nmech == ntc.nmech);
#endif

    for (NrnThreadMembList* current_tml = nt.tml; current_tml; current_tml = current_tml->next) {
        if (current_tml->index == patstimtype) {
            continue;
        }
        fh << current_tml->index << "\n";
        fh << current_tml->ml->nodecount << "\n";
    }

    fh << nt._nidata << " nidata\n";
    fh << nt._nvdata << " nvdata\n";
    fh << nt.n_weight << " nweight\n";

    // see comment about parent in node_permute.cpp
    int* pinv_nt = nullptr;
    if (nt._permute) {
        int* d = new int[nt.end];
        pinv_nt = inverse_permute(nt._permute, nt.end);
        for (int i = 0; i < nt.end; ++i) {
            int x = nt._v_parent_index[nt._permute[i]];
            if (x >= 0) {
                d[i] = pinv_nt[x];
            } else {
                d[i] = 0;  // really should be -1;
            }
        }
#if CHKPNTDEBUG
        for (int i = 0; i < nt.end; ++i) {
            assert(d[i] == ntc.parent[i]);
        }
#endif
        fh.write_array<int>(d, nt.end);
        delete[] d;
    } else {
#if CHKPNTDEBUG
        for (int i = 0; i < nt.end; ++i) {
            assert(nt._v_parent_index[i] == ntc.parent[i]);
        }
#endif
        fh.write_array<int>(nt._v_parent_index, nt.end);
        pinv_nt = new int[nt.end];
        for (int i = 0; i < nt.end; ++i) {
            pinv_nt[i] = i;
        }
    }

    chkpnt_data_write(fh, nt._actual_a, nt.end, 1, 0, nt._permute);
    chkpnt_data_write(fh, nt._actual_b, nt.end, 1, 0, nt._permute);

#if CHKPNTDEBUG
    for (int i = 0; i < nt.end; ++i) {
        assert(nt._actual_area[i] == ntc.area[pinv_nt[i]]);
    }
#endif

    chkpnt_data_write(fh, nt._actual_area, nt.end, 1, 0, nt._permute);
    chkpnt_data_write(fh, nt._actual_v, nt.end, 1, 0, nt._permute);

    if (nt._actual_diam) {
        chkpnt_data_write(fh, nt._actual_diam, nt.end, 1, 0, nt._permute);
    }

    auto& memb_func = corenrn.get_memb_funcs();
    // will need the ml_pinv inverse permutation of ml._permute for ions
    int** ml_pinv = (int**) ecalloc(memb_func.size(), sizeof(int*));

    for (NrnThreadMembList* current_tml = nt.tml; current_tml; current_tml = current_tml->next) {
        Memb_list* ml = current_tml->ml;
        int type = current_tml->index;
        if (type == patstimtype) {
            continue;
        }
        int cnt = ml->nodecount;
        auto& nrn_prop_param_size_ = corenrn.get_prop_param_size();
        auto& nrn_prop_dparam_size_ = corenrn.get_prop_dparam_size();
        auto& nrn_is_artificial_ = corenrn.get_is_artificial();

        int sz = nrn_prop_param_size_[type];
        int layout = corenrn.get_mech_data_layout()[type];
        int* semantics = memb_func[type].dparam_semantics;

        if (!nrn_is_artificial_[type]) {
            // ml->nodeindices values are permuted according to nt._permute
            // and locations according to ml._permute
            // i.e. according to comment in node_permute.cpp
            // nodelist[p_m[i]] = p[nodelist_original[i]
            // so pinv[nodelist[p_m[i]] = nodelist_original[i]
            int* nd_ix = new int[cnt];
            for (int i = 0; i < cnt; ++i) {
                int ip = ml->_permute ? ml->_permute[i] : i;
                int ipval = ml->nodeindices[ip];
                nd_ix[i] = pinv_nt[ipval];
            }
            fh.write_array<int>(nd_ix, cnt);
            delete[] nd_ix;
        }

        chkpnt_data_write(fh, ml->data, cnt, sz, layout, ml->_permute);

        sz = nrn_prop_dparam_size_[type];
        if (sz) {
            int* d = chkpnt_soa2aos(ml->pdata, cnt, sz, layout, ml->_permute);
            // need to update some values according to Datum semantics.
            if (!nrn_is_artificial_[type])
                for (int i_instance = 0; i_instance < cnt; ++i_instance) {
                    for (int i = 0; i < sz; ++i) {
                        int ix = i_instance * sz + i;
                        int s = semantics[i];
                        if (s == -1) {  // area
                            int p = pinv_nt[d[ix] - (nt._actual_area - nt._data)];
                            d[ix] = p;         // relative _actual_area
                        } else if (s == -9) {  // diam
                            int p = pinv_nt[d[ix] - (nt._actual_diam - nt._data)];

                            d[ix] = p;         // relative to _actual_diam
                        } else if (s == -5) {  // Assume pointer to membrane voltage
                            int p = pinv_nt[d[ix] - (nt._actual_v - nt._data)];
                            d[ix] = p;                    // relative to _actual_v
                        } else if (s >= 0 && s < 1000) {  // ion
                            // determine ei_instance and ei
                            int etype = s;
                            Memb_list* eml = nt._ml_list[etype];
                            int ecnt = eml->nodecount;
                            int esz = nrn_prop_param_size_[etype];
                            int elayout = corenrn.get_mech_data_layout()[etype];
                            // current index into eml->data is a  function
                            // of elayout, eml._permute, ei_instance, ei, and
                            // eml padding.
                            int p = d[ix] - (eml->data - nt._data);
                            int ei_instance, ei;
                            nrn_inverse_i_layout(p, ei_instance, ecnt, ei, esz, elayout);
                            if (elayout == Layout::SoA) {
                                if (eml->_permute) {
                                    if (!ml_pinv[etype]) {
                                        ml_pinv[etype] = inverse_permute(eml->_permute,
                                                                         eml->nodecount);
                                    }
                                    ei_instance = ml_pinv[etype][ei_instance];
                                }
                            }
                            d[ix] = ei_instance * esz + ei;
                        }
#if CHKPNTDEBUG
                        if (s != -8) {  // WATCH values change
                            assert(d[ix] ==
                                   ntc.mlmap[type]->pdata_not_permuted[i_instance * sz + i]);
                        }
#endif
                    }
                }
            fh.write_array<int>(d, cnt * sz);
            delete[] d;
        }
    }

    int nnetcon = nt.n_netcon;

    int* output_vindex = new int[nt.n_presyn];
    double* output_threshold = new double[nt.ncell];
    for (int i = 0; i < nt.n_presyn; ++i) {
        PreSyn* ps = nt.presyns + i;
        if (ps->thvar_index_ >= 0) {
            // real cell and index into (permuted) actual_v
            // if any assert fails in this loop then we have faulty understanding
            // of the for (int i = 0; i < nt.n_presyn; ++i) loop in nrn_setup.cpp
            assert(ps->thvar_index_ < nt.end);
            assert(ps->pntsrc_ == nullptr);
            output_threshold[i] = ps->threshold_;
            output_vindex[i] = pinv_nt[ps->thvar_index_];
        } else if (i < nt.ncell) {      // real cell without a presyn
            output_threshold[i] = 0.0;  // the way it was set in nrnbbcore_write.cpp
            output_vindex[i] = -1;
        } else {
            Point_process* pnt = ps->pntsrc_;
            assert(pnt);
            output_vindex[i] = -(pnt->_i_instance * 1000 + pnt->_type);
        }
    }
    fh.write_array<int>(output_vindex, nt.n_presyn);
    fh.write_array<double>(output_threshold, nt.ncell);
#if CHKPNTDEBUG
    for (int i = 0; i < nt.n_presyn; ++i) {
        assert(ntc.output_vindex[i] == output_vindex[i]);
    }
    for (int i = 0; i < nt.ncell; ++i) {
        assert(ntc.output_threshold[i] == output_threshold[i]);
    }
#endif
    delete[] output_vindex;
    delete[] output_threshold;
    delete[] pinv_nt;

    int synoffset = 0;
    std::vector<int> pnt_offset(memb_func.size(), -1);
    for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
        int type = tml->index;
        if (corenrn.get_pnt_map()[type] > 0) {
            pnt_offset[type] = synoffset;
            synoffset += tml->ml->nodecount;
        }
    }

    int* pnttype = new int[nnetcon];
    int* pntindex = new int[nnetcon];
    double* delay = new double[nnetcon];
    for (int i = 0; i < nnetcon; ++i) {
        NetCon& nc = nt.netcons[i];
        Point_process* pnt = nc.target_;
        if (pnt == nullptr) {
            // nrn_setup.cpp allows type <=0 which generates nullptr target.
            pnttype[i] = 0;
            pntindex[i] = -1;
        } else {
            pnttype[i] = pnt->_type;

            // todo: this seems most natural, but does not work. Perhaps should look
            // into how pntindex determined in nrnbbcore_write.cpp and change there.
            // int ix = pnt->_i_instance;
            // if (ml_pinv[pnt->_type]) {
            //     ix = ml_pinv[pnt->_type][ix];
            // }

            // follow the inverse of nrn_setup.cpp using pnt_offset computed above.
            int ix = (pnt - nt.pntprocs) - pnt_offset[pnt->_type];
            pntindex[i] = ix;
        }
        delay[i] = nc.delay_;
    }
    fh.write_array<int>(pnttype, nnetcon);
    fh.write_array<int>(pntindex, nnetcon);
    fh.write_array<double>(nt.weights, nt.n_weight);
    fh.write_array<double>(delay, nnetcon);
#if CHKPNTDEBUG
    for (int i = 0; i < nnetcon; ++i) {
        assert(ntc.pnttype[i] == pnttype[i]);
        assert(ntc.pntindex[i] == pntindex[i]);
        assert(ntc.delay[i] == delay[i]);
    }
#endif
    delete[] pnttype;
    delete[] pntindex;
    delete[] delay;

    // BBCOREPOINTER
    int nbcp = 0;
    for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
        if (corenrn.get_bbcore_read()[tml->index] && tml->index != patstimtype) {
            ++nbcp;
        }
    }

    fh << nbcp << " bbcorepointer\n";
#if CHKPNTDEBUG
    assert(nbcp == ntc.nbcp);
#endif
    nbcp = 0;
    for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
        if (corenrn.get_bbcore_read()[tml->index] && tml->index != patstimtype) {
            int i = nbcp++;
            int type = tml->index;
            assert(corenrn.get_bbcore_write()[type]);
            Memb_list* ml = tml->ml;
            double* d = nullptr;
            Datum* pd = nullptr;
            int layout = corenrn.get_mech_data_layout()[type];
            int dsz = corenrn.get_prop_param_size()[type];
            int pdsz = corenrn.get_prop_dparam_size()[type];
            int aln_cntml = nrn_soa_padded_size(ml->nodecount, layout);
            fh << type << "\n";
            int icnt = 0;
            int dcnt = 0;
            // data size and allocate
            for (int j = 0; j < ml->nodecount; ++j) {
                int jp = j;
                if (ml->_permute) {
                    jp = ml->_permute[j];
                }
                d = ml->data + nrn_i_layout(jp, ml->nodecount, 0, dsz, layout);
                pd = ml->pdata + nrn_i_layout(jp, ml->nodecount, 0, pdsz, layout);
                (*corenrn.get_bbcore_write()[type])(
                    nullptr, nullptr, &dcnt, &icnt, 0, aln_cntml, d, pd, ml->_thread, &nt, 0.0);
            }
            fh << icnt << "\n";
            fh << dcnt << "\n";
#if CHKPNTDEBUG
            assert(ntc.bcptype[i] == type);
            assert(ntc.bcpicnt[i] == icnt);
            assert(ntc.bcpdcnt[i] == dcnt);
#endif
            int* iArray = nullptr;
            double* dArray = nullptr;
            if (icnt) {
                iArray = new int[icnt];
            }
            if (dcnt) {
                dArray = new double[dcnt];
            }
            icnt = dcnt = 0;
            for (int j = 0; j < ml->nodecount; j++) {
                int jp = j;

                if (ml->_permute) {
                    jp = ml->_permute[j];
                }

                d = ml->data + nrn_i_layout(jp, ml->nodecount, 0, dsz, layout);
                pd = ml->pdata + nrn_i_layout(jp, ml->nodecount, 0, pdsz, layout);

                (*corenrn.get_bbcore_write()[type])(
                    dArray, iArray, &dcnt, &icnt, 0, aln_cntml, d, pd, ml->_thread, &nt, 0.0);
            }

            if (icnt) {
                fh.write_array<int>(iArray, icnt);
                delete[] iArray;
            }

            if (dcnt) {
                fh.write_array<double>(dArray, dcnt);
                delete[] dArray;
            }
            ++i;
        }
    }

    fh << nt.n_vecplay << " VecPlay instances\n";
    for (int i = 0; i < nt.n_vecplay; i++) {
        PlayRecord* pr = (PlayRecord*) nt._vecplay[i];
        int vtype = pr->type();
        int mtype = -1;
        int ix = -1;

        // not as efficient as possible but there should not be too many
        Memb_list* ml = nullptr;
        for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
            ml = tml->ml;
            int nn = corenrn.get_prop_param_size()[tml->index] * ml->nodecount;
            if (nn && pr->pd_ >= ml->data && pr->pd_ < (ml->data + nn)) {
                mtype = tml->index;
                ix = (pr->pd_ - ml->data);
                break;
            }
        }
        assert(mtype >= 0);
        int icnt, isz;
        nrn_inverse_i_layout(ix,
                             icnt,
                             ml->nodecount,
                             isz,
                             corenrn.get_prop_param_size()[mtype],
                             corenrn.get_mech_data_layout()[mtype]);
        if (ml_pinv[mtype]) {
            icnt = ml_pinv[mtype][icnt];
        }
        ix = nrn_i_layout(
            icnt, ml->nodecount, isz, corenrn.get_prop_param_size()[mtype], 1 /*AOS_LAYOUT*/);

        fh << vtype << "\n";
        fh << mtype << "\n";
        fh << ix << "\n";
#if CHKPNTDEBUG
        assert(ntc.vtype[i] == vtype);
        assert(ntc.mtype[i] == mtype);
        assert(ntc.vecplay_ix[i] == ix);
#endif
        if (vtype == VecPlayContinuousType) {
            VecPlayContinuous* vpc = (VecPlayContinuous*) pr;
            int sz = vpc->y_.size();
            fh << sz << "\n";
            fh.write_array<double>(vpc->y_.data(), sz);
            fh.write_array<double>(vpc->t_.data(), sz);
        } else {
            std::cerr << "Error checkpointing vecplay type" << std::endl;
            assert(0);
        }
    }

    for (size_t i = 0; i < memb_func.size(); ++i) {
        if (ml_pinv[i]) {
            delete[] ml_pinv[i];
        }
    }
    free(ml_pinv);

    write_tqueue(nt, fh);
    fh.close();
}

static void write_time(const char* output_dir) {
    std::ostringstream filename;
    FileHandler f;
    filename << output_dir << "/time.dat";
    f.open(filename.str(), std::ios::out);
    f.write_array(&t, 1);
    f.close();
}

/// todo : need to broadcast this rather than all reading a double
double restore_time(const char* restore_dir) {
    double rtime = 0;
    if (strlen(restore_dir)) {
        std::ostringstream filename;
        FileHandler f;
        filename << restore_dir << "/time.dat";
        f.open(filename.str(), std::ios::in);
        f.read_array(&rtime, 1);
        f.close();
    }
    return rtime;
}

static void write_tqueue(TQItem* q, NrnThread& nt, FileHandlerWrap& fh) {
    DiscreteEvent* d = (DiscreteEvent*) q->data_;

    // printf("  p %.20g %d\n", q->t_, d->type());
    // d->pr("", q->t_, net_cvode_instance);

    if (!d->require_checkpoint()) {
        return;
    }

    fh << d->type() << "\n";
    fh.write_array(&q->t_, 1);

    switch (d->type()) {
        case NetConType: {
            NetCon* nc = (NetCon*) d;
            assert(nc >= nt.netcons && (nc < (nt.netcons + nt.n_netcon)));
            fh << (nc - nt.netcons) << "\n";
            break;
        }
        case SelfEventType: {
            SelfEvent* se = (SelfEvent*) d;
            fh << int(se->target_->_type) << "\n";
            fh << se->target_ - nt.pntprocs << "\n";  // index of nrnthread.pntprocs
            fh << se->target_->_i_instance << "\n";   // not needed except for assert check
            fh.write_array(&se->flag_, 1);
            fh << (se->movable_ - nt._vdata) << "\n";  // DANGEROUS?
            fh << se->weight_index_ << "\n";
            // printf("    %d %ld %d %g %ld %d\n", se->target_->_type, se->target_ - nt.pntprocs,
            // se->target_->_i_instance, se->flag_, se->movable_ - nt._vdata, se->weight_index_);
            break;
        }
        case PreSynType: {
            PreSyn* ps = (PreSyn*) d;
            assert(ps >= nt.presyns && (ps < (nt.presyns + nt.n_presyn)));
            fh << (ps - nt.presyns) << "\n";
            break;
        }
        case NetParEventType: {
            // nothing extra to write
            break;
        }
        case PlayRecordEventType: {
            PlayRecord* pr = ((PlayRecordEvent*) d)->plr_;
            fh << pr->type() << "\n";
            if (pr->type() == VecPlayContinuousType) {
                VecPlayContinuous* vpc = (VecPlayContinuous*) pr;
                int ix = -1;
                for (int i = 0; i < nt.n_vecplay; ++i) {
                    // if too many for fast search, put ix in the instance
                    if (nt._vecplay[i] == (void*) vpc) {
                        ix = i;
                        break;
                    }
                }
                assert(ix >= 0);
                fh << ix << "\n";
            } else {
                assert(0);
            }
            break;
        }
        default: {
            // In particular, InputPreSyn does not appear in tqueue as it
            // immediately fans out to NetCon.
            assert(0);
            break;
        }
    }
}

static int patstim_index;
static double patstim_te;

static void checkpoint_restore_tqitem(int type,
                                      std::shared_ptr<Phase2::EventTypeBase> event,
                                      NrnThread& nt) {
    // printf("restore tqitem type=%d time=%.20g\n", type, time);

    switch (type) {
        case NetConType: {
            auto e = static_cast<Phase2::NetConType_*>(event.get());
            // printf("  NetCon %d\n", netcon_index);
            NetCon* nc = nt.netcons + e->netcon_index;
            nc->send(e->time, net_cvode_instance, &nt);
            break;
        }
        case SelfEventType: {
            auto e = static_cast<Phase2::SelfEventType_*>(event.get());
            if (e->target_type == patstimtype) {
                if (nt.id == 0) {
                    patstim_te = e->time;
                }
                break;
            }
            Point_process* pnt = nt.pntprocs + e->point_proc_instance;
            // printf("  SelfEvent %d %d %d %g %d %d\n", target_type, point_proc_instance,
            // target_instance, flag, movable, weight_index);
            nrn_assert(e->target_instance == pnt->_i_instance);
            nrn_assert(e->target_type == pnt->_type);
            net_send(nt._vdata + e->movable, e->weight_index, pnt, e->time, e->flag);
            break;
        }
        case PreSynType: {
            auto e = static_cast<Phase2::PreSynType_*>(event.get());
            // printf("  PreSyn %d\n", presyn_index);
            PreSyn* ps = nt.presyns + e->presyn_index;
            int gid = ps->output_index_;
            ps->output_index_ = -1;
            ps->send(e->time, net_cvode_instance, &nt);
            ps->output_index_ = gid;
            break;
        }
        case NetParEventType: {
            // nothing extra to read
            // printf("  NetParEvent\n");
            break;
        }
        case PlayRecordEventType: {
            auto e = static_cast<Phase2::PlayRecordEventType_*>(event.get());
            VecPlayContinuous* vpc = (VecPlayContinuous*) (nt._vecplay[e->vecplay_index]);
            vpc->e_->send(e->time, net_cvode_instance, &nt);
            break;
        }
        default: {
            assert(0);
            break;
        }
    }
}

extern int checkpoint_save_patternstim(_threadargsproto_);
extern void checkpoint_restore_patternstim(int, double, _threadargsproto_);

static void write_tqueue(NrnThread& nt, FileHandlerWrap& fh) {
    // VecPlayContinuous
    fh << nt.n_vecplay << " VecPlayContinuous state\n";
    for (int i = 0; i < nt.n_vecplay; ++i) {
        VecPlayContinuous* vpc = (VecPlayContinuous*) nt._vecplay[i];
        fh << vpc->last_index_ << "\n";
        fh << vpc->discon_index_ << "\n";
        fh << vpc->ubound_index_ << "\n";
    }

    // PatternStim
    int patstim_index = -1;
    for (NrnThreadMembList* tml = nrn_threads[0].tml; tml; tml = tml->next) {
        if (tml->index == patstimtype) {
            Memb_list* ml = tml->ml;
            patstim_index = checkpoint_save_patternstim(
                /* below correct only for AoS */
                0,
                ml->nodecount,
                ml->data,
                ml->pdata,
                ml->_thread,
                nrn_threads,
                0.0);
            break;
        }
    }
    fh << patstim_index << " PatternStim\n";

    // Avoid extra spikes due to some presyn voltages above threshold
    fh << -1 << " Presyn ConditionEvent flags\n";
    for (int i = 0; i < nt.n_presyn; ++i) {
        // PreSyn.flag_ not used. HPC memory utilizes PreSynHelper.flag_ array
        fh << nt.presyns_helper[i].flag_ << "\n";
    }

    NetCvodeThreadData& ntd = net_cvode_instance->p[nt.id];
    // printf("write_tqueue %d %p\n", nt.id, ndt.tqe_);
    TQueue<QTYPE>* tqe = ntd.tqe_;
    TQItem* q;

    fh << -1 << " TQItems from atomic_dq\n";
    while ((q = tqe->atomic_dq(1e20)) != nullptr) {
        write_tqueue(q, nt, fh);
    }
    fh << 0 << "\n";
    fh << -1 << " TQItemsfrom binq_\n";
    for (q = tqe->binq_->first(); q; q = tqe->binq_->next(q)) {
        write_tqueue(q, nt, fh);
    }
    fh << 0 << "\n";
}

static bool checkpoint_restored_ = false;

// Read a tqueue/checkpoint
// int :: should be equal to the previous n_vecplay
// n_vecplay:
//   int: last_index
//   int: discon_index
//   int: ubound_index
// int: patstim_index
// int: should be -1
// n_presyn:
//   int: flags of presyn_helper
// int: should be -1
// null terminated:
//   int: type
//   array of size 1:
//     double: time
//   ... depends of the type
// int: should be -1
// null terminated:
//   int: TO BE DEFINED
//   ... depends of the type
void checkpoint_restore_tqueue(NrnThread& nt, const Phase2& p2) {
    checkpoint_restored_ = true;

    for (int i = 0; i < nt.n_vecplay; ++i) {
        VecPlayContinuous* vpc = (VecPlayContinuous*) nt._vecplay[i];
        auto& vec = p2.vec_play_continuous[i];
        vpc->last_index_ = vec.last_index;
        vpc->discon_index_ = vec.discon_index;
        vpc->ubound_index_ = vec.ubound_index;
    }

    // PatternStim
    patstim_index = p2.patstim_index;  // PatternStim
    if (nt.id == 0) {
        patstim_te = -1.0;  // changed if relevant SelfEvent;
    }

    for (int i = 0; i < nt.n_presyn; ++i) {
        nt.presyns_helper[i].flag_ = p2.preSynConditionEventFlags[i];
    }

    for (const auto& event: p2.events) {
        checkpoint_restore_tqitem(event.first, event.second, nt);
    }
}

// A call to finitialize must be avoided after restoring the checkpoint
// as that would change all states to a voltage clamp initialization.
// Nevertheless t and some spike exchange and other computer state needs to
// be initialized.
// Also it is occasionally the case that nrn_init allocates data so we
// need to call it but avoid the internal call to initmodel.
// Consult finitialize.c to help decide what should be here
bool checkpoint_initialize() {
    dt2thread(-1.);
    nrn_thread_table_check();
    nrn_spike_exchange_init();

    // in case some nrn_init allocate data we need to do that but do not
    // want to call initmodel.
    _nrn_skip_initmodel = true;
    for (int i = 0; i < nrn_nthread; ++i) {  // should be parallel
        NrnThread& nt = nrn_threads[i];
        for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
            Memb_list* ml = tml->ml;
            mod_f_t s = corenrn.get_memb_func(tml->index).initialize;
            if (s) {
                (*s)(&nt, ml, tml->index);
            }
        }
    }
    _nrn_skip_initmodel = false;

    // if PatternStim exists, needs initialization
    for (NrnThreadMembList* tml = nrn_threads[0].tml; tml; tml = tml->next) {
        if (tml->index == patstimtype && patstim_index >= 0 && patstim_te > 0.0) {
            Memb_list* ml = tml->ml;
            checkpoint_restore_patternstim(patstim_index,
                                           patstim_te,
                                           /* below correct only for AoS */
                                           0,
                                           ml->nodecount,
                                           ml->data,
                                           ml->pdata,
                                           ml->_thread,
                                           nrn_threads,
                                           0.0);
            break;
        }
    }

    return checkpoint_restored_;
}
}  // namespace coreneuron
