/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include <queue>
#include <utility>

#include "coreneuron/apps/corenrn_parameters.hpp"
#include "coreneuron/gpu/nrn_acc_manager.hpp"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/network/netcon.hpp"
#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/utils/vrecitem.h"
#include "coreneuron/utils/profile/profiler_interface.h"
#include "coreneuron/permute/cellorder.hpp"
#include "coreneuron/permute/data_layout.hpp"
#include "coreneuron/sim/scopmath/newton_struct.h"
#include "coreneuron/coreneuron.hpp"
#include "coreneuron/utils/nrnoc_aux.hpp"
#include "coreneuron/mpi/nrnmpidec.h"
#include "coreneuron/utils/utils.hpp"

#ifdef CRAYPAT
#include <pat_api.h>
#endif

#if defined(CORENEURON_ENABLE_GPU) && defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && defined(_OPENMP)
#include <cuda_runtime_api.h>
#endif

#if __has_include(<cxxabi.h>)
#define USE_CXXABI
#include <cxxabi.h>
#include <memory>
#include <string>
#endif

#ifdef CORENEURON_ENABLE_PRESENT_TABLE
#include <cassert>
#include <cstddef>
#include <iostream>
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
/** @brief Try to demangle a type name, return the mangled name on failure.
 */
std::string cxx_demangle(const char* mangled) {
#ifdef USE_CXXABI
    int status{};
    // Note that the third argument to abi::__cxa_demangle returns the length of
    // the allocated buffer, which may be larger than strlen(demangled) + 1.
    std::unique_ptr<char, decltype(free)*> demangled{
        abi::__cxa_demangle(mangled, nullptr, nullptr, &status), free};
    return status ? mangled : demangled.get();
#else
    return mangled;
#endif
}
bool cnrn_target_debug_output_enabled() {
    const char* env = std::getenv("CORENEURON_GPU_DEBUG");
    if (!env) {
        return false;
    }
    std::string env_s{env};
    if (env_s == "1") {
        return true;
    } else if (env_s == "0") {
        return false;
    } else {
        throw std::runtime_error("CORENEURON_GPU_DEBUG must be set to 0 or 1 (got " + env_s + ")");
    }
}
bool cnrn_target_enable_debug{cnrn_target_debug_output_enabled()};
}  // namespace

