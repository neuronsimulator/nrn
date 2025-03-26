#include <../../nrnconf.h>
#include "nrnmpiuse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <array>
#include <cstdlib>
#include <iostream>
#include <string>

#if NRNMPI_DYNAMICLOAD /* to end of file */

#include "nrnwrap_dlfcn.h"

#include "nrnmpi.h"

#include "utils/logger.hpp"

extern char* cxx_char_alloc(size_t);
extern std::string corenrn_mpi_library;

#if DARWIN
extern void nrn_possible_mismatched_arch(const char*);
#endif

#if DARWIN || defined(__linux__)
extern const char* path_prefix_to_libnrniv();
#endif

#include <cstddef>
#include <string>  // for nrnmpi_str_broadcast_world

#include "mpispike.h"
#include "nrnmpi_def_cinc" /* nrnmpi global variables */
extern "C" {
#include "nrnmpi_dynam_cinc" /* autogenerated file */
}

#include "nrnmpi_dynam_wrappers.inc" /* autogenerated file */
#include "nrnmpi_dynam_stubs.cpp"

static void* load_mpi(const char* name, std::string& mes) {
    void* handle = dlopen(name, RTLD_NOW | RTLD_GLOBAL);
    mes.append(name);
    mes.append(": ");
    if (!handle) {
#if DARWIN
        nrn_possible_mismatched_arch(name);
#endif
        mes.append(dlerror());
    } else {
        mes.append("successful");
    }
    mes.append(1, '\n');
    return handle;
}

static void* load_nrnmpi(const char* name, std::string& mes) {
    void* handle = dlopen(name, RTLD_NOW | RTLD_GLOBAL);
    mes.append("load_nrnmpi: ");
    if (!handle) {
        mes.append(dlerror());
        mes.append("\n");
        return nullptr;
    }
    mes.append(name);
    mes.append("successful\n");
    for (int i = 0; ftable[i].name; ++i) {
        void* p = dlsym(handle, ftable[i].name);
        if (!p) {
            mes.append("load_nrnmpi: ");
            mes.append(ftable[i].name);
            mes.append(1, ' ');
            mes.append(dlerror());
            mes.append(1, '\n');
            dlclose(handle);
            return nullptr;
        }
        *ftable[i].ppf = p;
    }
    {
        auto* const p = reinterpret_cast<char* (**) (std::size_t)>(
            dlsym(handle, "p_cxx_char_alloc"));
        if (!p) {
            mes.append("load_nrnmpi: p_cxx_char_alloc ");
            mes.append(dlerror());
            mes.append("\n");
            dlclose(handle);
            return nullptr;
        }
        *p = cxx_char_alloc;
    }

    return handle;
}

