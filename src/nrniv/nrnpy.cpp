#include "nrnpy.h"
#include <../../nrnconf.h>
// For Linux and Max OS X,
// Solve the problem of not knowing what version of Python the user has by
// possibly deferring linking to libnrnpython.so to run time using the proper
// Python interface
#include <stdio.h>
#include <stdlib.h>
#include <InterViews/resource.h>
#include "nrnoc2iv.h"
#include "classreg.h"
#include "nonvintblock.h"
#include "nrnmpi.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace neuron::python {
// Declared extern in nrnpy.h, defined here.
impl_ptrs methods;
}  // namespace neuron::python
// Backwards-compatibility hack
int (*nrnpy_hoccommand_exec)(Object*);

extern int nrn_nopython;
extern std::string nrnpy_pyexe;
extern int nrn_is_python_extension;
using nrnpython_reg_real_t = void (*)(neuron::python::impl_ptrs*);
#if DARWIN
extern void nrn_possible_mismatched_arch(const char*);
#endif

#ifdef NRNPYTHON_DYNAMICLOAD
#include "nrnwrap_dlfcn.h"
extern char* neuron_home;
static nrnpython_reg_real_t load_nrnpython();
#else
extern "C" void nrnpython_reg_real(neuron::python::impl_ptrs*);
#endif

void nrnpython() {
    if (neuron::python::methods.hoc_nrnpython) {
        neuron::python::methods.hoc_nrnpython();
    } else {
        hoc_retpushx(0.);
    }
}

// Stub class for when Python does not exist
static void* p_cons(Object*) {
    return nullptr;
}
static void p_destruct(void*) {}
static Member_func p_members[] = {{nullptr, nullptr}};

#ifdef NRNPYTHON_DYNAMICLOAD
static std::string nrnpy_pylib{}, nrnpy_pyversion{};

/**
 * @brief Wrapper that executes a command and captures stdout.
 *
 * Throws std::runtime_error if the command does not execute cleanly.
 */
static std::string check_output(std::string command) {
    std::FILE* const p = popen(command.c_str(), "r");
    if (!p) {
        throw std::runtime_error("popen(" + command + ", \"r\") failed");
    }
    std::string output;
    std::array<char, 1024> buffer{};
    while (std::fgets(buffer.data(), buffer.size() - 1, p)) {
        output += buffer.data();
    }
    if (auto const code = pclose(p)) {
        std::ostringstream err;
        err << "'" << command << "' did not terminate cleanly, pclose returned non-zero (" << code
            << ") after the following output had been read:\n"
            << output;
        throw std::runtime_error(err.str());
    }
    return output;
}

// Included in C++20
static bool starts_with(std::string_view str, std::string_view prefix) {
    return str.substr(0, prefix.size()) == prefix;
}
static bool ends_with(std::string_view str, std::string_view suffix) {
    return str.size() >= suffix.size() &&
           str.substr(str.size() - suffix.size(), std::string_view::npos) == suffix;
}

/**
 * @brief Figure out which Python to load.
 *
 * When dynamic Python support is enabled, NEURON needs to figure out which
 * libpythonX.Y to load, and then load it followed by the corresponding
 * libnrnpythonX.Y. This can be steered both using commandline options and by
 * using environment variables. The logic is as follows:
 *
 * * the -pyexe argument to nrniv (special) takes precedence over NRN_PYTHONEXE
 *   and we have to assume that the other NRN_PY* environment variables are not
 *   compatible with it and use nrnpyenv.sh to find compatible values for them,
 *   i.e. -pyexe implies that NRN_PY* are ignored.
 * * if -pyexe is *not* passed, then we examine the NRN_PY* environment
 *   variables:
 *   * if all of them are set, nrnpyenv.sh is not run and they are assumed to
 *     form a coherent set
 *   * if only some, or none, of them are set, nrnpyenv.sh is run to fill in
 *     the missing values. NRN_PYTHONEXE is an input to nrnpyenv.sh, so if this
 *     is set then we will not search $PATH
 */