namespace coreneuron {
extern InterleaveInfo* interleave_info;
void nrn_ion_global_map_copyto_device();
void nrn_ion_global_map_delete_from_device();
void nrn_VecPlay_copyto_device(NrnThread* nt, void** d_vecplay);
void nrn_VecPlay_delete_from_device(NrnThread* nt);

void cnrn_target_copyin_debug(std::string_view file,
                              int line,
                              std::size_t sizeof_T,
                              std::type_info const& typeid_T,
                              void const* h_ptr,
                              std::size_t len,
                              void* d_ptr) {
    if (!cnrn_target_enable_debug) {
        return;
    }
    std::cerr << file << ':' << line << ": cnrn_target_copyin<" << cxx_demangle(typeid_T.name())
              << ">(" << h_ptr << ", " << len << " * " << sizeof_T << " = " << len * sizeof_T
              << ") -> " << d_ptr << std::endl;
}
void cnrn_target_delete_debug(std::string_view file,
                              int line,
                              std::size_t sizeof_T,
                              std::type_info const& typeid_T,
                              void const* h_ptr,
                              std::size_t len) {
    if (!cnrn_target_enable_debug) {
        return;
    }
    std::cerr << file << ':' << line << ": cnrn_target_delete<" << cxx_demangle(typeid_T.name())
              << ">(" << h_ptr << ", " << len << " * " << sizeof_T << " = " << len * sizeof_T << ')'
              << std::endl;
}
void cnrn_target_deviceptr_debug(std::string_view file,
                                 int line,
                                 std::type_info const& typeid_T,
                                 void const* h_ptr,
                                 void* d_ptr) {
    if (!cnrn_target_enable_debug) {
        return;
    }
    std::cerr << file << ':' << line << ": cnrn_target_deviceptr<" << cxx_demangle(typeid_T.name())
              << ">(" << h_ptr << ") -> " << d_ptr << std::endl;
}
void cnrn_target_is_present_debug(std::string_view file,
                                  int line,
                                  std::type_info const& typeid_T,
                                  void const* h_ptr,
                                  void* d_ptr) {
    if (!cnrn_target_enable_debug) {
        return;
    }
    std::cerr << file << ':' << line << ": cnrn_target_is_present<" << cxx_demangle(typeid_T.name())
              << ">(" << h_ptr << ") -> " << d_ptr << std::endl;
}
void cnrn_target_memcpy_to_device_debug(std::string_view file,
                                        int line,
                                        std::size_t sizeof_T,
                                        std::type_info const& typeid_T,
                                        void const* h_ptr,
                                        std::size_t len,
                                        void* d_ptr) {
    if (!cnrn_target_enable_debug) {
        return;
    }
    std::cerr << file << ':' << line << ": cnrn_target_memcpy_to_device<"
              << cxx_demangle(typeid_T.name()) << ">(" << d_ptr << ", " << h_ptr << ", " << len
              << " * " << sizeof_T << " = " << len * sizeof_T << ')' << std::endl;
}

#ifdef CORENEURON_ENABLE_PRESENT_TABLE
std::pair<void*, bool> cnrn_target_deviceptr_impl(bool must_be_present_or_null, void const* h_ptr) {
    if (!h_ptr) {
        return {nullptr, false};
    }
    // Concurrent calls to this method are safe, but they must be serialised
    // w.r.t. calls to the cnrn_target_*_update_present_table methods.
    std::shared_lock _{present_table_mutex};
    if (present_table.empty()) {
        return {nullptr, must_be_present_or_null};
    }
    // prev(first iterator greater than h_ptr or last if not found) gives the first iterator less
    // than or equal to h_ptr
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

void cnrn_target_copyin_update_present_table(void const* h_ptr, void* d_ptr, std::size_t len) {
    if (!h_ptr) {
        assert(!d_ptr);
        return;
    }
    std::lock_guard _{present_table_mutex};
    // TODO include more pedantic overlap checking?
    present_table_value new_val{};
    new_val.size = len;
    new_val.ref_count = 1;
    new_val.dev_ptr = static_cast<std::byte*>(d_ptr);
    auto const [iter, inserted] = present_table.emplace(static_cast<std::byte const*>(h_ptr),
                                                        std::move(new_val));
    if (!inserted) {
        // Insertion didn't occur because h_ptr was already in the present table
        assert(iter->second.size == len);
        assert(iter->second.dev_ptr == new_val.dev_ptr);
        ++(iter->second.ref_count);
    }
}
void cnrn_target_delete_update_present_table(void const* h_ptr, std::size_t len) {
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

int cnrn_target_get_num_devices() {
#if defined(CORENEURON_ENABLE_GPU) && !defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && \
    defined(_OPENACC)
    // choose nvidia GPU by default
    acc_device_t device_type = acc_device_nvidia;
    // check how many gpu devices available per node
    return acc_get_num_devices(device_type);
#elif defined(CORENEURON_ENABLE_GPU) && defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && \
    defined(_OPENMP)
    return omp_get_num_devices();
#else
    throw std::runtime_error(
        "cnrn_target_get_num_devices() not implemented without OpenACC/OpenMP and gpu build");
#endif
}

void cnrn_target_set_default_device(int device_num) {
#if defined(CORENEURON_ENABLE_GPU) && !defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && \
    defined(_OPENACC)
    acc_set_device_num(device_num, acc_device_nvidia);
#elif defined(CORENEURON_ENABLE_GPU) && defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && \
    defined(_OPENMP)
    omp_set_default_device(device_num);
    // It seems that with NVHPC 21.9 then only setting the default OpenMP device
    // is not enough: there were errors on some nodes when not-the-0th GPU was
    // used. These seemed to be related to the NMODL instance structs, which are
    // allocated using cudaMallocManaged.
    auto const cuda_code = cudaSetDevice(device_num);
    assert(cuda_code == cudaSuccess);
#else
    throw std::runtime_error(
        "cnrn_target_set_default_device() not implemented without OpenACC/OpenMP and gpu build");
#endif
}

#ifdef CORENEURON_ENABLE_GPU
#ifndef CORENEURON_UNIFIED_MEMORY
static Memb_list* copy_ml_to_device(const Memb_list* ml, int type) {
    // As we never run code for artificial cell inside GPU we don't copy it.
    int is_art = corenrn.get_is_artificial()[type];
    if (is_art) {
        return nullptr;
    }

    auto d_ml = cnrn_target_copyin(ml);

    if (ml->global_variables) {
        assert(ml->global_variables_size);
        void* d_inst = cnrn_target_copyin(static_cast<std::byte*>(ml->global_variables),
                                          ml->global_variables_size);
        cnrn_target_memcpy_to_device(&(d_ml->global_variables), &d_inst);
    }


    int n = ml->nodecount;
    int szp = corenrn.get_prop_param_size()[type];
    int szdp = corenrn.get_prop_dparam_size()[type];

    double* dptr = cnrn_target_deviceptr(ml->data);
    cnrn_target_memcpy_to_device(&(d_ml->data), &(dptr));


    int* d_nodeindices = cnrn_target_copyin(ml->nodeindices, n);
    cnrn_target_memcpy_to_device(&(d_ml->nodeindices), &d_nodeindices);

    if (szdp) {
        int pcnt = nrn_soa_padded_size(n, SOA_LAYOUT) * szdp;
        int* d_pdata = cnrn_target_copyin(ml->pdata, pcnt);
        cnrn_target_memcpy_to_device(&(d_ml->pdata), &d_pdata);
    }

    int ts = corenrn.get_memb_funcs()[type].thread_size_;
    if (ts) {
        ThreadDatum* td = cnrn_target_copyin(ml->_thread, ts);
        cnrn_target_memcpy_to_device(&(d_ml->_thread), &td);
    }

    // net_receive buffer associated with mechanism
    NetReceiveBuffer_t* nrb = ml->_net_receive_buffer;

    // if net receive buffer exist for mechanism
    if (nrb) {
        NetReceiveBuffer_t* d_nrb = cnrn_target_copyin(nrb);
        cnrn_target_memcpy_to_device(&(d_ml->_net_receive_buffer), &d_nrb);

        int* d_pnt_index = cnrn_target_copyin(nrb->_pnt_index, nrb->_size);
        cnrn_target_memcpy_to_device(&(d_nrb->_pnt_index), &d_pnt_index);

        int* d_weight_index = cnrn_target_copyin(nrb->_weight_index, nrb->_size);
        cnrn_target_memcpy_to_device(&(d_nrb->_weight_index), &d_weight_index);

        double* d_nrb_t = cnrn_target_copyin(nrb->_nrb_t, nrb->_size);
        cnrn_target_memcpy_to_device(&(d_nrb->_nrb_t), &d_nrb_t);

        double* d_nrb_flag = cnrn_target_copyin(nrb->_nrb_flag, nrb->_size);
        cnrn_target_memcpy_to_device(&(d_nrb->_nrb_flag), &d_nrb_flag);

        int* d_displ = cnrn_target_copyin(nrb->_displ, nrb->_size + 1);
        cnrn_target_memcpy_to_device(&(d_nrb->_displ), &d_displ);

        int* d_nrb_index = cnrn_target_copyin(nrb->_nrb_index, nrb->_size);
        cnrn_target_memcpy_to_device(&(d_nrb->_nrb_index), &d_nrb_index);
    }

    /* copy NetSendBuffer_t on to GPU */
    NetSendBuffer_t* nsb = ml->_net_send_buffer;

    if (nsb) {
        NetSendBuffer_t* d_nsb;
        int* d_iptr;
        double* d_dptr;

        d_nsb = cnrn_target_copyin(nsb);
        cnrn_target_memcpy_to_device(&(d_ml->_net_send_buffer), &d_nsb);

        d_iptr = cnrn_target_copyin(nsb->_sendtype, nsb->_size);
        cnrn_target_memcpy_to_device(&(d_nsb->_sendtype), &d_iptr);

        d_iptr = cnrn_target_copyin(nsb->_vdata_index, nsb->_size);
        cnrn_target_memcpy_to_device(&(d_nsb->_vdata_index), &d_iptr);

        d_iptr = cnrn_target_copyin(nsb->_pnt_index, nsb->_size);
        cnrn_target_memcpy_to_device(&(d_nsb->_pnt_index), &d_iptr);

        d_iptr = cnrn_target_copyin(nsb->_weight_index, nsb->_size);
        cnrn_target_memcpy_to_device(&(d_nsb->_weight_index), &d_iptr);

        d_dptr = cnrn_target_copyin(nsb->_nsb_t, nsb->_size);
        cnrn_target_memcpy_to_device(&(d_nsb->_nsb_t), &d_dptr);

        d_dptr = cnrn_target_copyin(nsb->_nsb_flag, nsb->_size);
        cnrn_target_memcpy_to_device(&(d_nsb->_nsb_flag), &d_dptr);
    }

    return d_ml;
}
#endif

static void update_ml_on_host(const Memb_list* ml, int type) {
    int is_art = corenrn.get_is_artificial()[type];
    if (is_art) {
        // Artificial mechanisms such as PatternStim and IntervalFire
        // are not copied onto the GPU. They should not, therefore, be
        // updated from the GPU.
        return;
    }

    int n = ml->nodecount;
    int szp = corenrn.get_prop_param_size()[type];
    int szdp = corenrn.get_prop_dparam_size()[type];

    int pcnt = nrn_soa_padded_size(n, SOA_LAYOUT) * szp;

    nrn_pragma_acc(update self(ml->data[:pcnt], ml->nodeindices[:n]))
    nrn_pragma_omp(target update from(ml->data[:pcnt], ml->nodeindices[:n]))

    int dpcnt = nrn_soa_padded_size(n, SOA_LAYOUT) * szdp;
    nrn_pragma_acc(update self(ml->pdata[:dpcnt]) if (szdp))
    nrn_pragma_omp(target update from(ml->pdata[:dpcnt]) if (szdp))

    auto nrb = ml->_net_receive_buffer;

    // clang-format off
    nrn_pragma_acc(update self(nrb->_cnt,
                               nrb->_size,
                               nrb->_pnt_offset,
                               nrb->_displ_cnt,
                               nrb->_pnt_index[:nrb->_size],
                               nrb->_weight_index[:nrb->_size],
                               nrb->_displ[:nrb->_size + 1],
                               nrb->_nrb_index[:nrb->_size])
                          if (nrb != nullptr))
    nrn_pragma_omp(target update from(nrb->_cnt,
                                      nrb->_size,
                                      nrb->_pnt_offset,
                                      nrb->_displ_cnt,
                                      nrb->_pnt_index[:nrb->_size],
                                      nrb->_weight_index[:nrb->_size],
                                      nrb->_displ[:nrb->_size + 1],
                                      nrb->_nrb_index[:nrb->_size])
                                 if (nrb != nullptr))
    // clang-format on
}

static void delete_ml_from_device(Memb_list* ml, int type) {
    int is_art = corenrn.get_is_artificial()[type];
    if (is_art) {
        return;
    }
    // Cleanup the net send buffer if it exists
    {
        NetSendBuffer_t* nsb{ml->_net_send_buffer};
        if (nsb) {
            cnrn_target_delete(nsb->_nsb_flag, nsb->_size);
            cnrn_target_delete(nsb->_nsb_t, nsb->_size);
            cnrn_target_delete(nsb->_weight_index, nsb->_size);
            cnrn_target_delete(nsb->_pnt_index, nsb->_size);
            cnrn_target_delete(nsb->_vdata_index, nsb->_size);
            cnrn_target_delete(nsb->_sendtype, nsb->_size);
            cnrn_target_delete(nsb);
        }
    }
    // Cleanup the net receive buffer if it exists.
    {
        NetReceiveBuffer_t* nrb{ml->_net_receive_buffer};
        if (nrb) {
            cnrn_target_delete(nrb->_nrb_index, nrb->_size);
            cnrn_target_delete(nrb->_displ, nrb->_size + 1);
            cnrn_target_delete(nrb->_nrb_flag, nrb->_size);
            cnrn_target_delete(nrb->_nrb_t, nrb->_size);
            cnrn_target_delete(nrb->_weight_index, nrb->_size);
            cnrn_target_delete(nrb->_pnt_index, nrb->_size);
            cnrn_target_delete(nrb);
        }
    }
    int n = ml->nodecount;
    int szdp = corenrn.get_prop_dparam_size()[type];
    int ts = corenrn.get_memb_funcs()[type].thread_size_;
    if (ts) {
        cnrn_target_delete(ml->_thread, ts);
    }
    if (szdp) {
        int pcnt = nrn_soa_padded_size(n, SOA_LAYOUT) * szdp;
        cnrn_target_delete(ml->pdata, pcnt);
    }
    cnrn_target_delete(ml->nodeindices, n);

    if (ml->global_variables) {
        assert(ml->global_variables_size);
        cnrn_target_delete(static_cast<std::byte*>(ml->global_variables),
                           ml->global_variables_size);
    }

    cnrn_target_delete(ml);
}

#endif

/* note: threads here are corresponding to global nrn_threads array */
void setup_nrnthreads_on_device(NrnThread* threads, int nthreads) {
#ifdef CORENEURON_ENABLE_GPU
    // initialize NrnThreads for gpu execution
    // empty thread or only artificial cells should be on cpu
    for (int i = 0; i < nthreads; i++) {
        NrnThread* nt = threads + i;
        nt->compute_gpu = (nt->end > 0) ? 1 : 0;
        nt->_dt = dt;
    }

    nrn_ion_global_map_copyto_device();

#ifdef CORENEURON_UNIFIED_MEMORY
    for (int i = 0; i < nthreads; i++) {
        NrnThread* nt = threads + i;  // NrnThread on host

        if (nt->n_presyn) {
            PreSyn* d_presyns = cnrn_target_copyin(nt->presyns, nt->n_presyn);
        }

        if (nt->n_vecplay) {
            /* copy VecPlayContinuous instances */
            /** just empty containers */
            void** d_vecplay = cnrn_target_copyin(nt->_vecplay, nt->n_vecplay);
            // note: we are using unified memory for NrnThread. Once VecPlay is copied to gpu,
            // we dont want to update nt->vecplay because it will also set gpu pointer of vecplay
            // inside nt on cpu (due to unified memory).

            nrn_VecPlay_copyto_device(nt, d_vecplay);
        }

        if (!nt->_permute && nt->end > 0) {
            printf("\n WARNING: NrnThread %d not permuted, error for linear algebra?", i);
        }
    }

#else
    /* -- copy NrnThread to device. this needs to be contigious vector because offset is used to
     * find
     * corresponding NrnThread using Point_process in NET_RECEIVE block
     */
    NrnThread* d_threads = cnrn_target_copyin(threads, nthreads);

    if (interleave_info == nullptr) {
        printf("\n Warning: No permutation data? Required for linear algebra!");
    }

    /* pointers for data struct on device, starting with d_ */

    for (int i = 0; i < nthreads; i++) {
        NrnThread* nt = threads + i;      // NrnThread on host
        NrnThread* d_nt = d_threads + i;  // NrnThread on device
        if (!nt->compute_gpu) {
            continue;
        }
        double* d__data;  // nrn_threads->_data on device

        /* -- copy _data to device -- */

        /*copy all double data for thread */
        d__data = cnrn_target_copyin(nt->_data, nt->_ndata);


        /* Here is the example of using OpenACC data enter/exit
         * Remember that we are not allowed to use nt->_data but we have to use:
         *      double *dtmp = nt->_data;  // now use dtmp!
                #pragma acc enter data copyin(dtmp[0:nt->_ndata]) async(nt->stream_id)
                #pragma acc wait(nt->stream_id)
         */

        /*update d_nt._data to point to device copy */
        cnrn_target_memcpy_to_device(&(d_nt->_data), &d__data);

        /* -- setup rhs, d, a, b, v, node_aread to point to device copy -- */
        double* dptr;

        /* for padding, we have to recompute ne */
        int ne = nrn_soa_padded_size(nt->end, 0);

        dptr = d__data + 0 * ne;
        cnrn_target_memcpy_to_device(&(d_nt->_actual_rhs), &(dptr));

        dptr = d__data + 1 * ne;
        cnrn_target_memcpy_to_device(&(d_nt->_actual_d), &(dptr));

        dptr = d__data + 2 * ne;
        cnrn_target_memcpy_to_device(&(d_nt->_actual_a), &(dptr));

        dptr = d__data + 3 * ne;
        cnrn_target_memcpy_to_device(&(d_nt->_actual_b), &(dptr));

        dptr = d__data + 4 * ne;
        cnrn_target_memcpy_to_device(&(d_nt->_actual_v), &(dptr));

        dptr = d__data + 5 * ne;
        cnrn_target_memcpy_to_device(&(d_nt->_actual_area), &(dptr));

        if (nt->_actual_diam) {
            dptr = d__data + 6 * ne;
            cnrn_target_memcpy_to_device(&(d_nt->_actual_diam), &(dptr));
        }

        int* d_v_parent_index = cnrn_target_copyin(nt->_v_parent_index, nt->end);
        cnrn_target_memcpy_to_device(&(d_nt->_v_parent_index), &(d_v_parent_index));

        /* nt._ml_list is used in NET_RECEIVE block and should have valid membrane list id*/
        Memb_list** d_ml_list = cnrn_target_copyin(nt->_ml_list, corenrn.get_memb_funcs().size());
        cnrn_target_memcpy_to_device(&(d_nt->_ml_list), &(d_ml_list));

        /* -- copy NrnThreadMembList list ml to device -- */

        NrnThreadMembList* d_last_tml;

        bool first_tml = true;

        for (auto tml = nt->tml; tml; tml = tml->next) {
            /*copy tml to device*/
            /*QUESTIONS: does tml will point to nullptr as in host ? : I assume so!*/
            auto d_tml = cnrn_target_copyin(tml);

            /*first tml is pointed by nt */
            if (first_tml) {
                cnrn_target_memcpy_to_device(&(d_nt->tml), &d_tml);
                first_tml = false;
            } else {
                /*rest of tml forms linked list */
                cnrn_target_memcpy_to_device(&(d_last_tml->next), &d_tml);
            }

            // book keeping for linked-list
            d_last_tml = d_tml;

            /* now for every tml, there is a ml. copy that and setup pointer */
            Memb_list* d_ml = copy_ml_to_device(tml->ml, tml->index);
            cnrn_target_memcpy_to_device(&(d_tml->ml), &d_ml);
            /* setup nt._ml_list */
            cnrn_target_memcpy_to_device(&(d_ml_list[tml->index]), &d_ml);
        }

        if (nt->shadow_rhs_cnt) {
            double* d_shadow_ptr;

            int pcnt = nrn_soa_padded_size(nt->shadow_rhs_cnt, 0);

            /* copy shadow_rhs to device and fix-up the pointer */
            d_shadow_ptr = cnrn_target_copyin(nt->_shadow_rhs, pcnt);
            cnrn_target_memcpy_to_device(&(d_nt->_shadow_rhs), &d_shadow_ptr);

            /* copy shadow_d to device and fix-up the pointer */
            d_shadow_ptr = cnrn_target_copyin(nt->_shadow_d, pcnt);
            cnrn_target_memcpy_to_device(&(d_nt->_shadow_d), &d_shadow_ptr);
        }

        /* Fast membrane current calculation struct */
        if (nt->nrn_fast_imem) {
            NrnFastImem* d_fast_imem = cnrn_target_copyin(nt->nrn_fast_imem);
            cnrn_target_memcpy_to_device(&(d_nt->nrn_fast_imem), &d_fast_imem);
            {
                double* d_ptr = cnrn_target_copyin(nt->nrn_fast_imem->nrn_sav_rhs, nt->end);
                cnrn_target_memcpy_to_device(&(d_fast_imem->nrn_sav_rhs), &d_ptr);
            }
            {
                double* d_ptr = cnrn_target_copyin(nt->nrn_fast_imem->nrn_sav_d, nt->end);
                cnrn_target_memcpy_to_device(&(d_fast_imem->nrn_sav_d), &d_ptr);
            }
        }

        if (nt->n_pntproc) {
            /* copy Point_processes array and fix the pointer to execute net_receive blocks on GPU
             */
            Point_process* pntptr = cnrn_target_copyin(nt->pntprocs, nt->n_pntproc);
            cnrn_target_memcpy_to_device(&(d_nt->pntprocs), &pntptr);
        }

        if (nt->n_weight) {
            /* copy weight vector used in NET_RECEIVE which is pointed by netcon.weight */
            double* d_weights = cnrn_target_copyin(nt->weights, nt->n_weight);
            cnrn_target_memcpy_to_device(&(d_nt->weights), &d_weights);
        }

        if (nt->_nvdata) {
            /* copy vdata which is setup in bbcore_read. This contains cuda allocated
             * nrnran123_State * */
            void** d_vdata = cnrn_target_copyin(nt->_vdata, nt->_nvdata);
            cnrn_target_memcpy_to_device(&(d_nt->_vdata), &d_vdata);
        }

        if (nt->n_presyn) {
            /* copy presyn vector used for spike exchange, note we have added new PreSynHelper due
             * to issue
             * while updating PreSyn objects which has virtual base class. May be this is issue due
             * to
             * VTable and alignment */
            PreSynHelper* d_presyns_helper = cnrn_target_copyin(nt->presyns_helper, nt->n_presyn);
            cnrn_target_memcpy_to_device(&(d_nt->presyns_helper), &d_presyns_helper);
            PreSyn* d_presyns = cnrn_target_copyin(nt->presyns, nt->n_presyn);
            cnrn_target_memcpy_to_device(&(d_nt->presyns), &d_presyns);
        }

        if (nt->_net_send_buffer_size) {
            /* copy send_receive buffer */
            int* d_net_send_buffer = cnrn_target_copyin(nt->_net_send_buffer,
                                                        nt->_net_send_buffer_size);
            cnrn_target_memcpy_to_device(&(d_nt->_net_send_buffer), &d_net_send_buffer);
        }

        if (nt->n_vecplay) {
            /* copy VecPlayContinuous instances */
            /** just empty containers */
            void** d_vecplay = cnrn_target_copyin(nt->_vecplay, nt->n_vecplay);
            cnrn_target_memcpy_to_device(&(d_nt->_vecplay), &d_vecplay);

            nrn_VecPlay_copyto_device(nt, d_vecplay);
        }

        if (nt->_permute) {
            if (interleave_permute_type == 1) {
                /* todo: not necessary to setup pointers, just copy it */
                InterleaveInfo* info = interleave_info + i;
                int* d_ptr = nullptr;
                InterleaveInfo* d_info = cnrn_target_copyin(info);

                d_ptr = cnrn_target_copyin(info->stride, info->nstride + 1);
                cnrn_target_memcpy_to_device(&(d_info->stride), &d_ptr);

                d_ptr = cnrn_target_copyin(info->firstnode, nt->ncell);
                cnrn_target_memcpy_to_device(&(d_info->firstnode), &d_ptr);

                d_ptr = cnrn_target_copyin(info->lastnode, nt->ncell);
                cnrn_target_memcpy_to_device(&(d_info->lastnode), &d_ptr);

                d_ptr = cnrn_target_copyin(info->cellsize, nt->ncell);
                cnrn_target_memcpy_to_device(&(d_info->cellsize), &d_ptr);

            } else if (interleave_permute_type == 2) {
                /* todo: not necessary to setup pointers, just copy it */
                InterleaveInfo* info = interleave_info + i;
                InterleaveInfo* d_info = cnrn_target_copyin(info);
                int* d_ptr = nullptr;

                d_ptr = cnrn_target_copyin(info->stride, info->nstride);
                cnrn_target_memcpy_to_device(&(d_info->stride), &d_ptr);

                d_ptr = cnrn_target_copyin(info->firstnode, info->nwarp + 1);
                cnrn_target_memcpy_to_device(&(d_info->firstnode), &d_ptr);

                d_ptr = cnrn_target_copyin(info->lastnode, info->nwarp + 1);
                cnrn_target_memcpy_to_device(&(d_info->lastnode), &d_ptr);

                d_ptr = cnrn_target_copyin(info->stridedispl, info->nwarp + 1);
                cnrn_target_memcpy_to_device(&(d_info->stridedispl), &d_ptr);

                d_ptr = cnrn_target_copyin(info->cellsize, info->nwarp);
                cnrn_target_memcpy_to_device(&(d_info->cellsize), &d_ptr);
            } else {
                printf("\n ERROR: only --cell_permute = [12] implemented");
                abort();
            }
        } else {
            printf("\n WARNING: NrnThread %d not permuted, error for linear algebra?", i);
        }

        {
            TrajectoryRequests* tr = nt->trajec_requests;
            if (tr) {
                // Create a device-side copy of the `trajec_requests` struct and
                // make sure the device-side NrnThread object knows about it.
                TrajectoryRequests* d_trajec_requests = cnrn_target_copyin(tr);
                cnrn_target_memcpy_to_device(&(d_nt->trajec_requests), &d_trajec_requests);
                // Initialise the double** gather member of the struct.
                double** d_tr_gather = cnrn_target_copyin(tr->gather, tr->n_trajec);
                cnrn_target_memcpy_to_device(&(d_trajec_requests->gather), &d_tr_gather);
                // Initialise the double** varrays member of the struct if it's
                // set.
                double** d_tr_varrays{nullptr};
                if (tr->varrays) {
                    d_tr_varrays = cnrn_target_copyin(tr->varrays, tr->n_trajec);
                    cnrn_target_memcpy_to_device(&(d_trajec_requests->varrays), &d_tr_varrays);
                }
                for (int i = 0; i < tr->n_trajec; ++i) {
                    if (tr->varrays) {
                        // tr->varrays[i] is a buffer of tr->bsize doubles on the host,
                        // make a device-side copy of it and store a pointer to it in
                        // the device-side version of tr->varrays.
                        double* d_buf_traj_i = cnrn_target_copyin(tr->varrays[i], tr->bsize);
                        cnrn_target_memcpy_to_device(&(d_tr_varrays[i]), &d_buf_traj_i);
                    }
                    // tr->gather[i] is a double* referring to (host) data in the
                    // (host) _data block
                    auto* d_gather_i = cnrn_target_deviceptr(tr->gather[i]);
                    cnrn_target_memcpy_to_device(&(d_tr_gather[i]), &d_gather_i);
                }
                // TODO: other `double** scatter` and `void** vpr` members of
                // the TrajectoryRequests struct are not copied to the device.
                // The `int vsize` member is updated during the simulation but
                // not kept up to date timestep-by-timestep on the device.
            }
        }
        {
            auto* d_fornetcon_perm_indices = cnrn_target_copyin(nt->_fornetcon_perm_indices,
                                                                nt->_fornetcon_perm_indices_size);
            cnrn_target_memcpy_to_device(&(d_nt->_fornetcon_perm_indices),
                                         &d_fornetcon_perm_indices);
        }
        {
            auto* d_fornetcon_weight_perm = cnrn_target_copyin(nt->_fornetcon_weight_perm,
                                                               nt->_fornetcon_weight_perm_size);
            cnrn_target_memcpy_to_device(&(d_nt->_fornetcon_weight_perm), &d_fornetcon_weight_perm);
        }
    }

#endif
#else
    (void) threads;
    (void) nthreads;
#endif
}

void copy_ivoc_vect_to_device(const IvocVect& from, IvocVect& to) {
#ifdef CORENEURON_ENABLE_GPU
    /// by default `to` is desitionation pointer on a device
    IvocVect* d_iv = &to;

    size_t n = from.size();
    if (n) {
        double* d_data = cnrn_target_copyin(from.data(), n);
        cnrn_target_memcpy_to_device(&(d_iv->data_), &d_data);
    }
#else
    (void) from;
    (void) to;
#endif
}

void delete_ivoc_vect_from_device(IvocVect& vec) {
#ifdef CORENEURON_ENABLE_GPU
    auto const n = vec.size();
    if (n) {
        cnrn_target_delete(vec.data(), n);
    }
#else
    static_cast<void>(vec);
#endif
}

void realloc_net_receive_buffer(NrnThread* nt, Memb_list* ml) {
    NetReceiveBuffer_t* nrb = ml->_net_receive_buffer;
    if (!nrb) {
        return;
    }

#ifdef CORENEURON_ENABLE_GPU
    if (nt->compute_gpu) {
        // free existing vectors in buffers on gpu
        cnrn_target_delete(nrb->_pnt_index, nrb->_size);
        cnrn_target_delete(nrb->_weight_index, nrb->_size);
        cnrn_target_delete(nrb->_nrb_t, nrb->_size);
        cnrn_target_delete(nrb->_nrb_flag, nrb->_size);
        cnrn_target_delete(nrb->_displ, nrb->_size + 1);
        cnrn_target_delete(nrb->_nrb_index, nrb->_size);
    }
#endif
    // Reallocate host buffers using ecalloc_align (as in phase2.cpp) and
    // free_memory (as in nrn_setup.cpp)
    auto const realloc = [old_size = nrb->_size, nrb](auto*& ptr, std::size_t extra_size = 0) {
        using T = std::remove_pointer_t<std::remove_reference_t<decltype(ptr)>>;
        static_assert(std::is_trivial<T>::value,
                      "Only trivially constructible and copiable types are supported.");
        static_assert(std::is_same<decltype(ptr), T*&>::value,
                      "ptr should be reference-to-pointer");
        auto* const new_data = static_cast<T*>(ecalloc_align((nrb->_size + extra_size), sizeof(T)));
        std::memcpy(new_data, ptr, (old_size + extra_size) * sizeof(T));
        free_memory(ptr);
        ptr = new_data;
    };
    nrb->_size *= 2;
    realloc(nrb->_pnt_index);
    realloc(nrb->_weight_index);
    realloc(nrb->_nrb_t);
    realloc(nrb->_nrb_flag);
    realloc(nrb->_displ, 1);
    realloc(nrb->_nrb_index);
#ifdef CORENEURON_ENABLE_GPU
    if (nt->compute_gpu) {
        // update device copy
        nrn_pragma_acc(update device(nrb));
        nrn_pragma_omp(target update to(nrb));

        NetReceiveBuffer_t* const d_nrb{cnrn_target_deviceptr(nrb)};
        // recopy the vectors in the buffer
        int* const d_pnt_index{cnrn_target_copyin(nrb->_pnt_index, nrb->_size)};
        cnrn_target_memcpy_to_device(&(d_nrb->_pnt_index), &d_pnt_index);

        int* const d_weight_index{cnrn_target_copyin(nrb->_weight_index, nrb->_size)};
        cnrn_target_memcpy_to_device(&(d_nrb->_weight_index), &d_weight_index);

        double* const d_nrb_t{cnrn_target_copyin(nrb->_nrb_t, nrb->_size)};
        cnrn_target_memcpy_to_device(&(d_nrb->_nrb_t), &d_nrb_t);

        double* const d_nrb_flag{cnrn_target_copyin(nrb->_nrb_flag, nrb->_size)};
        cnrn_target_memcpy_to_device(&(d_nrb->_nrb_flag), &d_nrb_flag);

        int* const d_displ{cnrn_target_copyin(nrb->_displ, nrb->_size + 1)};
        cnrn_target_memcpy_to_device(&(d_nrb->_displ), &d_displ);

        int* const d_nrb_index{cnrn_target_copyin(nrb->_nrb_index, nrb->_size)};
        cnrn_target_memcpy_to_device(&(d_nrb->_nrb_index), &d_nrb_index);
    }
#endif
}

using NRB_P = std::pair<int, int>;

struct comp {
    bool operator()(const NRB_P& a, const NRB_P& b) {
        if (a.first == b.first) {
            return a.second > b.second;  // same instances in original net_receive order
        }
        return a.first > b.first;
    }
};

static void net_receive_buffer_order(NetReceiveBuffer_t* nrb) {
    Instrumentor::phase p_net_receive_buffer_order("net-receive-buf-order");
    if (nrb->_cnt == 0) {
        nrb->_displ_cnt = 0;
        return;
    }

    std::priority_queue<NRB_P, std::vector<NRB_P>, comp> nrbq;

    for (int i = 0; i < nrb->_cnt; ++i) {
        nrbq.push(NRB_P(nrb->_pnt_index[i], i));
    }

    int displ_cnt = 0;
    int index_cnt = 0;
    int last_instance_index = -1;
    nrb->_displ[0] = 0;

    while (!nrbq.empty()) {
        const NRB_P& p = nrbq.top();
        nrb->_nrb_index[index_cnt++] = p.second;
        if (p.first != last_instance_index) {
            ++displ_cnt;
        }
        nrb->_displ[displ_cnt] = index_cnt;
        last_instance_index = p.first;
        nrbq.pop();
    }
    nrb->_displ_cnt = displ_cnt;
}

/* when we execute NET_RECEIVE block on GPU, we provide the index of synapse instances
 * which we need to execute during the current timestep. In order to do this, we have
 * update NetReceiveBuffer_t object to GPU. When size of cpu buffer changes, we set
 * reallocated to true and hence need to reallocate buffer on GPU and then need to copy
 * entire buffer. If reallocated is 0, that means buffer size is not changed and hence
 * only need to copy _size elements to GPU.
 * Note: this is very preliminary implementation, optimisations will be done after first
 * functional version.
 */
void update_net_receive_buffer(NrnThread* nt) {
    Instrumentor::phase p_update_net_receive_buffer("update-net-receive-buf");
    for (auto tml = nt->tml; tml; tml = tml->next) {
        int is_art = corenrn.get_is_artificial()[tml->index];
        if (is_art) {
            continue;
        }
        // net_receive buffer to copy
        NetReceiveBuffer_t* nrb = tml->ml->_net_receive_buffer;

        // if net receive buffer exist for mechanism
        if (nrb && nrb->_cnt) {
            // instance order to avoid race. setup _displ and _nrb_index
            net_receive_buffer_order(nrb);

            if (nt->compute_gpu) {
                Instrumentor::phase p_net_receive_buffer_order("net-receive-buf-cpu2gpu");
                // note that dont update nrb otherwise we lose pointers

                // clang-format off

                /* update scalar elements */
                nrn_pragma_acc(update device(nrb->_cnt,
                                             nrb->_displ_cnt,
                                             nrb->_pnt_index[:nrb->_cnt],
                                             nrb->_weight_index[:nrb->_cnt],
                                             nrb->_nrb_t[:nrb->_cnt],
                                             nrb->_nrb_flag[:nrb->_cnt],
                                             nrb->_displ[:nrb->_displ_cnt + 1],
                                             nrb->_nrb_index[:nrb->_cnt])
                                             async(nt->stream_id))
                nrn_pragma_omp(target update to(nrb->_cnt,
                                                nrb->_displ_cnt,
                                                nrb->_pnt_index[:nrb->_cnt],
                                                nrb->_weight_index[:nrb->_cnt],
                                                nrb->_nrb_t[:nrb->_cnt],
                                                nrb->_nrb_flag[:nrb->_cnt],
                                                nrb->_displ[:nrb->_displ_cnt + 1],
                                                nrb->_nrb_index[:nrb->_cnt]))
                // clang-format on
            }
        }
    }
    nrn_pragma_acc(wait(nt->stream_id))
}

void update_net_send_buffer_on_host(NrnThread* nt, NetSendBuffer_t* nsb) {
#ifdef CORENEURON_ENABLE_GPU
    if (!nt->compute_gpu)
        return;

    // check if nsb->_cnt was exceeded on GPU: as the buffer can not be increased
    // during gpu execution, we should just abort the execution.
    // \todo: this needs to be fixed with different memory allocation strategy
    if (nsb->_cnt > nsb->_size) {
        printf("ERROR: NetSendBuffer exceeded during GPU execution (rank %d)\n", nrnmpi_myid);
        nrn_abort(1);
    }

    if (nsb->_cnt) {
        Instrumentor::phase p_net_receive_buffer_order("net-send-buf-gpu2cpu");
    }
    // clang-format off
    nrn_pragma_acc(update self(nsb->_sendtype[:nsb->_cnt],
                               nsb->_vdata_index[:nsb->_cnt],
                               nsb->_pnt_index[:nsb->_cnt],
                               nsb->_weight_index[:nsb->_cnt],
                               nsb->_nsb_t[:nsb->_cnt],
                               nsb->_nsb_flag[:nsb->_cnt])
                          if (nsb->_cnt))
    nrn_pragma_omp(target update from(nsb->_sendtype[:nsb->_cnt],
                                      nsb->_vdata_index[:nsb->_cnt],
                                      nsb->_pnt_index[:nsb->_cnt],
                                      nsb->_weight_index[:nsb->_cnt],
                                      nsb->_nsb_t[:nsb->_cnt],
                                      nsb->_nsb_flag[:nsb->_cnt])
                                 if (nsb->_cnt))
    // clang-format on
#else
    (void) nt;
    (void) nsb;
#endif
}

void update_nrnthreads_on_host(NrnThread* threads, int nthreads) {
#ifdef CORENEURON_ENABLE_GPU

    for (int i = 0; i < nthreads; i++) {
        NrnThread* nt = threads + i;

        if (nt->compute_gpu && (nt->end > 0)) {
            /* -- copy data to host -- */

            int ne = nrn_soa_padded_size(nt->end, 0);

            // clang-format off
            nrn_pragma_acc(update self(nt->_actual_rhs[:ne],
                                       nt->_actual_d[:ne],
                                       nt->_actual_a[:ne],
                                       nt->_actual_b[:ne],
                                       nt->_actual_v[:ne],
                                       nt->_actual_area[:ne]))
            nrn_pragma_omp(target update from(nt->_actual_rhs[:ne],
                                              nt->_actual_d[:ne],
                                              nt->_actual_a[:ne],
                                              nt->_actual_b[:ne],
                                              nt->_actual_v[:ne],
                                              nt->_actual_area[:ne]))
            // clang-format on

            nrn_pragma_acc(update self(nt->_actual_diam[:ne]) if (nt->_actual_diam != nullptr))
            nrn_pragma_omp(
                target update from(nt->_actual_diam[:ne]) if (nt->_actual_diam != nullptr))

            /* @todo: nt._ml_list[tml->index] = tml->ml; */

            /* -- copy NrnThreadMembList list ml to host -- */
            for (auto tml = nt->tml; tml; tml = tml->next) {
                if (!corenrn.get_is_artificial()[tml->index]) {
                    nrn_pragma_acc(update self(tml->index, tml->ml->nodecount))
                    nrn_pragma_omp(target update from(tml->index, tml->ml->nodecount))
                }
                update_ml_on_host(tml->ml, tml->index);
            }

            int pcnt = nrn_soa_padded_size(nt->shadow_rhs_cnt, 0);
            /* copy shadow_rhs to host */
            /* copy shadow_d to host */
            nrn_pragma_acc(
                update self(nt->_shadow_rhs[:pcnt], nt->_shadow_d[:pcnt]) if (nt->shadow_rhs_cnt))
            nrn_pragma_omp(target update from(
                nt->_shadow_rhs[:pcnt], nt->_shadow_d[:pcnt]) if (nt->shadow_rhs_cnt))

            // clang-format off
            nrn_pragma_acc(update self(nt->nrn_fast_imem->nrn_sav_rhs[:nt->end],
                                       nt->nrn_fast_imem->nrn_sav_d[:nt->end])
                                  if (nt->nrn_fast_imem != nullptr))
            nrn_pragma_omp(target update from(nt->nrn_fast_imem->nrn_sav_rhs[:nt->end],
                                              nt->nrn_fast_imem->nrn_sav_d[:nt->end])
                                         if (nt->nrn_fast_imem != nullptr))
            // clang-format on

            nrn_pragma_acc(update self(nt->pntprocs[:nt->n_pntproc]) if (nt->n_pntproc))
            nrn_pragma_omp(target update from(nt->pntprocs[:nt->n_pntproc]) if (nt->n_pntproc))

            nrn_pragma_acc(update self(nt->weights[:nt->n_weight]) if (nt->n_weight))
            nrn_pragma_omp(target update from(nt->weights[:nt->n_weight]) if (nt->n_weight))

            nrn_pragma_acc(update self(
                nt->presyns_helper[:nt->n_presyn], nt->presyns[:nt->n_presyn]) if (nt->n_presyn))
            nrn_pragma_omp(target update from(
                nt->presyns_helper[:nt->n_presyn], nt->presyns[:nt->n_presyn]) if (nt->n_presyn))

            {
                TrajectoryRequests* tr = nt->trajec_requests;
                if (tr && tr->varrays) {
                    // The full buffers have `bsize` entries, but only `vsize`
                    // of them are valid.
                    for (int i = 0; i < tr->n_trajec; ++i) {
                        nrn_pragma_acc(update self(tr->varrays[i][:tr->vsize]))
                        nrn_pragma_omp(target update from(tr->varrays[i][:tr->vsize]))
                    }
                }
            }

            /* dont update vdata, its pointer array
               nrn_pragma_acc(update self(nt->_vdata[:nt->_nvdata) if nt->_nvdata)
               nrn_pragma_omp(target update from(nt->_vdata[:nt->_nvdata) if (nt->_nvdata))
             */
        }
    }
#else
    (void) threads;
    (void) nthreads;
#endif
}

/**
 * Copy weights from GPU to CPU
 *
 * User may record NetCon weights at the end of simulation.
 * For this purpose update weights of all NrnThread objects
 * from GPU to CPU.
 */
void update_weights_from_gpu(NrnThread* threads, int nthreads) {
#ifdef CORENEURON_ENABLE_GPU
    for (int i = 0; i < nthreads; i++) {
        NrnThread* nt = threads + i;
        size_t n_weight = nt->n_weight;
        if (nt->compute_gpu && n_weight > 0) {
            double* weights = nt->weights;
            nrn_pragma_acc(update host(weights [0:n_weight]))
            nrn_pragma_omp(target update from(weights [0:n_weight]))
        }
    }
#endif
}

/** Cleanup device memory that is being tracked by the OpenACC runtime.
 *
 *  This function painstakingly calls `cnrn_target_delete` in reverse order on all
 *  pointers that were passed to `cnrn_target_copyin` in `setup_nrnthreads_on_device`.
 *  This cleanup ensures that if the GPU is initialised multiple times from the
 *  same process then the OpenACC runtime will not be polluted with old
 *  pointers, which can cause errors. In particular if we do:
 *  @code
 *    {
 *      // ... some_ptr is dynamically allocated ...
 *      cnrn_target_copyin(some_ptr, some_size);
 *      // ... do some work ...
 *      // cnrn_target_delete(some_ptr);
 *      free(some_ptr);
 *    }
 *    {
 *      // ... same_ptr_again is dynamically allocated at the same address ...
 *      cnrn_target_copyin(same_ptr_again, some_other_size); // ERROR
 *    }
 *  @endcode
 *  the application will/may abort with an error such as:
 *    FATAL ERROR: variable in data clause is partially present on the device.
 *  The pattern above is typical of calling CoreNEURON on GPU multiple times in
 *  the same process.
 */
void delete_nrnthreads_on_device(NrnThread* threads, int nthreads) {
#ifdef CORENEURON_ENABLE_GPU
    for (int i = 0; i < nthreads; i++) {
        NrnThread* nt = threads + i;
        if (!nt->compute_gpu) {
            continue;
        }
        cnrn_target_delete(nt->_fornetcon_weight_perm, nt->_fornetcon_weight_perm_size);
        cnrn_target_delete(nt->_fornetcon_perm_indices, nt->_fornetcon_perm_indices_size);
        {
            TrajectoryRequests* tr = nt->trajec_requests;
            if (tr) {
                if (tr->varrays) {
                    for (int i = 0; i < tr->n_trajec; ++i) {
                        cnrn_target_delete(tr->varrays[i], tr->bsize);
                    }
                    cnrn_target_delete(tr->varrays, tr->n_trajec);
                }
                cnrn_target_delete(tr->gather, tr->n_trajec);
                cnrn_target_delete(tr);
            }
        }
        if (nt->_permute) {
            if (interleave_permute_type == 1) {
                InterleaveInfo* info = interleave_info + i;
                cnrn_target_delete(info->cellsize, nt->ncell);
                cnrn_target_delete(info->lastnode, nt->ncell);
                cnrn_target_delete(info->firstnode, nt->ncell);
                cnrn_target_delete(info->stride, info->nstride + 1);
                cnrn_target_delete(info);
            } else if (interleave_permute_type == 2) {
                InterleaveInfo* info = interleave_info + i;
                cnrn_target_delete(info->cellsize, info->nwarp);
                cnrn_target_delete(info->stridedispl, info->nwarp + 1);
                cnrn_target_delete(info->lastnode, info->nwarp + 1);
                cnrn_target_delete(info->firstnode, info->nwarp + 1);
                cnrn_target_delete(info->stride, info->nstride);
                cnrn_target_delete(info);
            }
        }

        if (nt->n_vecplay) {
            nrn_VecPlay_delete_from_device(nt);
            cnrn_target_delete(nt->_vecplay, nt->n_vecplay);
        }

        // Cleanup send_receive buffer.
        if (nt->_net_send_buffer_size) {
            cnrn_target_delete(nt->_net_send_buffer, nt->_net_send_buffer_size);
        }

        if (nt->n_presyn) {
            cnrn_target_delete(nt->presyns, nt->n_presyn);
            cnrn_target_delete(nt->presyns_helper, nt->n_presyn);
        }

        // Cleanup data that's setup in bbcore_read.
        if (nt->_nvdata) {
            cnrn_target_delete(nt->_vdata, nt->_nvdata);
        }

        // Cleanup weight vector used in NET_RECEIVE
        if (nt->n_weight) {
            cnrn_target_delete(nt->weights, nt->n_weight);
        }

        // Cleanup point processes
        if (nt->n_pntproc) {
            cnrn_target_delete(nt->pntprocs, nt->n_pntproc);
        }

        if (nt->nrn_fast_imem) {
            cnrn_target_delete(nt->nrn_fast_imem->nrn_sav_d, nt->end);
            cnrn_target_delete(nt->nrn_fast_imem->nrn_sav_rhs, nt->end);
            cnrn_target_delete(nt->nrn_fast_imem);
        }

        if (nt->shadow_rhs_cnt) {
            int pcnt = nrn_soa_padded_size(nt->shadow_rhs_cnt, 0);
            cnrn_target_delete(nt->_shadow_d, pcnt);
            cnrn_target_delete(nt->_shadow_rhs, pcnt);
        }

        for (auto tml = nt->tml; tml; tml = tml->next) {
            delete_ml_from_device(tml->ml, tml->index);
            cnrn_target_delete(tml);
        }
        cnrn_target_delete(nt->_ml_list, corenrn.get_memb_funcs().size());
        cnrn_target_delete(nt->_v_parent_index, nt->end);
        cnrn_target_delete(nt->_data, nt->_ndata);
    }
    cnrn_target_delete(threads, nthreads);
    nrn_ion_global_map_delete_from_device();
#endif
}


void nrn_newtonspace_copyto_device(NewtonSpace* ns) {
#ifdef CORENEURON_ENABLE_GPU
    // FIXME this check needs to be tweaked if we ever want to run with a mix
    //       of CPU and GPU threads.
    if (nrn_threads[0].compute_gpu == 0) {
        return;
    }

    int n = ns->n * ns->n_instance;
    // actually, the values of double do not matter, only the  pointers.
    NewtonSpace* d_ns = cnrn_target_copyin(ns);

    double* pd;

    pd = cnrn_target_copyin(ns->delta_x, n);
    cnrn_target_memcpy_to_device(&(d_ns->delta_x), &pd);

    pd = cnrn_target_copyin(ns->high_value, n);
    cnrn_target_memcpy_to_device(&(d_ns->high_value), &pd);

    pd = cnrn_target_copyin(ns->low_value, n);
    cnrn_target_memcpy_to_device(&(d_ns->low_value), &pd);

    pd = cnrn_target_copyin(ns->rowmax, n);
    cnrn_target_memcpy_to_device(&(d_ns->rowmax), &pd);

    auto pint = cnrn_target_copyin(ns->perm, n);
    cnrn_target_memcpy_to_device(&(d_ns->perm), &pint);

    auto ppd = cnrn_target_copyin(ns->jacobian, ns->n);
    cnrn_target_memcpy_to_device(&(d_ns->jacobian), &ppd);

    // the actual jacobian doubles were allocated as a single array
    double* d_jacdat = cnrn_target_copyin(ns->jacobian[0], ns->n * n);

    for (int i = 0; i < ns->n; ++i) {
        pd = d_jacdat + i * n;
        cnrn_target_memcpy_to_device(&(ppd[i]), &pd);
    }
#endif
}

void nrn_newtonspace_delete_from_device(NewtonSpace* ns) {
#ifdef CORENEURON_ENABLE_GPU
    // FIXME this check needs to be tweaked if we ever want to run with a mix
    //       of CPU and GPU threads.
    if (nrn_threads[0].compute_gpu == 0) {
        return;
    }
    int n = ns->n * ns->n_instance;
    cnrn_target_delete(ns->jacobian[0], ns->n * n);
    cnrn_target_delete(ns->jacobian, ns->n);
    cnrn_target_delete(ns->perm, n);
    cnrn_target_delete(ns->rowmax, n);
    cnrn_target_delete(ns->low_value, n);
    cnrn_target_delete(ns->high_value, n);
    cnrn_target_delete(ns->delta_x, n);
    cnrn_target_delete(ns);
#endif
}

void nrn_sparseobj_copyto_device(SparseObj* so) {
#if defined(CORENEURON_ENABLE_GPU) && !defined(CORENEURON_UNIFIED_MEMORY)
    // FIXME this check needs to be tweaked if we ever want to run with a mix
    //       of CPU and GPU threads.
    if (nrn_threads[0].compute_gpu == 0) {
        return;
    }

    unsigned n1 = so->neqn + 1;
    SparseObj* d_so = cnrn_target_copyin(so);
    // only pointer fields in SparseObj that need setting up are
    //   rowst, diag, rhs, ngetcall, coef_list
    // only pointer fields in Elm that need setting up are
    //   r_down, c_right, value
    // do not care about the Elm* ptr value, just the space.

    Elm** d_rowst = cnrn_target_copyin(so->rowst, n1);
    cnrn_target_memcpy_to_device(&(d_so->rowst), &d_rowst);

    Elm** d_diag = cnrn_target_copyin(so->diag, n1);
    cnrn_target_memcpy_to_device(&(d_so->diag), &d_diag);

    unsigned* pu = cnrn_target_copyin(so->ngetcall, so->_cntml_padded);
    cnrn_target_memcpy_to_device(&(d_so->ngetcall), &pu);

    double* pd = cnrn_target_copyin(so->rhs, n1 * so->_cntml_padded);
    cnrn_target_memcpy_to_device(&(d_so->rhs), &pd);

    double** d_coef_list = cnrn_target_copyin(so->coef_list, so->coef_list_size);
    cnrn_target_memcpy_to_device(&(d_so->coef_list), &d_coef_list);

    // Fill in relevant Elm pointer values

    for (unsigned irow = 1; irow < n1; ++irow) {
        for (Elm* elm = so->rowst[irow]; elm; elm = elm->c_right) {
            Elm* pelm = cnrn_target_copyin(elm);

            if (elm == so->rowst[irow]) {
                cnrn_target_memcpy_to_device(&(d_rowst[irow]), &pelm);
            } else {
                Elm* d_e = cnrn_target_deviceptr(elm->c_left);
                cnrn_target_memcpy_to_device(&(pelm->c_left), &d_e);
            }

            if (elm->col == elm->row) {
                cnrn_target_memcpy_to_device(&(d_diag[irow]), &pelm);
            }

            if (irow > 1) {
                if (elm->r_up) {
                    Elm* d_e = cnrn_target_deviceptr(elm->r_up);
                    cnrn_target_memcpy_to_device(&(pelm->r_up), &d_e);
                }
            }

            pd = cnrn_target_copyin(elm->value, so->_cntml_padded);
            cnrn_target_memcpy_to_device(&(pelm->value), &pd);
        }
    }

    // visit all the Elm again and fill in pelm->r_down and pelm->c_left
    for (unsigned irow = 1; irow < n1; ++irow) {
        for (Elm* elm = so->rowst[irow]; elm; elm = elm->c_right) {
            auto pelm = cnrn_target_deviceptr(elm);
            if (elm->r_down) {
                auto d_e = cnrn_target_deviceptr(elm->r_down);
                cnrn_target_memcpy_to_device(&(pelm->r_down), &d_e);
            }
            if (elm->c_right) {
                auto d_e = cnrn_target_deviceptr(elm->c_right);
                cnrn_target_memcpy_to_device(&(pelm->c_right), &d_e);
            }
        }
    }

    // Fill in the d_so->coef_list
    for (unsigned i = 0; i < so->coef_list_size; ++i) {
        pd = cnrn_target_deviceptr(so->coef_list[i]);
        cnrn_target_memcpy_to_device(&(d_coef_list[i]), &pd);
    }
#endif
}

void nrn_sparseobj_delete_from_device(SparseObj* so) {
#if defined(CORENEURON_ENABLE_GPU) && !defined(CORENEURON_UNIFIED_MEMORY)
    // FIXME this check needs to be tweaked if we ever want to run with a mix
    //       of CPU and GPU threads.
    if (nrn_threads[0].compute_gpu == 0) {
        return;
    }
    unsigned n1 = so->neqn + 1;
    for (unsigned irow = 1; irow < n1; ++irow) {
        for (Elm* elm = so->rowst[irow]; elm; elm = elm->c_right) {
            cnrn_target_delete(elm->value, so->_cntml_padded);
            cnrn_target_delete(elm);
        }
    }
    cnrn_target_delete(so->coef_list, so->coef_list_size);
    cnrn_target_delete(so->rhs, n1 * so->_cntml_padded);
    cnrn_target_delete(so->ngetcall, so->_cntml_padded);
    cnrn_target_delete(so->diag, n1);
    cnrn_target_delete(so->rowst, n1);
    cnrn_target_delete(so);
#endif
}

#ifdef CORENEURON_ENABLE_GPU

void nrn_ion_global_map_copyto_device() {
    if (nrn_ion_global_map_size) {
        double** d_data = cnrn_target_copyin(nrn_ion_global_map, nrn_ion_global_map_size);
        for (int j = 0; j < nrn_ion_global_map_size; j++) {
            if (nrn_ion_global_map[j]) {
                double* d_mechmap = cnrn_target_copyin(nrn_ion_global_map[j],
                                                       ion_global_map_member_size);
                cnrn_target_memcpy_to_device(&(d_data[j]), &d_mechmap);
            }
        }
    }
}

void nrn_ion_global_map_delete_from_device() {
    for (int j = 0; j < nrn_ion_global_map_size; j++) {
        if (nrn_ion_global_map[j]) {
            cnrn_target_delete(nrn_ion_global_map[j], ion_global_map_member_size);
        }
    }
    if (nrn_ion_global_map_size) {
        cnrn_target_delete(nrn_ion_global_map, nrn_ion_global_map_size);
    }
}

void init_gpu() {
    // check how many gpu devices available per node
    int num_devices_per_node = cnrn_target_get_num_devices();

    // if no gpu found, can't run on GPU
    if (num_devices_per_node == 0) {
        nrn_fatal_error("\n ERROR : Enabled GPU execution but couldn't find NVIDIA GPU!\n");
    }

    if (corenrn_param.num_gpus != 0) {
        if (corenrn_param.num_gpus > num_devices_per_node) {
            nrn_fatal_error("Fatal error: asking for '%d' GPUs per node but only '%d' available\n",
                            corenrn_param.num_gpus,
                            num_devices_per_node);
        } else {
            num_devices_per_node = corenrn_param.num_gpus;
        }
    }

    // get local rank within a node and assign specific gpu gpu for this node.
    // multiple threads within the node will use same device.
    int local_rank = 0;
    int local_size = 1;
#if NRNMPI
    if (corenrn_param.mpi_enable) {
        local_rank = nrnmpi_local_rank();
        local_size = nrnmpi_local_size();
    }
#endif

    cnrn_target_set_default_device(local_rank % num_devices_per_node);

    if (nrnmpi_myid == 0 && !corenrn_param.is_quiet()) {
        std::cout << " Info : " << num_devices_per_node << " GPUs shared by " << local_size
                  << " ranks per node\n";
    }
}

void nrn_VecPlay_copyto_device(NrnThread* nt, void** d_vecplay) {
    for (int i = 0; i < nt->n_vecplay; i++) {
        VecPlayContinuous* vecplay_instance = (VecPlayContinuous*) nt->_vecplay[i];

        /** just VecPlayContinuous object */
        VecPlayContinuous* d_vecplay_instance = cnrn_target_copyin(vecplay_instance);
        cnrn_target_memcpy_to_device((VecPlayContinuous**) (&(d_vecplay[i])), &d_vecplay_instance);

        /** copy y_, t_ and discon_indices_ */
        copy_ivoc_vect_to_device(vecplay_instance->y_, d_vecplay_instance->y_);
        copy_ivoc_vect_to_device(vecplay_instance->t_, d_vecplay_instance->t_);
        // OL211213: beware, the test suite does not currently include anything
        // with a non-null discon_indices_.
        if (vecplay_instance->discon_indices_) {
            IvocVect* d_discon_indices = cnrn_target_copyin(vecplay_instance->discon_indices_);
            cnrn_target_memcpy_to_device(&(d_vecplay_instance->discon_indices_), &d_discon_indices);
            copy_ivoc_vect_to_device(*(vecplay_instance->discon_indices_),
                                     *(d_vecplay_instance->discon_indices_));
        }

        /** copy PlayRecordEvent : todo: verify this */
        PlayRecordEvent* d_e_ = cnrn_target_copyin(vecplay_instance->e_);

        cnrn_target_memcpy_to_device(&(d_e_->plr_), (PlayRecord**) (&d_vecplay_instance));
        cnrn_target_memcpy_to_device(&(d_vecplay_instance->e_), &d_e_);

        /** copy pd_ : note that it's pointer inside ml->data and hence data itself is
         * already on GPU */
        double* d_pd_ = cnrn_target_deviceptr(vecplay_instance->pd_);
        cnrn_target_memcpy_to_device(&(d_vecplay_instance->pd_), &d_pd_);
    }
}

void nrn_VecPlay_delete_from_device(NrnThread* nt) {
    for (int i = 0; i < nt->n_vecplay; i++) {
        auto* vecplay_instance = static_cast<VecPlayContinuous*>(nt->_vecplay[i]);
        cnrn_target_delete(vecplay_instance->e_);
        if (vecplay_instance->discon_indices_) {
            delete_ivoc_vect_from_device(*(vecplay_instance->discon_indices_));
        }
        delete_ivoc_vect_from_device(vecplay_instance->t_);
        delete_ivoc_vect_from_device(vecplay_instance->y_);
        cnrn_target_delete(vecplay_instance);
    }
}

#endif
}  // namespace coreneuron
