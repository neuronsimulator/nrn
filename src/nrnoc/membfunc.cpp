#include "hocdec.h"
#include "membfunc.h"
#include "multicore.h"
#include "neuron/cache/model_data.hpp"

#include <cassert>

// namespace {
// bool datum_equal(Datum const& lhs, Datum const& rhs) {
//     return std::memcmp(&lhs, &rhs, sizeof(Datum)) == 0;
// }
// }
// See also call_net_receive
void Memb_func::invoke_initialize(NrnThread* nt, Memb_list* ml, int mech_type) const {
    assert(has_initialize());
    // auto model_cache = neuron::cache::acquire_valid();
    // auto const& mech_pdata = nt->mech_cache(mech_type).pdata;
    // auto const before = mech_pdata;
    m_initialize(nt, ml, mech_type);
    // for (auto i = 0; i < mech_pdata.size(); ++i) { // instance
    //   for (auto j = 0; j < mech_pdata[i].size(); ++j) { // variable
    //     auto const& old_value = before[i][j];
    //     auto const& new_value = mech_pdata[i][j];
    //     auto const equal = datum_equal(old_value, new_value);
    //     if (!equal) {
    //       std::cout << nt << ' ' << ml << ' ' << mech_type << ' ' << i << ' ' << j << ' ' <<
    //       equal << std::endl;
    //       // Sync the changes back out of the cache data into the main model
    //       // TODO if this is a data handle then need to store it as one
    //       auto const var_semantics = memb_func[mech_type].dparam_semantics[j];
    //       if (var_semantics == -5) {
    //           // POINTER variable
    //           assert(false);
    //       } else {
    //           ml->pdata[i][j] = new_value;
    //       }
    //     }
    //   }
    // }
}
