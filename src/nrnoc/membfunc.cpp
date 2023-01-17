#include "membfunc.h"

#include <cassert>

void Memb_func::invoke_initialize(NrnThread* nt, Memb_list* ml, int mech_type) const {
    assert(has_initialize());
    m_initialize(nt, ml, mech_type);
}
