/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#ifndef _H_NRNCHECKPOINT_
#define _H_NRNCHECKPOINT_

#include "coreneuron/io/phase2.hpp"

namespace coreneuron {
class NrnThread;
class FileHandler;

extern bool nrn_checkpoint_arg_exists;

void write_checkpoint(NrnThread* nt, int nb_threads, const char* dir);

void checkpoint_restore_tqueue(NrnThread&, const Phase2& p2);

int* inverse_permute(int* p, int n);
void nrn_inverse_i_layout(int i, int& icnt, int cnt, int& isz, int sz, int layout);

/* return true if special checkpoint initialization carried out and
   one should not do finitialize
*/
bool checkpoint_initialize();

/** return time to start simulation : if restore_dir provided
 *  then tries to read time.dat file otherwise returns 0
 */
double restore_time(const char* restore_path);

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

struct Memb_list_ckpnt {
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
#endif
