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

#if NRNMPI
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

// Those functions and classes are part of a mechanism to dynamically or statically load mpi functions
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
  operator bool() const { return m_fptr; }
  mpi_function_base(const char* name)
  : m_name{name} {
    mpi_manager().register_function(this);
  }
protected:
  void* m_fptr{};
  const char* m_name;
};

// This could be done with a simpler
//   template <auto fptr> struct function : function_base { ... };
// pattern in C++17...
template <typename>
struct mpi_function {};

#define cnrn_make_integral_constant_t(x) std::integral_constant<std::decay_t<decltype(x)>, x>

template <typename function_ptr, function_ptr fptr>
struct mpi_function<std::integral_constant<function_ptr, fptr>> : mpi_function_base {
    using mpi_function_base::mpi_function_base;
    template <typename... Args> // in principle deducible from `function_ptr`
    auto operator()(Args&&... args) const {
#ifdef CORENRN_ENABLE_DYNAMIC_MPI
        // Dynamic MPI, m_fptr should have been initialised via dlsym.
        assert(m_fptr);
        return (*reinterpret_cast<decltype(fptr)>(m_fptr))(std::forward<Args>( args )...);
#else
        // No dynamic MPI, use `fptr` directly. Will produce link errors if libmpi.so is not linked.
        return (*fptr)(std::forward<Args>(args)...);
#endif
    }
};
}  // namespace coreneuron
#include "coreneuron/mpi/nrnmpidec.h"

#endif /*NRNMPI*/
