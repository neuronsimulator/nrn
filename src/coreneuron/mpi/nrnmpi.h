/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#pragma once

#include <cassert>
#include <string>
#include <type_traits>
#include <vector>

#include "coreneuron/mpi/nrnmpiuse.h"

#ifndef nrn_spikebuf_size
#define nrn_spikebuf_size 0
#endif

namespace coreneuron {
struct NRNMPI_Spikebuf {
    int nspike;
    int gid[nrn_spikebuf_size];
    double spiketime[nrn_spikebuf_size];
};
}  // namespace coreneuron

namespace coreneuron {
struct NRNMPI_Spike {
    int gid;
    double spiketime;
};

// Those functions and classes are part of a mechanism to dynamically or statically load mpi
// functions
struct mpi_function_base;

struct mpi_manager_t {
    void register_function(mpi_function_base* ptr) {
        m_function_ptrs.push_back(ptr);
    }
    void resolve_symbols(void* dlsym_handle);

  private:
    std::vector<mpi_function_base*> m_function_ptrs;
    // true when symbols are resolved
};

inline mpi_manager_t& mpi_manager() {
    static mpi_manager_t x;
    return x;
}

struct mpi_function_base {
    void resolve(void* dlsym_handle);
    operator bool() const {
        return m_fptr;
    }
    mpi_function_base(const char* name)
        : m_name{name} {
        mpi_manager().register_function(this);
    }

  protected:
    void* m_fptr{};
    const char* m_name;
};

#ifdef NRNMPI_DYNAMICLOAD
template <typename fptr>
struct mpi_function: mpi_function_base {
    using mpi_function_base::mpi_function_base;
    template <typename... Args>  // in principle deducible from `function_ptr`
    auto operator()(Args&&... args) const {
        // Dynamic MPI, m_fptr should have been initialised via dlsym.
        assert(m_fptr);
        return (*reinterpret_cast<fptr>(m_fptr))(std::forward<Args>(args)...);
    }
};
#define declare_mpi_method(x) mpi_function<decltype(&x)>
#else
template <auto fptr>
struct mpi_function: mpi_function_base {
    using mpi_function_base::mpi_function_base;
    template <typename... Args>  // in principle deducible from `function_ptr`
    auto operator()(Args&&... args) const {
        // No dynamic MPI, use `fptr` directly. Will produce link errors if libmpi.so is not linked.
        return (*fptr)(std::forward<Args>(args)...);
    }
};
#define declare_mpi_method(x) mpi_function<x>
#endif

}  // namespace coreneuron
#include "coreneuron/mpi/nrnmpidec.h"
