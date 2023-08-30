/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#pragma once

#include <initializer_list>
#include <type_traits>

#if defined(NRN_CALIPER)
#include <caliper/cali.h>
#endif

#ifdef CORENEURON_CUDA_PROFILING
#include <cuda_profiler_api.h>
#endif

#if defined(CRAYPAT)
#include <pat_api.h>
#endif

#if defined(TAU)
#include <TAU.h>
#endif

#if defined(LIKWID_PERFMON)
#include <likwid.h>
#endif

namespace coreneuron {

namespace detail {

/*! \class Instrumentor
 *  \brief Instrumentation infrastructure for benchmarking and profiling.
 *
 *  The Instrumentor class exposes static methods that can be used to
 *  toggle with fine-grained resolution the profiling of specific
 *  areas within the code.
 */
template <class... TProfilerImpl>
struct Instrumentor {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-value"
    /*! \fn phase_begin
     *  \brief Activate the collection of profiling data within a code region.
     *
     *  This function semantically defines the beginning of a region
     *  of code that the user wishes to profile.
     *  Loops through all enabled profilers and calls the relevant
     *  `phase_begin` function.
     *  This function should have a non-empty implementation only for
     *  profilers that allow multiple code regions with different names
     *  to be profiled concurrently.
     *
     *  @param name the (unique) identifier of the code region to be profiled
     */
    inline static void phase_begin(const char* name) {
        std::initializer_list<int>{(TProfilerImpl::phase_begin(name), 0)...};
    }

    /*! \fn phase_end
     *  \brief Deactivate the collection of profiling data within a code region.
     *
     *  This function semantically defines the end of a region
     *  of code that the user wishes to profile.
     *  Loops through all enabled profilers and calls the relevant
     *  `phase_end` function.
     *  This function should have a non-empty implementation only for
     *  profilers that allow multiple code regions with different names
     *  to be profiled concurrently.
     *
     *  @param name the (unique) identifier of the code region to be profiled
     */
    inline static void phase_end(const char* name) {
        std::initializer_list<int>{(TProfilerImpl::phase_end(name), 0)...};
    }

    /*! \fn start_profile
     *  \brief Globally activate the collection of profiling data.
     *
     *  Activate the collection of profiler data without defining
     *  a region of interest with a given name, as opposed to `phase_begin`.
     *  Loops through all enabled profilers and calls the relevant
     *  `start_profile` function.
     *  This function should have a non-empty implementation only for
     *  profilers that expose simply a global begin/end interface, without
     *  named regions.
     */
    inline static void start_profile() {
        std::initializer_list<int>{(TProfilerImpl::start_profile(), 0)...};
    }

    /*! \fn stop_profile
     *  \brief Globally deactivate the collection of profiling data.
     *
     *  Deactivate the collection of profiler data without defining
     *  a region of interest with a given name, as opposed to `phase_end`.
     *  Loops through all enabled profilers and calls the relevant
     *  `stop_profile` function.
     *  This function should have a non-empty implementation only for
     *  profilers that expose simply a global begin/end interface, without
     *  named regions.
     */
    inline static void stop_profile() {
        std::initializer_list<int>{(TProfilerImpl::stop_profile(), 0)...};
    }

    /*! \fn init_profile
     *  \brief Initialize the profiler.
     *
     *  Initialize a profiler's internal structure, without activating yet
     *  any data collection, similar in concept to MPI_Init.
     *  Loops through all enabled profilers and calls the relevant
     *  `init_profile` function.
     *  This function should have a non-empty implementation only for
     *  profilers that require special initialization, typically before
     *  any memory allocation is done.
     */
    inline static void init_profile() {
        std::initializer_list<int>{(TProfilerImpl::init_profile(), 0)...};
    }

