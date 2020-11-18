/*
Copyright (c) 2016, Blue Brain Project
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _H_NRNCHECKPOINT_
#define _H_NRNCHECKPOINT_

#include "coreneuron/io/phase2.hpp"

namespace coreneuron {
class NrnThread;
class FileHandler;

extern bool nrn_checkpoint_arg_exists;

void write_checkpoint(NrnThread* nt,
                      int nb_threads,
                      const char* dir);

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