static void set_nrnpylib() {
    std::array<std::pair<std::string&, const char*>, 3> params{
        {{nrnpy_pylib, "NRN_PYLIB"},
         {nrnpy_pyexe, "NRN_PYTHONEXE"},
         {nrnpy_pyversion, "NRN_PYTHONVERSION"}}};
    auto const all_set = [&params]() {
        return std::all_of(params.begin(), params.end(), [](auto const& p) {
            return !p.first.empty();
        });
    };
    if (nrnpy_pyexe.empty()) {
        // -pyexe was not passed, read from the environment
        for (auto& [glob_var, env_var]: params) {
            if (const char* v = std::getenv(env_var)) {
                glob_var = v;
            }
        }
        if (all_set()) {
            // the environment specified everything, nothing more to do
            return;
        }
    }
    // Populate missing values using nrnpyenv.sh. Pass the possibly-null value of nrnpy_pyexe, which
    // may have come from -pyexe or NRN_PYTHONEXE, to nrnpyenv.sh. Do all of this on rank 0, and
    // broadcast the results to other ranks afterwards.
    if (nrnmpi_myid_world == 0) {
        // Construct a command to execute
        auto const command = []() -> std::string {
#ifdef MINGW
            std::string bnrnhome{neuron_home}, fnrnhome{neuron_home};
            std::replace(bnrnhome.begin(), bnrnhome.end(), '/', '\\');
            std::replace(fnrnhome.begin(), fnrnhome.end(), '\\', '/');
            return bnrnhome + R"(\mingw\usr\bin\bash )" + fnrnhome + "/bin/nrnpyenv.sh " +
                   nrnpy_pyexe + " --NEURON_HOME=" + fnrnhome;
#else
            return "bash " + std::string{neuron_home} + "/../../bin/nrnpyenv.sh " + nrnpy_pyexe;
#endif
        }();
        // Execute the command, capture its stdout and wrap that in a C++ stream. This will throw if
        // the commnand fails.
        std::istringstream cmd_stdout{check_output(command)};
        std::string line;
        // if line is of the form:
        // export FOO="bar"
        // then proc_line(x, "FOO") sets x to bar
        auto const proc_line = [](std::string_view line, auto& glob_var, std::string_view env_var) {
            std::string_view const suffix{"\""};
            auto const prefix = "export " + std::string{env_var} + "=\"";
            if (starts_with(line, prefix) && ends_with(line, suffix)) {
                line.remove_prefix(prefix.size());
                line.remove_suffix(suffix.size());
                if (!glob_var.empty() && glob_var != line) {
                    std::cout << "WARNING: overriding " << env_var << '=' << glob_var << " with "
                              << line << std::endl;
                }
                glob_var = line;
            }
        };
        // Process the output of nrnpyenv.sh line by line
        while (std::getline(cmd_stdout, line)) {
            for (auto& [glob_var, env_var]: params) {
                proc_line(line, glob_var, env_var);
            }
        }
        // After having run nrnpyenv.sh, we should know everything about the Python library that is
        // to be loaded.
        if (!all_set()) {
            std::ostringstream err;
            err << "After running nrnpyenv.sh (" << command << ") with output:\n"
                << cmd_stdout.str()
                << "\nwe are still missing information about the Python to be loaded:\n"
                << "  nrnpy_pyexe=" << nrnpy_pyexe << '\n'
                << "  nrnpy_pylib=" << nrnpy_pylib << '\n'
                << "  nrnpy_pyversion=" << nrnpy_pyversion << '\n';
            throw std::runtime_error(err.str());
        }
    }
#if NRNMPI
    if (nrnmpi_numprocs_world > 1) {  // 0 broadcasts to everyone else.
        nrnmpi_str_broadcast_world(nrnpy_pyexe, 0);
        nrnmpi_str_broadcast_world(nrnpy_pylib, 0);
        nrnmpi_str_broadcast_world(nrnpy_pyversion, 0);
    }
#endif
}
#endif

/**
 * @brief Load + register an nrnpython library for a specific Python version.
 *
 * This finds the library (if needed because dynamic Python is enabled), opens it and gets + calls
 * its nrnpython_reg_real method. This ensures that NEURON's global state knows about a Python
 * implementation.
 */
