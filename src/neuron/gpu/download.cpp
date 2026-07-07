#include "neuron/gpu/download.hpp"

#include "multicore.h"
#include "neuron/gpu/config.hpp"
#include "neuron/gpu/sync.hpp"

namespace neuron::gpu {
namespace {

std::size_t g_flush_interval{1};
std::size_t g_step_counter{0};

}  // namespace

std::size_t download_flush_interval() noexcept {
    return g_flush_interval;
}

void set_download_flush_interval(std::size_t interval) noexcept {
    g_flush_interval = interval;
}

void reset_download_step_counter() noexcept {
    g_step_counter = 0;
}

void advance_download_step_counter() noexcept {
    ++g_step_counter;
}

bool should_flush_download() noexcept {
    if (g_flush_interval == 0) {
        return false;
    }
    return (g_step_counter % g_flush_interval) == 0;
}

void batch_download_post_solve(NrnThread& nt) {
    sync_voltages_to_host_after_post_solve(nt);
    sync_fast_imem_to_host_after_post_solve(nt);
}

void batch_download_to_host() {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native()) {
        return;
    }
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        batch_download_post_solve(nrn_threads[ith]);
    }
#endif
}

void batch_upload_to_device() {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native()) {
        return;
    }
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        sync_after_vecplay(nrn_threads[ith]);
    }
#endif
}

void finalize_psolve_download() {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native()) {
        return;
    }
    batch_download_to_host();
    reset_download_step_counter();
#endif
}

}  // namespace neuron::gpu