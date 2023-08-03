#include <../../nrnconf.h>

#include <vector>
#include <errno.h>
#include "nrniv_mf.h"
#include <nrnoc2iv.h>
#include <nrnmpi.h>

/*
Started out attempting a general implementation in which the subtrees could be
on any host and any number of them. That required a distinct gid for each
subtree.  However, since the only purpose of cellsplit is for load balance
efficiency, we settled on a very restrictive but still adequate policy of
subtrees being on adjacent hosts and at most one cell split between i and i+1
and at most one cell split between i and i-1. We also disallow a split where
both subtrees are on the same host. This policy allows a very simple and direct
setting up and transfer of matrix information. Note that gid information about
the subtrees is no longer required by this implementation.
*/

#if NRNMPI
void nrnmpi_split_clear();
extern void (*nrnmpi_splitcell_compute_)();
extern void nrnmpi_send_doubles(double*, int cnt, int dest, int tag);
extern void nrnmpi_recv_doubles(double*, int cnt, int src, int tag);
extern double nrnmpi_splitcell_wait_;

static int change_cnt_;
static void transfer();
static void set_structure();
static void splitcell_compute();

struct SplitCell {
    Section* rootsec_;
    int that_host_;
};

static std::vector<SplitCell> splitcell_list_;

#define ip 0
#define im 2
static bool splitcell_connected_[2];
static double* transfer_p_[4];

#endif

// that_host must be adjacent to nrnmpi_myid
void nrnmpi_splitcell_connect(int that_host) {
#if NRNMPI
    Section* rootsec = chk_access();
    if (std::abs(nrnmpi_myid - that_host) != 1) {
        hoc_execerror("cells may be split only on adjacent hosts", 0);
    }
    if (that_host < 0 || that_host >= nrnmpi_numprocs) {
        hoc_execerror("adjacent host out of range", 0);
    }
    if (rootsec->parentsec) {
        hoc_execerror(secname(rootsec), "is not a root section");
    }
    nrnmpi_splitcell_compute_ = splitcell_compute;
    for (size_t i = 0; i < 2; ++i)
        if (that_host == nrnmpi_myid + i * 2 - 1) {
            if (splitcell_connected_[i]) {
                char buf[100];
                Sprintf(buf, "%d and %d", nrnmpi_myid, that_host);
                hoc_execerror("splitcell connection already exists between hosts", buf);
            }
            splitcell_connected_[i] = true;
        }
    splitcell_list_.push_back({rootsec, that_host});
#else
    if (nrnmpi_myid == 0) {
        hoc_execerror("ParallelContext.splitcell not available.",
                      "NEURON not configured with --with-paranrn");
    }
#endif
}

#if NRNMPI

void nrnmpi_split_clear() {
    if (nrnmpi_splitcell_compute_ == splitcell_compute) {
        if (nrnmpi_myid == 0) {
            hoc_execerror("nrnmpi_split_clear ", "not implemented");
        }
    }
}

void splitcell_compute() {
    if (structure_change_cnt != change_cnt_) {
        set_structure();
        change_cnt_ = structure_change_cnt;
    }
    transfer();
}

void transfer() {
    double trans[2], trans_sav[2];
    double wt = nrnmpi_wtime();
    if (transfer_p_[ip]) {
        trans[0] = *transfer_p_[ip + 0];
        trans[1] = *transfer_p_[ip + 1];
        nrnmpi_send_doubles(trans, 2, nrnmpi_myid + 1, 1);
    }
    if (transfer_p_[im]) {
        nrnmpi_recv_doubles(trans_sav, 2, nrnmpi_myid - 1, 1);
        // defer copying to the d and rhs variables since the
        // old info needs to be transferred next
        trans[0] = *transfer_p_[im + 0];
        trans[1] = *transfer_p_[im + 1];
        *transfer_p_[im + 0] += trans_sav[0];
        *transfer_p_[im + 1] += trans_sav[1];
        nrnmpi_send_doubles(trans, 2, nrnmpi_myid - 1, 1);
    }
    if (transfer_p_[ip]) {
        nrnmpi_recv_doubles(trans, 2, nrnmpi_myid + 1, 1);
        *transfer_p_[ip + 0] += trans[0];
        *transfer_p_[ip + 1] += trans[1];
    }
    nrnmpi_splitcell_wait_ += nrnmpi_wtime() - wt;
    errno = 0;
}

void set_structure() {
    for (auto& sc: splitcell_list_) {
        if (sc.that_host_ == nrnmpi_myid + 1) {
            transfer_p_[ip + 0] = &NODED(sc.rootsec_->parentnode);
            transfer_p_[ip + 1] = &NODERHS(sc.rootsec_->parentnode);
        } else {
            assert(sc.that_host_ == nrnmpi_myid - 1);
            transfer_p_[im + 0] = &NODED(sc.rootsec_->parentnode);
            transfer_p_[im + 1] = &NODERHS(sc.rootsec_->parentnode);
        }
    }
}

#endif