    /*! \fn finalize_profile
     *  \brief Finalize the profiler.
     *
     *  Finalize a profiler's internal structure, without activating yet
     *  any data collection, similar in concept to MPI_Finalize.
     *  Loops through all enabled profilers and calls the relevant
     *  `finalize_profile` function.
     *  This function should have a non-empty implementation only for
     *  profilers that require special finalization.
     */
    inline static void finalize_profile() {
        std::initializer_list<int>{(TProfilerImpl::finalize_profile(), 0)...};
    }
#pragma clang diagnostic pop
};

#if defined(NRN_CALIPER)

struct Caliper {
    inline static void phase_begin(const char* name) {
        CALI_MARK_BEGIN(name);
    };

    inline static void phase_end(const char* name) {
        CALI_MARK_END(name);
    };

    inline static void start_profile(){};

    inline static void stop_profile(){};

    inline static void init_profile(){};

    inline static void finalize_profile(){};
};

#endif

#ifdef CORENEURON_CUDA_PROFILING

struct CudaProfiling {
    inline static void phase_begin(const char* name){};

    inline static void phase_end(const char* name){};

    inline static void start_profile() {
        cudaProfilerStart();
    };

    inline static void stop_profile() {
        cudaProfilerStop();
    };

    inline static void init_profile(){};

    inline static void finalize_profile(){};
};

#endif

#if defined(CRAYPAT)

struct CrayPat {
    inline static void phase_begin(const char* name){};

    inline static void phase_end(const char* name){};

    inline static void start_profile() {
        PAT_record(PAT_STATE_ON);
    };

    inline static void stop_profile() {
        PAT_record(PAT_STATE_OFF);
    };

    inline static void init_profile(){};

    inline static void finalize_profile(){};
};
#endif

#if defined(TAU)

struct Tau {
    inline static void phase_begin(const char* name){};

    inline static void phase_end(const char* name){};

    inline static void start_profile() {
        TAU_ENABLE_INSTRUMENTATION();
    };

    inline static void stop_profile() {
        TAU_DISABLE_INSTRUMENTATION();
    };

    inline static void init_profile(){};

    inline static void finalize_profile(){};
};

#endif

#if defined(LIKWID_PERFMON)

struct Likwid {
    inline static void phase_begin(const char* name) {
        LIKWID_MARKER_START(name);
    };

    inline static void phase_end(const char* name) {
        LIKWID_MARKER_STOP(name);
    };

    inline static void start_profile(){};

    inline static void stop_profile(){};

    inline static void init_profile() {
        LIKWID_MARKER_INIT;

#pragma omp parallel
        { LIKWID_MARKER_THREADINIT; }
    };

    inline static void finalize_profile() {
        LIKWID_MARKER_CLOSE;
    };
};

#endif

struct NullInstrumentor {
    inline static void phase_begin(const char* name){};
    inline static void phase_end(const char* name){};
    inline static void start_profile(){};
    inline static void stop_profile(){};
    inline static void init_profile(){};
    inline static void finalize_profile(){};
};

using InstrumentorImpl = detail::Instrumentor<
#if defined NRN_CALIPER
    detail::Caliper,
#endif
#ifdef CORENEURON_CUDA_PROFILING
    detail::CudaProfiling,
#endif
#if defined(CRAYPAT)
    detail::CrayPat,
#endif
#if defined(TAU)
    detail::Tau,
#endif
#if defined(LIKWID_PERFMON)
    detail::Likwid,
#endif
    detail::NullInstrumentor>;
}  // namespace detail

namespace Instrumentor {
struct phase {
    const char* phase_name;
    phase(const char* name)
        : phase_name(name) {
        detail::InstrumentorImpl::phase_begin(phase_name);
    }
    ~phase() {
        detail::InstrumentorImpl::phase_end(phase_name);
    }
};

inline static void start_profile() {
    detail::InstrumentorImpl::start_profile();
}

inline static void stop_profile() {
    detail::InstrumentorImpl::stop_profile();
}

inline static void phase_begin(const char* name) {
    detail::InstrumentorImpl::phase_begin(name);
}

inline static void phase_end(const char* name) {
    detail::InstrumentorImpl::phase_end(name);
}

inline static void init_profile() {
    detail::InstrumentorImpl::init_profile();
}

inline static void finalize_profile() {
    detail::InstrumentorImpl::finalize_profile();
}
}  // namespace Instrumentor

}  // namespace coreneuron
