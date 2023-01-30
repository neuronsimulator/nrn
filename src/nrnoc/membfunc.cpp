#include "membfunc.h"
#include "multicore.h"

#include <cassert>

void Memb_func::invoke_initialize(neuron::model_sorted_token const& sorted_token,
                                  NrnThread* nt,
                                  Memb_list* ml,
                                  int mech_type) const {
    assert(has_initialize());
    if (ml->type() != mech_type) {
        throw std::runtime_error("Memb_func::invoke_initialize(nt[" + std::to_string(nt->id) +
                                 "], ml, " + std::to_string(mech_type) +
                                 "): type mismatch, ml->type()=" + std::to_string(ml->type()));
    }
    m_initialize(sorted_token, nt, ml, mech_type);
}
