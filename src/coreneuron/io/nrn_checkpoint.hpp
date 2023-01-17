/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#pragma once

#include "coreneuron/io/phase2.hpp"

namespace coreneuron {
struct NrnThread;
class FileHandler;

class CheckPoints {
  public:
    CheckPoints(const std::string& save, const std::string& restore);
    std::string get_save_path() const {
        return save_;
    }
    std::string get_restore_path() const {
        return restore_;
    }
    bool should_save() const {
        return !save_.empty();
    }
    bool should_restore() const {
        return !restore_.empty();
    }
    double restore_time() const;
    void write_checkpoint(NrnThread* nt, int nb_threads) const;
    /* return true if special checkpoint initialization carried out and
       one should not do finitialize
     */
    bool initialize();
    void restore_tqueue(NrnThread&, const Phase2& p2);

  private:
    const std::string save_;
    const std::string restore_;
    bool restored;
    int patstim_index;
    double patstim_te;

    void write_time() const;
    void write_phase2(NrnThread& nt) const;

    template <typename T>
    void data_write(FileHandler& F, T* data, int cnt, int sz, int layout, int* permute) const;
    template <typename T>
    T* soa2aos(T* data, int cnt, int sz, int layout, int* permute) const;
    void write_tqueue(TQItem* q, NrnThread& nt, FileHandler& fh) const;
    void write_tqueue(NrnThread& nt, FileHandler& fh) const;
    void restore_tqitem(int type, std::shared_ptr<Phase2::EventTypeBase> event, NrnThread& nt);
};


int* inverse_permute(int* p, int n);
void nrn_inverse_i_layout(int i, int& icnt, int cnt, int& isz, int sz, int layout);

extern int patstimtype;

#ifndef CHKPNTDEBUG
#define CHKPNTDEBUG 0
#endif

#if CHKPNTDEBUG
// Factored out from checkpoint changes to nrnoc/multicore.h and nrnoc/nrnoc_ml.h
// Put here to avoid potential issues with gpu transfer and to allow
// debugging comparison with respect to checkpoint writing to verify that
// data is same as on reading when inverse transforming SoA and permutations.
// Following is a mixture of substantive information which is lost during
// nrn_setup.cpp and debugging only information which is retrievable from
// NrnThread and Memb_list. Ideally, this should all go away

struct Memb_list_chkpnt {
    // debug only
    double* data_not_permuted;
    Datum* pdata_not_permuted;
    int* nodeindices_not_permuted;
};

#endif  // CHKPNTDEBUG but another section for it below

struct NrnThreadChkpnt {
    int file_id;

#if CHKPNTDEBUG
    int nmech;
    double* area;
    int* parent;
    Memb_list_chkpnt** mlmap;

    int n_outputgids;
    int* output_vindex;
    double* output_threshold;

    int* pnttype;
    int* pntindex;
    double* delay;

    // BBCOREPOINTER
    int nbcp;
    int* bcptype;
    int* bcpicnt;
    int* bcpdcnt;

    // VecPlay
    int* vtype;
    int* mtype;
    int* vecplay_ix;
#endif  // CHKPNTDEBUG
};

extern NrnThreadChkpnt* nrnthread_chkpnt;
}  // namespace coreneuron
