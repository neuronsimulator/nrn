#include "neuron/gpu/post_solve.hpp"

#include "coreneuron/utils/offload.hpp"
#include "membfunc.h"
#include "multicore.h"
#include "neuron/cache/mechanism_range.hpp"
#include "neuron/model_data.hpp"
#include "nrn_ansi.h"
#include "nrnoc_ml.h"

extern int secondorder;

namespace neuron::gpu {
namespace {

constexpr int cap_cm_index = 0;
constexpr int cap_i_cap_index = 1;
constexpr int ion_cur_index = 3;
constexpr int ion_dcurdv_index = 4;

void second_order_cur_on_device(model_sorted_token const& sorted_token, NrnThread& nt) {
    if (secondorder != 2 || nt.end <= 0) {
        return;
    }
    auto* const vec_rhs = nt.node_rhs_storage();
    for (auto* tml = nt.tml; tml; tml = tml->next) {
        if (!nrn_is_ion(tml->index)) {
            continue;
        }
        auto* const ml = tml->ml;
        int const count = ml->nodecount;
        if (count <= 0) {
            continue;
        }
        neuron::cache::MechanismRange<5, 1> ml_cache{sorted_token, *ml};
        double* const cur = ml_cache.data_array_ptr<ion_cur_index, 1>();
        double* const dcurdv = ml_cache.data_array_ptr<ion_dcurdv_index, 1>();
        int* const ni = ml->nodeindices;
        // clang-format off
        nrn_pragma_acc(parallel loop present(cur [0:count],
                                             dcurdv [0:count],
                                             ni [0:count],
                                             vec_rhs [0:nt.end]) if (nt.compute_gpu)
                               async(nt.stream_id))
        // clang-format on
        nrn_pragma_omp(target teams distribute parallel for simd if(nt.compute_gpu))
        for (int i = 0; i < count; ++i) {
            cur[i] += dcurdv[i] * vec_rhs[ni[i]];
        }
    }
}

void update_voltage_on_device(NrnThread& nt) {
    if (nt.end <= 0) {
        return;
    }
    auto* const vec_rhs = nt.node_rhs_storage();
    auto* const vec_v = nt.node_voltage_storage();
    if (secondorder) {
        // clang-format off
        nrn_pragma_acc(parallel loop present(vec_v [0:nt.end], vec_rhs [0:nt.end]) if (nt.compute_gpu)
                           async(nt.stream_id))
        // clang-format on
        nrn_pragma_omp(target teams distribute parallel for simd if(nt.compute_gpu))
        for (int i = 0; i < nt.end; ++i) {
            vec_v[i] += 2. * vec_rhs[i];
        }
    } else {
        // clang-format off
        nrn_pragma_acc(parallel loop present(vec_v [0:nt.end], vec_rhs [0:nt.end]) if (nt.compute_gpu)
                           async(nt.stream_id))
        // clang-format on
        nrn_pragma_omp(target teams distribute parallel for simd if(nt.compute_gpu))
        for (int i = 0; i < nt.end; ++i) {
            vec_v[i] += vec_rhs[i];
        }
    }
}

void capacity_current_on_device(model_sorted_token const& sorted_token, NrnThread& nt) {
#if I_MEMBRANE
    if (!nt.tml || nt.tml->index != CAP || nt.end <= 0) {
        return;
    }
    auto* const ml = nt.tml->ml;
    int const count = ml->nodecount;
    if (count <= 0) {
        return;
    }
    neuron::cache::MechanismRange<2, 0> ml_cache{sorted_token, *ml};
    auto* const vec_rhs = nt.node_rhs_storage();
    double* const cm = ml_cache.data_array_ptr<cap_cm_index, 1>();
    double* const i_cap = ml_cache.data_array_ptr<cap_i_cap_index, 1>();
    int* const ni = ml->nodeindices;
    double const cfac = .001 * nt.cj;
    // clang-format off
    nrn_pragma_acc(parallel loop present(vec_rhs [0:nt.end],
                                         cm [0:count],
                                         i_cap [0:count],
                                         ni [0:count]) if (nt.compute_gpu)
                       async(nt.stream_id))
    // clang-format on
    nrn_pragma_omp(target teams distribute parallel for simd if(nt.compute_gpu))
    for (int i = 0; i < count; ++i) {
        i_cap[i] = cfac * cm[i] * vec_rhs[ni[i]];
    }
#else
    (void) sorted_token;
    (void) nt;
#endif
}

void fast_imem_on_device(NrnThread& nt) {
    if (!::nrn_use_fast_imem || nt.end <= 0) {
        return;
    }
    auto* const vec_area = nt.node_area_storage();
    auto* const vec_rhs = nt.node_rhs_storage();
    auto* const vec_sav_d = nt.node_sav_d_storage();
    auto* const vec_sav_rhs = nt.node_sav_rhs_storage();
    if (!vec_sav_d || !vec_sav_rhs) {
        return;
    }
    // clang-format off
    nrn_pragma_acc(parallel loop present(vec_rhs [0:nt.end],
                                         vec_area [0:nt.end],
                                         vec_sav_d [0:nt.end],
                                         vec_sav_rhs [0:nt.end]) if (nt.compute_gpu)
                       async(nt.stream_id))
    // clang-format on
    nrn_pragma_omp(target teams distribute parallel for simd if(nt.compute_gpu))
    for (int i = 0; i < nt.end; ++i) {
        vec_sav_rhs[i] = (vec_sav_d[i] * vec_rhs[i] + vec_sav_rhs[i]) * vec_area[i] * 0.01;
    }
}

}  // namespace

bool post_solve_needs_host_fallback(NrnThread const& nt) {
    extern int use_sparse13;
    if (use_sparse13) {
        return true;
    }
#if EXTRACELLULAR
    (void) nt;
    return true;
#else
    (void) nt;
    return false;
#endif
}

void post_solve_on_device(model_sorted_token const& sorted_token, NrnThread& nt) {
    if (!nt.compute_gpu || nt.end <= 0) {
        return;
    }

    second_order_cur_on_device(sorted_token, nt);
    update_voltage_on_device(nt);
    capacity_current_on_device(sorted_token, nt);
    fast_imem_on_device(nt);

#if defined(NRN_ENABLE_GPU)
    nrn_pragma_acc(wait(nt.stream_id))
#endif
}

}  // namespace neuron::gpu