std::string nrnmpi_load() {
    std::string pmes;
    void* handle = nullptr;
    // If libmpi already in memory, find name and dlopen that.
    void* sym = dlsym(RTLD_DEFAULT, "MPI_Initialized");
    if (sym) {
        Dl_info info;
        if (dladdr(sym, &info)) {
            if (info.dli_fname[0] == '/' || strchr(info.dli_fname, ':')) {
                pmes = "<libmpi> is loaded in the sense the MPI_Initialized has an address\n";
                handle = load_mpi(info.dli_fname, pmes);
                if (handle) {
                    // Normally corenrn_mpi_library points to an
                    // libcorenrnmpi_X.{so, ...} file, why do we want it to
                    // point to libmpi.{so, ...} in this case?
                    corenrn_mpi_library = info.dli_fname;
                }
            }
        }
    }

    if (!handle) {
        // Otherwise, try to "dlopen(libmpi)", trying a few different search paths
        // that are slightly different for different platforms. MPI_LIB_NRN_PATH may
        // be set explicitly by the user, or by NEURON's Python code via
        // ctypes.find_library().
        using const_char_ptr = const char*;
        std::array libmpi_names {
#if defined(DARWIN)
            "libmpi.dylib", const_char_ptr{std::getenv("MPI_LIB_NRN_PATH")},
#elif defined(MINGW)
            "msmpi.dll"
#else  // Linux
       // libmpi.so is not standard but used by most of the implemenntation
       // (mpich, openmpi, intel-mpi, parastation-mpi, hpe-mpt) but not
       // cray-mpich. we first load libmpi and then libmpich.so as a fallaback
       // for cray systems.
            "libmpi.so", const_char_ptr{std::getenv("MPI_LIB_NRN_PATH")}, "libmpich.so"
#endif
        };

        // Look for the MPI implementation in this search path
        pmes = "Tried loading an MPI library from:\n";
        for (auto const* mpi_path: libmpi_names) {
            if (!mpi_path) {
                // MPI_LIB_NRN_PATH might not be set
                continue;
            }
            handle = load_mpi(mpi_path, pmes);
            if (handle) {
                // Success
                break;
            }
        }
    }

    if (!handle) {
        // Failed to find an MPI implementation
        pmes.append(
            "Is an MPI library such as openmpi, mpich, intel-mpi or sgi-mpt installed? If yes, it "
            "may be installed in a non-standard location that you can add to LD_LIBRARY_PATH (or "
            "DYLD_LIBRARY_PATH on macOS), or on Linux or macOS you can provide a full path in "
            "MPI_LIB_NRN_PATH\n");
        return pmes;
    }

#if !defined(DARWIN) && !defined(MINGW)
    // Linux-specific hack; with CMake the problem of Python launch on Linux not
    // resolving variables from already loaded shared libraries has returned.
    {
        std::string error{"Promoted none of"};
        auto const promote_to_global = [&error](const char* lib) {
            if (!dlopen(lib, RTLD_NOW | RTLD_NOLOAD | RTLD_GLOBAL)) {
                char const* dlerr = dlerror();
                error = error + ' ' + lib + " (" + (dlerr ? dlerr : "nullptr") + ')';
                return false;
            }
            return true;
        };
        if (!promote_to_global("libnrniv.so") && !promote_to_global("libnrniv-without-nvidia.so")) {
            Fprintf(stderr, fmt::format("{} to RTLD_GLOBAL\n", error).c_str());
        }
    }
#endif

    // Found the MPI implementation, `handle` refers to it . Now deduce which
    // MPI implementation that actually is
    assert(handle);
    auto const mpi_implementation = [handle] {
#ifdef MINGW
        return "msmpi";
#else
        if (dlsym(handle, "ompi_mpi_init")) {
            // OpenMPI
            return "ompi";
        } else if (dlsym(handle, "MPI_SGI_vtune_is_running")) {
            // Got sgi-mpt. MPI_SGI_init exists in both mpt and hmpt, so we look
            // for MPI_SGI_vtune_is_running which only exists in the non-hmpt
            // version.
            return "mpt";
        } else {
            // Assume mpich. Could check for MPID_nem_mpich_init...
            return "mpich";
        }
#endif
    }();

    // Figure out where to find lib[core]nrnmpi{...} libraries. Older versions
    // of this code used @loader_path on macOS, which caused problems in now that the code that
    // calls dlopen(libcorenrnmpi_...) is in libcorenrnmech.so (in some
    // model-specific directory) rather than the CoreNEURON installation
    // directory where libcorenrnmpi_*.so live. Now libcorenrnmpi_*.so will be
    // looked for in the same directory as libnrniv.so, which will be incorrect
    // if CoreNEURON is built externally with dynamic MPI enabled.
    auto const libnrnmpi_prefix = []() -> std::string {
#ifdef MINGW
        // Preserve old behaviour on Windows
        return {};
#else
        if (const char* nrn_home = std::getenv("NRNHOME")) {
            // TODO: what about windows path separators?
            return std::string{nrn_home} + "/lib/";
        } else {
            // Use the directory libnrniv.so is in
            return path_prefix_to_libnrniv();
        }
#endif
    }();


    auto const mpi_path = [&](std::string_view middle) {
        std::string name{libnrnmpi_prefix};
        name.append(neuron::config::shared_library_prefix);
        name.append(middle);
        name.append(mpi_implementation);
        name.append(neuron::config::shared_library_suffix);
        return name;
    };
    auto const nrn_mpi_library = mpi_path("nrnmpi_");
    corenrn_mpi_library = mpi_path("corenrnmpi_");

    // This env variable is only needed in usage like neurodamus where
    // `solve_core()` is directly called by MOD file and it doesn't have
    // an easy way to know which MPI library to load.
    // TODO: remove when BlueBrain/neurodamus/issues/17 is fixed.
#if defined(HAVE_SETENV)
    setenv("NRN_CORENRN_MPI_LIB", corenrn_mpi_library.c_str(), 0);
#endif

    if (!load_nrnmpi(nrn_mpi_library.c_str(), pmes)) {
        return pmes;
    }

    // No error, return an empty string. We have called dlopen(...) on the
    // libnrnmpi_* shared library and potentially on "libmpi" without
    // corresponding calls to dlclose().
    return {};
}

// nrnmpi_load cannot safely be called from nrnmpi.cpp because of pre/post-C++11
// ABI compatibility issues with std::string. See
// https://github.com/neuronsimulator/nrn/issues/1963 for more information.
void nrnmpi_load_or_exit() {
    auto const err = nrnmpi_load();
    if (!err.empty()) {
        Printf(fmt::format("{}\n", err).c_str());
        std::exit(1);
    }
}

#endif
