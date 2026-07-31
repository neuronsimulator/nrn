#include "neuron/gpu/ion_cur_device.hpp"

#include "neuron/gpu/device_state.hpp"
#include "neuron/gpu/offload.hpp"

#include "membfunc.h"
#include "multicore.h"
#include "nrn_ansi.h"
#include "nrnoc_ml.h"
#include "nrnunits.h"

#include <cmath>
#include <mutex>
#include <vector>

extern double celsius;

namespace neuron::gpu {
namespace {

constexpr int iontype_index_dparam = 0;
constexpr int erev_index = 0;
constexpr int conci_index = 1;
constexpr int conco_index = 2;
constexpr int cur_index = 3;
constexpr int dcurdv_index = 4;

double ion_nernst_on_device(double ci, double co, double z, double celsius_val) {
    if (z == 0.) {
        return 0.;
    }
    if (ci <= 0.) {
        return 1e6;
    }
    if (co <= 0.) {
        return -1e6;
    }
    auto const ktf_val =
        1000. * _gasconstant_codata2018 * (celsius_val + 273.15) / _faraday_codata2018;
    return ktf_val / z * std::log(co / ci);
}

/**
 * Process-wide iontype staging for device ion_cur (not per-call std::vector).
 * Free-before-grow: dropping device map at known capacity before host realloc
 * avoids OpenACC partial-present when the allocator reuses the address.
 * Mutex: OpenACC host APIs + shared staging are not multi-thread safe.
 */
struct IontypeStaging {
    std::vector<int> host;
    std::size_t device_count{0};  // 0 = not present on device
    std::mutex mutex;
};

IontypeStaging& iontype_staging() {
    static IontypeStaging s;
    return s;
}

void iontype_staging_drop_device(IontypeStaging& s) {
    if (s.device_count == 0 || s.host.empty()) {
        s.device_count = 0;
        return;
    }
    nrn_target_delete(s.host.data(), s.device_count);
    s.device_count = 0;
}

}  // namespace

bool ion_cur_on_device(neuron::model_sorted_token const& sorted_token,
                       NrnThread& nt,
                       Memb_list& ml,
                       int type,
                       double charge) {
#if defined(NRN_ENABLE_GPU)
    if (!nt.compute_gpu || !model_is_on_device()) {
        return false;
    }
    auto* const gpu_data = neuron::mechanism::_get::gpu_data_ptrs(sorted_token, type);
    if (!gpu_data) {
        return false;
    }
    int const count = ml.nodecount;
    if (count <= 0) {
        return true;
    }

    auto& staging = iontype_staging();
    std::lock_guard<std::mutex> const lock{staging.mutex};

    // Free-before-grow host vector (device map must match host.data()).
    if (static_cast<std::size_t>(count) > staging.host.capacity()) {
        iontype_staging_drop_device(staging);
        staging.host.reserve(static_cast<std::size_t>(count));
    } else if (staging.device_count != 0 &&
               staging.device_count != static_cast<std::size_t>(count)) {
        // Present at a different length — drop before re-copyin.
        iontype_staging_drop_device(staging);
    }
    staging.host.resize(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        staging.host[static_cast<std::size_t>(i)] =
            ml.pdata[i][iontype_index_dparam].get<int>();
    }

    int* const iontypes_host = staging.host.data();
    if (staging.device_count == static_cast<std::size_t>(count) &&
        nrn_target_is_present(iontypes_host)) {
        nrn_target_update_on_device(iontypes_host, static_cast<std::size_t>(count));
    } else {
        iontype_staging_drop_device(staging);
        (void) nrn_target_copyin(iontypes_host, static_cast<std::size_t>(count));
        staging.device_count = static_cast<std::size_t>(count);
    }
    int* const iontypes_dev = nrn_target_deviceptr(iontypes_host);

    auto const offset = ml.get_storage_offset();
    double* const erev_dev = gpu_data[erev_index] + offset;
    double* const conci_dev = gpu_data[conci_index] + offset;
    double* const conco_dev = gpu_data[conco_index] + offset;
    double* const cur_dev = gpu_data[cur_index] + offset;
    double* const dcurdv_dev = gpu_data[dcurdv_index] + offset;
    double* const d_celsius = static_cast<double*>(nrn_target_deviceptr(&celsius));
    // clang-format off
    nrn_pragma_acc(parallel loop deviceptr(iontypes_dev, erev_dev, conci_dev, conco_dev,
                                          cur_dev, dcurdv_dev, d_celsius)
                       async(nt.stream_id) if (nt.compute_gpu))
    // clang-format on
    nrn_pragma_omp(target teams distribute parallel for if (nt.compute_gpu))
    for (int i = 0; i < count; ++i) {
        auto const iontype = iontypes_dev[i];
        dcurdv_dev[i] = 0.0;
        cur_dev[i] = 0.0;
        if (iontype & 0100) {
            erev_dev[i] = ion_nernst_on_device(conci_dev[i], conco_dev[i], charge, *d_celsius);
        }
    }
    nrn_pragma_acc(wait(nt.stream_id))
    // Keep staging on device for reuse at the same count (dropped on capacity change).
    return true;
#else
    (void) sorted_token;
    (void) nt;
    (void) ml;
    (void) type;
    (void) charge;
    return false;
#endif
}

}  // namespace neuron::gpu