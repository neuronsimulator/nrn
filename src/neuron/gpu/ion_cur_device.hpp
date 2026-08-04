#pragma once

#include "neuron/model_data.hpp"

struct Memb_list;
struct NrnThread;

namespace neuron::gpu {

/** Zero ion cur/dcurdv (and optionally update erev) on device. Returns true if handled. */
bool ion_cur_on_device(neuron::model_sorted_token const& sorted_token,
                       NrnThread& nt,
                       Memb_list& ml,
                       int type,
                       double charge);

/** Drop cached iontype device mirrors (topology / ion_style / full re-upload). */
void invalidate_iontype_device_cache() noexcept;

}  // namespace neuron::gpu
