/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/
#pragma once
#define nrn_pragma_stringify(x) #x
#if defined(CORENEURON_ENABLE_GPU) && defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && defined(_OPENMP)
#define nrn_pragma_acc(x)
#define nrn_pragma_omp(x) _Pragma(nrn_pragma_stringify(omp x))
#include <omp.h>
#elif defined(CORENEURON_ENABLE_GPU) && !defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && \
    defined(_OPENACC)
#define nrn_pragma_acc(x) _Pragma(nrn_pragma_stringify(acc x))
#define nrn_pragma_omp(x)
#include <openacc.h>
#else
#define nrn_pragma_acc(x)
#define nrn_pragma_omp(x)
#endif

#include <cstddef>
#include <stdexcept>
#include <string_view>

namespace coreneuron {
void cnrn_target_copyin_debug(std::string_view file,
                              int line,
                              std::size_t sizeof_T,
                              std::type_info const& typeid_T,
                              void const* h_ptr,
                              std::size_t len,
                              void* d_ptr);
void cnrn_target_delete_debug(std::string_view file,
                              int line,
                              std::size_t sizeof_T,
                              std::type_info const& typeid_T,
                              void const* h_ptr,
                              std::size_t len);
void cnrn_target_deviceptr_debug(std::string_view file,
                                 int line,
                                 std::type_info const& typeid_T,
                                 void const* h_ptr,
                                 void* d_ptr);
void cnrn_target_is_present_debug(std::string_view file,
                                  int line,
                                  std::type_info const& typeid_T,
                                  void const* h_ptr,
                                  void* d_ptr);
void cnrn_target_memcpy_to_device_debug(std::string_view file,
                                        int line,
                                        std::size_t sizeof_T,
                                        std::type_info const& typeid_T,
                                        void const* h_ptr,
                                        std::size_t len,
                                        void* d_ptr);
#if defined(CORENEURON_ENABLE_GPU) && !defined(CORENEURON_UNIFIED_MEMORY) && \
    defined(__NVCOMPILER_MAJOR__) && defined(__NVCOMPILER_MINOR__) &&        \
    (__NVCOMPILER_MAJOR__ <= 22) && (__NVCOMPILER_MINOR__ <= 3)
// Homegrown implementation for buggy NVHPC versions (<=22.3), see
// https://forums.developer.nvidia.com/t/acc-deviceptr-does-not-work-in-openacc-code-dynamically-loaded-from-a-shared-library/211599
#define CORENEURON_ENABLE_PRESENT_TABLE
std::pair<void*, bool> cnrn_target_deviceptr_impl(bool must_be_present_or_null, void const* h_ptr);
void cnrn_target_copyin_update_present_table(void const* h_ptr, void* d_ptr, std::size_t len);
void cnrn_target_delete_update_present_table(void const* h_ptr, std::size_t len);
#endif

template <typename T>
T* cnrn_target_deviceptr_or_present(std::string_view file,
                                    int line,
                                    bool must_be_present_or_null,
                                    const T* h_ptr) {
    T* d_ptr{};
    bool error{false};
#ifdef CORENEURON_ENABLE_PRESENT_TABLE
    auto const d_ptr_and_error = cnrn_target_deviceptr_impl(must_be_present_or_null, h_ptr);
    d_ptr = static_cast<T*>(d_ptr_and_error.first);
    error = d_ptr_and_error.second;
#elif defined(CORENEURON_ENABLE_GPU) && !defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && \
    defined(_OPENACC)
    d_ptr = static_cast<T*>(acc_deviceptr(const_cast<T*>(h_ptr)));
#elif defined(CORENEURON_ENABLE_GPU) && defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && \
    defined(_OPENMP)
    if (must_be_present_or_null || omp_target_is_present(h_ptr, omp_get_default_device())) {
        nrn_pragma_omp(target data use_device_ptr(h_ptr))
        { d_ptr = const_cast<T*>(h_ptr); }
    }
#else
    if (must_be_present_or_null && h_ptr) {
        throw std::runtime_error(
            "cnrn_target_deviceptr() not implemented without OpenACC/OpenMP and gpu build");
    }
#endif
    if (must_be_present_or_null) {
        cnrn_target_deviceptr_debug(file, line, typeid(T), h_ptr, d_ptr);
    } else {
        cnrn_target_is_present_debug(file, line, typeid(T), h_ptr, d_ptr);
    }
    if (error) {
        throw std::runtime_error(
            "cnrn_target_deviceptr() encountered an error, you may want to try setting "
            "CORENEURON_GPU_DEBUG=1");
    }
    return d_ptr;
}

template <typename T>
T* cnrn_target_copyin(std::string_view file, int line, const T* h_ptr, std::size_t len = 1) {
    T* d_ptr{};
#if defined(CORENEURON_ENABLE_GPU) && !defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && \
    defined(_OPENACC)
    d_ptr = static_cast<T*>(acc_copyin(const_cast<T*>(h_ptr), len * sizeof(T)));
#elif defined(CORENEURON_ENABLE_GPU) && defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && \
    defined(_OPENMP)
    nrn_pragma_omp(target enter data map(to : h_ptr[:len]))
    nrn_pragma_omp(target data use_device_ptr(h_ptr))
    { d_ptr = const_cast<T*>(h_ptr); }
#else
    throw std::runtime_error(
        "cnrn_target_copyin() not implemented without OpenACC/OpenMP and gpu build");
#endif
#ifdef CORENEURON_ENABLE_PRESENT_TABLE
    cnrn_target_copyin_update_present_table(h_ptr, d_ptr, len * sizeof(T));
#endif
    cnrn_target_copyin_debug(file, line, sizeof(T), typeid(T), h_ptr, len, d_ptr);
    return d_ptr;
}

template <typename T>
void cnrn_target_delete(std::string_view file, int line, T* h_ptr, std::size_t len = 1) {
    cnrn_target_delete_debug(file, line, sizeof(T), typeid(T), h_ptr, len);
#ifdef CORENEURON_ENABLE_PRESENT_TABLE
    cnrn_target_delete_update_present_table(h_ptr, len * sizeof(T));
#endif
#if defined(CORENEURON_ENABLE_GPU) && !defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && \
    defined(_OPENACC)
    acc_delete(h_ptr, len * sizeof(T));
#elif defined(CORENEURON_ENABLE_GPU) && defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && \
    defined(_OPENMP)
    nrn_pragma_omp(target exit data map(delete : h_ptr[:len]))
#else
    throw std::runtime_error(
        "cnrn_target_delete() not implemented without OpenACC/OpenMP and gpu build");
#endif
}

template <typename T>
void cnrn_target_memcpy_to_device(std::string_view file,
                                  int line,
                                  T* d_ptr,
                                  const T* h_ptr,
                                  std::size_t len = 1) {
    cnrn_target_memcpy_to_device_debug(file, line, sizeof(T), typeid(T), h_ptr, len, d_ptr);
#if defined(CORENEURON_ENABLE_GPU) && !defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && \
    defined(_OPENACC)
    acc_memcpy_to_device(d_ptr, const_cast<T*>(h_ptr), len * sizeof(T));
#elif defined(CORENEURON_ENABLE_GPU) && defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && \
    defined(_OPENMP)
    omp_target_memcpy(d_ptr,
                      const_cast<T*>(h_ptr),
                      len * sizeof(T),
                      0,
                      0,
                      omp_get_default_device(),
                      omp_get_initial_device());
#else
    throw std::runtime_error(
        "cnrn_target_memcpy_to_device() not implemented without OpenACC/OpenMP and gpu build");
#endif
}

template <typename T>
void cnrn_target_update_on_device(std::string_view file,
                                  int line,
                                  const T* h_ptr,
                                  std::size_t len = 1) {
    auto* d_ptr = cnrn_target_deviceptr_or_present(file, line, true, h_ptr);
    cnrn_target_memcpy_to_device(file, line, d_ptr, h_ptr);
}

// Replace with std::source_location once we have C++20
#define cnrn_target_copyin(...) cnrn_target_copyin(__FILE__, __LINE__, __VA_ARGS__)
#define cnrn_target_delete(...) cnrn_target_delete(__FILE__, __LINE__, __VA_ARGS__)
#define cnrn_target_is_present(...) \
    cnrn_target_deviceptr_or_present(__FILE__, __LINE__, false, __VA_ARGS__)
#define cnrn_target_deviceptr(...) \
    cnrn_target_deviceptr_or_present(__FILE__, __LINE__, true, __VA_ARGS__)
#define cnrn_target_memcpy_to_device(...) \
    cnrn_target_memcpy_to_device(__FILE__, __LINE__, __VA_ARGS__)
#define cnrn_target_update_on_device(...) \
    cnrn_target_update_on_device(__FILE__, __LINE__, __VA_ARGS__)

}  // namespace coreneuron
