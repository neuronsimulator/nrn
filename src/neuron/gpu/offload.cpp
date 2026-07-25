/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include "neuron/gpu/offload.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#if defined(NRN_ENABLE_GPU) && defined(NRN_PREFER_OPENMP_OFFLOAD) && defined(_OPENMP) && \
    __has_include(<cuda_runtime_api.h>)
#include <cuda_runtime_api.h>
#endif

#if __has_include(<cxxabi.h>)
#define NRN_GPU_USE_CXXABI
#include <cxxabi.h>
#include <memory>
#endif

#ifdef NRN_ENABLE_PRESENT_TABLE
#include <cassert>
#include <cstddef>
#include <map>
#include <shared_mutex>

namespace {
struct present_table_value {
    std::size_t ref_count{}, size{};
    std::byte* dev_ptr{};
};
std::map<std::byte const*, present_table_value> present_table;
std::shared_mutex present_table_mutex;
}  // namespace
#endif

namespace {
std::string cxx_demangle(const char* mangled) {
#ifdef NRN_GPU_USE_CXXABI
    int status{};
    std::unique_ptr<char, decltype(free)*> demangled{
        abi::__cxa_demangle(mangled, nullptr, nullptr, &status), free};
    return status ? mangled : demangled.get();
#else
    return mangled;
#endif
}

bool target_debug_output_enabled() {
    const char* env = std::getenv("NRN_GPU_DEBUG");
    if (!env) {
        return false;
    }
    std::string env_s{env};
    if (env_s == "1") {
        return true;
    }
    if (env_s == "0") {
        return false;
    }
    throw std::runtime_error("NRN_GPU_DEBUG must be set to 0 or 1 (got " + env_s + ')');
}

bool target_enable_debug{target_debug_output_enabled()};
}  // namespace

