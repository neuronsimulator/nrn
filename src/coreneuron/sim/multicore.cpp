/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#include <cstdlib>
#include <vector>

#include "coreneuron/nrnconf.h"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/utils/memory.h"
#include "coreneuron/coreneuron.hpp"
#include "coreneuron/utils/nrnoc_aux.hpp"

/*
Now that threads have taken over the actual_v, v_node, etc, it might
be a good time to regularize the method of freeing, allocating, and
updating those arrays. To recapitulate the history, Node used to
be the structure that held direct values for v, area, d, rhs, etc.
That continued to hold for the cray vectorization project which
introduced v_node, v_parent, memb_list. Cache efficiency introduced
actual_v, actual_area, actual_d, etc and the Node started pointing
into those arrays. Additional nodes after allocation required updating
pointers to v and area since those arrays were freed and reallocated.
Now, the threads hold all these arrays and we want to update them
properly under the circumstances of changing topology, changing
number of threads, and changing distribution of cells on threads.
Note there are no longer global versions of any of these arrays.
We do not want to update merely due to a change in area. Recently
we have dealt with diam, area, ri on a section basis. We generally
desire an update just before a simulation when the efficient
structures are necessary. This is reasonably well handled by the
v_structure_change flag which historically freed and reallocated
v_node and v_parent and, just before this comment,
ended up setting the NrnThread tml. This makes most of the old
memb_list vestigial and we now got rid of it except for
the artificial cells (and it is possibly not really necessary there).
Switching between sparse and tree matrix just cause freeing and
reallocation of actual_rhs.

If we can get the freeing, reallocation, and pointer update correct
for _actual_v, I am guessing everything else can be dragged along with
it. We have two major cases, call to pc.nthread and change in
model structure. We want to use Node* as much as possible and defer
the handling of v_structure_change as long as possible.
*/

namespace coreneuron {

CoreNeuron corenrn;

int nrn_nthread = 0;
NrnThread* nrn_threads = nullptr;
void (*nrn_mk_transfer_thread_data_)();

/// --> CoreNeuron class
static int table_check_cnt_;
static ThreadDatum* table_check_;


NrnThreadMembList* create_tml(NrnThread& nt,
                              int mech_id,
                              Memb_func& memb_func,
                              int& shadow_rhs_cnt,
                              const std::vector<int>& mech_types,
                              const std::vector<int>& nodecounts) {
    auto tml = (NrnThreadMembList*) emalloc_align(sizeof(NrnThreadMembList), 0);
    tml->next = nullptr;
    tml->index = mech_types[mech_id];

    tml->ml = (Memb_list*) ecalloc_align(1, sizeof(Memb_list), 0);
    tml->ml->_net_receive_buffer = nullptr;
    tml->ml->_net_send_buffer = nullptr;
    tml->ml->_permute = nullptr;
    if (memb_func.alloc == nullptr) {
        hoc_execerror(memb_func.sym, "mechanism does not exist");
    }
    tml->ml->nodecount = nodecounts[mech_id];
    if (!memb_func.sym) {
        printf("%s (type %d) is not available\n", nrn_get_mechname(tml->index), tml->index);
        exit(1);
    }
    tml->ml->_nodecount_padded = nrn_soa_padded_size(tml->ml->nodecount,
                                                     corenrn.get_mech_data_layout()[tml->index]);
    if (memb_func.is_point && corenrn.get_is_artificial()[tml->index] == 0) {
        // Avoid race for multiple PointProcess instances in same compartment.
        if (tml->ml->nodecount > shadow_rhs_cnt) {
            shadow_rhs_cnt = tml->ml->nodecount;
        }
    }

    if (auto* const priv_ctor = corenrn.get_memb_func(tml->index).private_constructor) {
        priv_ctor(&nt, tml->ml, tml->index);
    }

    return tml;
}

void nrn_threads_create(int n) {
    if (nrn_nthread != n) {
        /*printf("sizeof(NrnThread)=%d   sizeof(Memb_list)=%d\n", sizeof(NrnThread),
         * sizeof(Memb_list));*/

        nrn_threads = nullptr;
        nrn_nthread = n;
        if (n > 0) {
            nrn_threads = new NrnThread[n];
            for (int i = 0; i < nrn_nthread; ++i) {
                NrnThread& nt = nrn_threads[i];
                nt.id = i;
                for (int j = 0; j < BEFORE_AFTER_SIZE; ++j) {
                    nt.tbl[j] = nullptr;
                }
            }
        }
        v_structure_change = 1;
        diam_changed = 1;
    }
    /*printf("nrn_threads_create %d %d\n", nrn_nthread, nrn_thread_parallel_);*/
}

void nrn_threads_free() {
    if (nrn_nthread) {
        delete[] nrn_threads;
        nrn_threads = nullptr;
        nrn_nthread = 0;
    }
}

void nrn_mk_table_check() {
    if (table_check_) {
        free((void*) table_check_);
        table_check_ = nullptr;
    }
    auto& memb_func = corenrn.get_memb_funcs();
    // Allocate int array of size of mechanism types
    std::vector<int> ix(memb_func.size(), -1);
    table_check_cnt_ = 0;
    for (int id = 0; id < nrn_nthread; ++id) {
        auto& nt = nrn_threads[id];
        for (auto tml = nt.tml; tml; tml = tml->next) {
            int index = tml->index;
            if (memb_func[index].thread_table_check_ && ix[index] == -1) {
                ix[index] = id;
                table_check_cnt_ += 2;
            }
        }
    }
    if (table_check_cnt_) {
        table_check_ = (ThreadDatum*) emalloc(table_check_cnt_ * sizeof(ThreadDatum));
    }
    int i = 0;
    for (int id = 0; id < nrn_nthread; ++id) {
        auto& nt = nrn_threads[id];
        for (auto tml = nt.tml; tml; tml = tml->next) {
            int index = tml->index;
            if (memb_func[index].thread_table_check_ && ix[index] == id) {
                table_check_[i++].i = id;
                table_check_[i++]._pvoid = (void*) tml;
            }
        }
    }
}

void nrn_thread_table_check() {
    for (int i = 0; i < table_check_cnt_; i += 2) {
        auto& nt = nrn_threads[table_check_[i].i];
        auto tml = static_cast<NrnThreadMembList*>(table_check_[i + 1]._pvoid);
        Memb_list* ml = tml->ml;
        (*corenrn.get_memb_func(tml->index).thread_table_check_)(
            0, ml->_nodecount_padded, ml->data, ml->pdata, ml->_thread, &nt, ml, tml->index);
    }
}
}  // namespace coreneuron