void nrnpython_reg() {
    nrnpython_reg_real_t reg_fn{};
#if USE_PYTHON
    if (!nrn_nopython) {
#ifdef NRNPYTHON_DYNAMICLOAD
        void* handle{};
        if (!nrn_is_python_extension) {
            // find the details of the libpythonX.Y.so we are going to load.
            try {
                set_nrnpylib();
            } catch (std::exception const& e) {
                std::cerr << "Could not determine Python library details: " << e.what()
                          << std::endl;
                exit(1);
            }
            handle = dlopen(nrnpy_pylib.c_str(), RTLD_NOW | RTLD_GLOBAL);
            if (!handle) {
                std::cerr << "Could not dlopen NRN_PYLIB: " << nrnpy_pylib << std::endl;
#if DARWIN
                nrn_possible_mismatched_arch(nrnpy_pylib.c_str());
#endif
                exit(1);
            }
        }
        if (handle || nrn_is_python_extension) {
            // Load libnrnpython.X.Y.so
            reg_fn = load_nrnpython();
        }
#else
        // Python enabled, but not dynamic
        reg_fn = nrnpython_reg_real;
#endif
    }
    if (reg_fn) {
        // Register Python-specific things in the NEURON global state
        reg_fn(&neuron::python::methods);
        // Compatibility hack for legacy MOD file in nrntest
        nrnpy_hoccommand_exec = neuron::python::methods.hoccommand_exec;
        return;
    }
#endif
    // Stub implementation of PythonObject if Python support was not enabled, or a nrnpython library
    // could not be loaded.
    class2oc("PythonObject", p_cons, p_destruct, p_members, nullptr, nullptr, nullptr);
}

#ifdef NRNPYTHON_DYNAMICLOAD  // to end of file
static nrnpython_reg_real_t load_nrnpython() {
    std::string pyversion{};
    if (auto const pv10 = nrn_is_python_extension; pv10 > 0) {
        // pv10 is one of the packed integers like 310 (3.10) or 38 (3.8)
        auto const factor = (pv10 >= 100) ? 100 : 10;
        pyversion = std::to_string(pv10 / factor) + "." + std::to_string(pv10 % factor);
    } else {
        if (nrnpy_pylib.empty() || nrnpy_pyversion.empty()) {
            std::cerr << "Do not know what Python to load [nrnpy_pylib=" << nrnpy_pylib
                      << " nrnpy_pyversion=" << nrnpy_pyversion << ']' << std::endl;
            return nullptr;
        }
        pyversion = nrnpy_pyversion;
        // It's possible to get this far with an incompatible version, if nrnpy_pyversion and
        // friends were set from the environment to bypass nrnpyenv.sh, and nrniv -python was
        // launched.
        auto const& supported_versions = neuron::config::supported_python_versions;
        auto const iter =
            std::find(supported_versions.begin(), supported_versions.end(), pyversion);
        if (iter == supported_versions.end()) {
            std::cerr << "Python " << pyversion
                      << " is not supported by this NEURON installation (supported:";
            for (auto const& good_ver: supported_versions) {
                std::cerr << ' ' << good_ver;
            }
            std::cerr << "). If you are seeing this message, your environment probably contains "
                         "NRN_PYLIB, NRN_PYTHONEXE and NRN_PYTHONVERSION settings that are "
                         "incompatible with this NEURON. Try unsetting them."
                      << std::endl;
            return nullptr;
        }
    }
    // Construct libnrnpythonX.Y.so (or other platforms' equivalent)
    std::string name;
    name.append(neuron::config::shared_library_prefix);
    name.append("nrnpython");
    name.append(pyversion);
    name.append(neuron::config::shared_library_suffix);
#ifndef MINGW
    // Build a path from neuron_home on macOS and Linux
    name = neuron_home + ("/../../lib/" + name);
#endif
    auto* const handle = dlopen(name.c_str(), RTLD_NOW);
    if (!handle) {
        std::cerr << "Could not load " << name << std::endl;
        std::cerr << "nrn_is_python_extension=" << nrn_is_python_extension << std::endl;
        return nullptr;
    }
    auto* const reg = reinterpret_cast<nrnpython_reg_real_t>(dlsym(handle, "nrnpython_reg_real"));
    if (!reg) {
        std::cerr << "Could not load registration function from " << name << std::endl;
    }
    return reg;
}
#endif
