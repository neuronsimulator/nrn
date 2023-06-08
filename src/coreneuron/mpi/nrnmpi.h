/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#pragma once

#include <array>
#include <cassert>
#include <stdexcept>

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

struct NRNMPI_Spike {
    int gid;
    double spiketime;
};

// Those functions and classes are part of a mechanism to dynamically or statically load mpi
// functions
struct mpi_function_base;

struct mpi_manager_t {
    void register_function(mpi_function_base* ptr) {
        if (m_num_function_ptrs == max_mpi_functions) {
            throw std::runtime_error("mpi_manager_t::max_mpi_functions reached");
        }
        m_function_ptrs[m_num_function_ptrs++] = ptr;
    }
    void resolve_symbols(void* dlsym_handle);

  private:
    constexpr static auto max_mpi_functions = 128;
    std::size_t m_num_function_ptrs{};
    std::array<mpi_function_base*, max_mpi_functions> m_function_ptrs{};
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
#define declare_mpi_method(x)                    \
    inline mpi_function<decltype(&x##_impl)> x { \
#x "_impl"                               \
    }
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
#define declare_mpi_method(x)         \
    inline mpi_function<x##_impl> x { \
#x "_impl"                    \
    }
#endif

}  // namespace coreneuron
#include "coreneuron/mpi/nrnmpidec.h"