namespace neuron::gpu {

void target_copyin_debug(std::string_view file,
                         int line,
                         std::size_t sizeof_T,
                         std::type_info const& typeid_T,
                         void const* h_ptr,
                         std::size_t len,
                         void* d_ptr) {
    if (!target_enable_debug) {
        return;
    }
    std::cerr << file << ':' << line << ": nrn_target_copyin<" << cxx_demangle(typeid_T.name())
              << ">(" << h_ptr << ", " << len << " * " << sizeof_T << " = " << len * sizeof_T
              << ") -> " << d_ptr << std::endl;
}

void target_delete_debug(std::string_view file,
                         int line,
                         std::size_t sizeof_T,
                         std::type_info const& typeid_T,
                         void const* h_ptr,
                         std::size_t len) {
    if (!target_enable_debug) {
        return;
    }
    std::cerr << file << ':' << line << ": nrn_target_delete<" << cxx_demangle(typeid_T.name())
              << ">(" << h_ptr << ", " << len << " * " << sizeof_T << " = " << len * sizeof_T << ')'
              << std::endl;
}

void target_deviceptr_debug(std::string_view file,
                            int line,
                            std::type_info const& typeid_T,
                            void const* h_ptr,
                            void* d_ptr) {
    if (!target_enable_debug) {
        return;
    }
    std::cerr << file << ':' << line << ": nrn_target_deviceptr<" << cxx_demangle(typeid_T.name())
              << ">(" << h_ptr << ") -> " << d_ptr << std::endl;
}

void target_is_present_debug(std::string_view file,
                             int line,
                             std::type_info const& typeid_T,
                             void const* h_ptr,
                             void* d_ptr) {
    if (!target_enable_debug) {
        return;
    }
    std::cerr << file << ':' << line << ": nrn_target_is_present<" << cxx_demangle(typeid_T.name())
              << ">(" << h_ptr << ") -> " << d_ptr << std::endl;
}

void target_memcpy_to_device_debug(std::string_view file,
                                   int line,
                                   std::size_t sizeof_T,
                                   std::type_info const& typeid_T,
                                   void const* h_ptr,
                                   std::size_t len,
                                   void* d_ptr) {
    if (!target_enable_debug) {
        return;
    }
    std::cerr << file << ':' << line << ": nrn_target_memcpy_to_device<"
              << cxx_demangle(typeid_T.name()) << ">(" << d_ptr << ", " << h_ptr << ", " << len
              << " * " << sizeof_T << " = " << len * sizeof_T << ')' << std::endl;
}

#ifdef NRN_ENABLE_PRESENT_TABLE
std::pair<void*, bool> target_deviceptr_impl(bool must_be_present_or_null, void const* h_ptr) {
    if (!h_ptr) {
        return {nullptr, false};
    }
    std::shared_lock _{present_table_mutex};
    if (present_table.empty()) {
        return {nullptr, must_be_present_or_null};
    }
    auto const iter = std::prev(std::upper_bound(
        present_table.begin(), present_table.end(), h_ptr, [](void const* hp, auto const& entry) {
            return hp < entry.first;
        }));
    if (iter == present_table.end()) {
        return {nullptr, must_be_present_or_null};
    }
    std::byte const* const h_byte_ptr{static_cast<std::byte const*>(h_ptr)};
    std::byte const* const h_start_of_block{iter->first};
    std::size_t const block_size{iter->second.size};
    std::byte* const d_start_of_block{iter->second.dev_ptr};
    bool const is_present{h_byte_ptr < h_start_of_block + block_size};
    if (!is_present) {
        return {nullptr, must_be_present_or_null};
    }
    return {d_start_of_block + (h_byte_ptr - h_start_of_block), false};
}

void target_copyin_update_present_table(void const* h_ptr, void* d_ptr, std::size_t len) {
    if (!h_ptr) {
        assert(!d_ptr);
        return;
    }
    std::lock_guard _{present_table_mutex};
    present_table_value new_val{};
    new_val.size = len;
    new_val.ref_count = 1;
    new_val.dev_ptr = static_cast<std::byte*>(d_ptr);
    auto const [iter, inserted] = present_table.emplace(static_cast<std::byte const*>(h_ptr),
                                                        std::move(new_val));
    if (!inserted) {
        assert(iter->second.size == len);
        assert(iter->second.dev_ptr == new_val.dev_ptr);
        ++(iter->second.ref_count);
    }
}

void target_delete_update_present_table(void const* h_ptr, std::size_t len) {
    if (!h_ptr) {
        return;
    }
    std::lock_guard _{present_table_mutex};
    auto const iter = present_table.find(static_cast<std::byte const*>(h_ptr));
    assert(iter != present_table.end());
    assert(iter->second.size == len);
    --(iter->second.ref_count);
    if (iter->second.ref_count == 0) {
        present_table.erase(iter);
    }
}
#endif

int target_get_num_devices() {
#if defined(NRN_ENABLE_GPU) && !defined(NRN_PREFER_OPENMP_OFFLOAD) && defined(_OPENACC)
    acc_device_t const device_type = acc_device_nvidia;
    return acc_get_num_devices(device_type);
#elif defined(NRN_ENABLE_GPU) && defined(NRN_PREFER_OPENMP_OFFLOAD) && defined(_OPENMP)
    return omp_get_num_devices();
#else
    return 0;
#endif
}

void target_set_default_device(int device_num) {
#if defined(NRN_ENABLE_GPU) && !defined(NRN_PREFER_OPENMP_OFFLOAD) && defined(_OPENACC)
    acc_set_device_num(device_num, acc_device_nvidia);
#elif defined(NRN_ENABLE_GPU) && defined(NRN_PREFER_OPENMP_OFFLOAD) && defined(_OPENMP)
    omp_set_default_device(device_num);
#if __has_include(<cuda_runtime_api.h>)
    auto const cuda_code = cudaSetDevice(device_num);
    if (cuda_code != cudaSuccess) {
        throw std::runtime_error("neuron::gpu::target_set_default_device: cudaSetDevice failed");
    }
#endif
#else
    (void) device_num;
#endif
}

int target_get_default_device() {
#if defined(NRN_ENABLE_GPU) && !defined(NRN_PREFER_OPENMP_OFFLOAD) && defined(_OPENACC)
    return acc_get_device_num(acc_device_nvidia);
#elif defined(NRN_ENABLE_GPU) && defined(NRN_PREFER_OPENMP_OFFLOAD) && defined(_OPENMP)
    return omp_get_default_device();
#else
    return -1;
#endif
}

}  // namespace neuron::gpu