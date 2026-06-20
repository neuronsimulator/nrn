/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/
#pragma once

// NEURON-native OpenACC/OpenMP offload helpers (forked from coreneuron/utils/offload.hpp).
// CoreNEURON continues to use coreneuron::cnrn_target_* during the Phase A transition.

#define nrn_gpu_pragma_stringify(x) #x
#if defined(NRN_ENABLE_GPU) && defined(NRN_PREFER_OPENMP_OFFLOAD) && defined(_OPENMP)
#define nrn_gpu_pragma_acc(x)
#define nrn_gpu_pragma_omp(x) _Pragma(nrn_gpu_pragma_stringify(omp x))
#include <omp.h>
#elif defined(NRN_ENABLE_GPU) && !defined(NRN_PREFER_OPENMP_OFFLOAD) && defined(_OPENACC)
#define nrn_gpu_pragma_acc(x) _Pragma(nrn_gpu_pragma_stringify(acc x))
#define nrn_gpu_pragma_omp(x)
#include <openacc.h>
#else
#define nrn_gpu_pragma_acc(x)
#define nrn_gpu_pragma_omp(x)
#endif

#include <cstddef>
#include <stdexcept>
#include <string_view>
#include <typeinfo>
#include <utility>

namespace neuron::gpu {

void target_copyin_debug(std::string_view file,
                         int line,
                         std::size_t sizeof_T,
                         std::type_info const& typeid_T,
                         void const* h_ptr,
                         std::size_t len,
                         void* d_ptr);
void target_delete_debug(std::string_view file,
                         int line,
                         std::size_t sizeof_T,
                         std::type_info const& typeid_T,
                         void const* h_ptr,
                         std::size_t len);
void target_deviceptr_debug(std::string_view file,
                            int line,
                            std::type_info const& typeid_T,
                            void const* h_ptr,
                            void* d_ptr);
void target_is_present_debug(std::string_view file,
                               int line,
                               std::type_info const& typeid_T,
                               void const* h_ptr,
                               void* d_ptr);
void target_memcpy_to_device_debug(std::string_view file,
                                   int line,
                                   std::size_t sizeof_T,
                                   std::type_info const& typeid_T,
                                   void const* h_ptr,
                                   std::size_t len,
                                   void* d_ptr);

#if defined(NRN_ENABLE_GPU) && !defined(NRN_UNIFIED_MEMORY) && defined(__NVCOMPILER_MAJOR__) && \
    defined(__NVCOMPILER_MINOR__) && (__NVCOMPILER_MAJOR__ <= 22) && (__NVCOMPILER_MINOR__ <= 3)
// Homegrown implementation for buggy NVHPC versions (<=22.3); required for dynamically loaded
// mechanisms. See NVIDIA forum thread 211599.
#define NRN_ENABLE_PRESENT_TABLE
std::pair<void*, bool> target_deviceptr_impl(bool must_be_present_or_null, void const* h_ptr);
void target_copyin_update_present_table(void const* h_ptr, void* d_ptr, std::size_t len);
void target_delete_update_present_table(void const* h_ptr, std::size_t len);
#endif

template <typename T>
T* target_deviceptr_or_present(std::string_view file,
                               int line,
                               bool must_be_present_or_null,
                               const T* h_ptr) {
    T* d_ptr{};
    bool error{false};
#ifdef NRN_ENABLE_PRESENT_TABLE
    auto const d_ptr_and_error = target_deviceptr_impl(must_be_present_or_null, h_ptr);
    d_ptr = static_cast<T*>(d_ptr_and_error.first);
    error = d_ptr_and_error.second;
#elif defined(NRN_ENABLE_GPU) && !defined(NRN_PREFER_OPENMP_OFFLOAD) && defined(_OPENACC)
    d_ptr = static_cast<T*>(acc_deviceptr(const_cast<T*>(h_ptr)));
#elif defined(NRN_ENABLE_GPU) && defined(NRN_PREFER_OPENMP_OFFLOAD) && defined(_OPENMP)
    if (must_be_present_or_null || omp_target_is_present(h_ptr, omp_get_default_device())) {
        nrn_gpu_pragma_omp(target data use_device_ptr(h_ptr))
        { d_ptr = const_cast<T*>(h_ptr); }
    }
#else
    if (must_be_present_or_null && h_ptr) {
        throw std::runtime_error(
            "neuron::gpu::target_deviceptr() not implemented without OpenACC/OpenMP and "
            "NRN_ENABLE_GPU");
    }
#endif
    if (must_be_present_or_null) {
        target_deviceptr_debug(file, line, typeid(T), h_ptr, d_ptr);
    } else {
        target_is_present_debug(file, line, typeid(T), h_ptr, d_ptr);
    }
    if (error) {
        throw std::runtime_error(
            "neuron::gpu::target_deviceptr() encountered an error; try NRN_GPU_DEBUG=1");
    }
    return d_ptr;
}

template <typename T>
T* target_copyin(std::string_view file, int line, const T* h_ptr, std::size_t len = 1) {
    T* d_ptr{};
#if defined(NRN_ENABLE_GPU) && !defined(NRN_PREFER_OPENMP_OFFLOAD) && defined(_OPENACC)
    d_ptr = static_cast<T*>(acc_copyin(const_cast<T*>(h_ptr), len * sizeof(T)));
#elif defined(NRN_ENABLE_GPU) && defined(NRN_PREFER_OPENMP_OFFLOAD) && defined(_OPENMP)
    nrn_gpu_pragma_omp(target enter data map(to : h_ptr[:len]))
    nrn_gpu_pragma_omp(target data use_device_ptr(h_ptr))
    { d_ptr = const_cast<T*>(h_ptr); }
#else
    throw std::runtime_error(
        "neuron::gpu::target_copyin() not implemented without OpenACC/OpenMP and NRN_ENABLE_GPU");
#endif
#ifdef NRN_ENABLE_PRESENT_TABLE
    target_copyin_update_present_table(h_ptr, d_ptr, len * sizeof(T));
#endif
    target_copyin_debug(file, line, sizeof(T), typeid(T), h_ptr, len, d_ptr);
    return d_ptr;
}

template <typename T>
void target_delete(std::string_view file, int line, T* h_ptr, std::size_t len = 1) {
    target_delete_debug(file, line, sizeof(T), typeid(T), h_ptr, len);
#ifdef NRN_ENABLE_PRESENT_TABLE
    target_delete_update_present_table(h_ptr, len * sizeof(T));
#endif
#if defined(NRN_ENABLE_GPU) && !defined(NRN_PREFER_OPENMP_OFFLOAD) && defined(_OPENACC)
    acc_delete(h_ptr, len * sizeof(T));
#elif defined(NRN_ENABLE_GPU) && defined(NRN_PREFER_OPENMP_OFFLOAD) && defined(_OPENMP)
    nrn_gpu_pragma_omp(target exit data map(delete : h_ptr[:len]))
#else
    throw std::runtime_error(
        "neuron::gpu::target_delete() not implemented without OpenACC/OpenMP and NRN_ENABLE_GPU");
#endif
}

template <typename T>
void target_memcpy_to_device(std::string_view file,
                             int line,
                             T* d_ptr,
                             const T* h_ptr,
                             std::size_t len = 1) {
    target_memcpy_to_device_debug(file, line, sizeof(T), typeid(T), h_ptr, len, d_ptr);
#if defined(NRN_ENABLE_GPU) && !defined(NRN_PREFER_OPENMP_OFFLOAD) && defined(_OPENACC)
    acc_memcpy_to_device(d_ptr, const_cast<T*>(h_ptr), len * sizeof(T));
#elif defined(NRN_ENABLE_GPU) && defined(NRN_PREFER_OPENMP_OFFLOAD) && defined(_OPENMP)
    omp_target_memcpy(d_ptr,
                      const_cast<T*>(h_ptr),
                      len * sizeof(T),
                      0,
                      0,
                      omp_get_default_device(),
                      omp_get_initial_device());
#else
    throw std::runtime_error(
        "neuron::gpu::target_memcpy_to_device() not implemented without OpenACC/OpenMP and "
        "NRN_ENABLE_GPU");
#endif
}

template <typename T>
void target_update_on_device(std::string_view file, int line, const T* h_ptr, std::size_t len = 1) {
    auto* d_ptr = target_deviceptr_or_present(file, line, true, h_ptr);
    target_memcpy_to_device(file, line, d_ptr, h_ptr, len);
}

}  // namespace neuron::gpu

#define nrn_target_copyin(...) neuron::gpu::target_copyin(__FILE__, __LINE__, __VA_ARGS__)
#define nrn_target_delete(...) neuron::gpu::target_delete(__FILE__, __LINE__, __VA_ARGS__)
#define nrn_target_is_present(...) \
    neuron::gpu::target_deviceptr_or_present(__FILE__, __LINE__, false, __VA_ARGS__)
#define nrn_target_deviceptr(...) \
    neuron::gpu::target_deviceptr_or_present(__FILE__, __LINE__, true, __VA_ARGS__)
#define nrn_target_memcpy_to_device(...) \
    neuron::gpu::target_memcpy_to_device(__FILE__, __LINE__, __VA_ARGS__)
#define nrn_target_update_on_device(...) \
    neuron::gpu::target_update_on_device(__FILE__, __LINE__, __VA_ARGS__)