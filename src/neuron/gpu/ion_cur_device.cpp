#include "neuron/gpu/ion_cur_device.hpp"

#include "neuron/gpu/device_state.hpp"
#include "neuron/gpu/offload.hpp"

#include "membfunc.h"
#include "multicore.h"
#include "nrn_ansi.h"
#include "nrnoc_ml.h"
#include "nrnunits.h"

#include <cmath>
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
    std::vector<int> iontypes(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        iontypes[static_cast<std::size_t>(i)] =
            ml.pdata[i][iontype_index_dparam].get<int>();
    }
    int* const iontypes_dev =
        nrn_target_copyin(iontypes.data(), static_cast<std::size_t>(count));
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
    nrn_target_delete(iontypes.data(), static_cast<std::size_t>(count));
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