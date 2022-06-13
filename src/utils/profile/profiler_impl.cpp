#include "utils/profile/profiler_interface.h"

#if defined(NRN_CALIPER)
#include <caliper/cali-manager.h>

namespace {
  cali::ConfigManager g_caliper_mgr{};
}

void nrn::detail::Caliper::start_profile() {
  g_caliper_mgr.add("mpi-report");
  g_caliper_mgr.start();
}

void nrn::detail::Caliper::flush_profile() {
  g_caliper_mgr.flush();
}

#endif
