#include "nrnpy.h"
#include <../../nrnconf.h>
// Which Python NEURON runs: the version the user has is not known until run time.
#include <stdio.h>
#include <stdlib.h>
#include <InterViews/resource.h>
#include "nrnoc2iv.h"
#include "classreg.h"
#include "nonvintblock.h"
#include "nrnmpi.h"
#include "nrnpy_launch.h"
#include "nrnpy_object.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>
#if defined(NRNPYTHON_DYNAMICLOAD) && defined(_WIN32) && !defined(MINGW)
#include <windows.h>
#include "../oc/nrn_windows_home.hpp"
#endif

namespace neuron::python {
// Declared extern in nrnpy.h, defined here.
impl_ptrs methods;
}  // namespace neuron::python
// Backwards-compatibility hack
NRN_DLLSYM int (*nrnpy_hoccommand_exec)(Object*);

extern int nrn_nopython;
extern std::string nrnpy_pyexe;
#if DARWIN
extern void nrn_possible_mismatched_arch(const char*);
#endif

#ifdef NRNPYTHON_DYNAMICLOAD
#include "nrnwrap_dlfcn.h"
extern char* neuron_home;
#endif

void nrnpython() {
    if (neuron::python::methods.hoc_nrnpython) {
        neuron::python::methods.hoc_nrnpython();
    } else {
        hoc_retpushx(0.);
    }
}

#ifdef NRNPYTHON_DYNAMICLOAD
static std::string nrnpy_pylib{}, nrnpy_pyversion{};

/**
 * @brief Wrapper that executes a command and captures stdout.
 *
 * Throws std::runtime_error if the command does not execute cleanly.
 */
static std::string check_output(std::string command) {
    // MSVC CRT: POSIX popen/pclose are deprecated aliases of _popen/_pclose.
#if defined(_MSC_VER)
    std::FILE* const p = _popen(command.c_str(), "r");
#else
    std::FILE* const p = popen(command.c_str(), "r");
#endif
    if (!p) {
        throw std::runtime_error("popen(" + command + ", \"r\") failed");
    }
    std::string output;
    std::array<char, 1024> buffer{};
    while (std::fgets(buffer.data(), buffer.size() - 1, p)) {
        output += buffer.data();
    }
#if defined(_MSC_VER)
    if (auto const code = _pclose(p)) {
#else
    if (auto const code = pclose(p)) {
#endif
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

// MSVC _popen uses cmd.exe /c. A command that starts with " loses its first and
// last quote. Extra-wrap, same as the nrnpyenv.py launch below. POSIX popen
// keeps the quotes.
static std::string popen_command(std::string command) {
#if defined(_MSC_VER)
    if (!command.empty() && command.front() == '"') {
        command = "\"" + command + "\"";
    }
#endif
    return command;
}

#if defined(_WIN32) && !defined(MINGW)
// Error text only. The launch itself stays wchar_t.
static std::string wide_to_utf8(std::wstring const& w) {
    if (w.empty()) {
        return {};
    }
    int const n = WideCharToMultiByte(
        CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    if (n <= 0) {
        return {};
    }
    std::string s(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(
        CP_UTF8, 0, w.data(), static_cast<int>(w.size()), &s[0], n, nullptr, nullptr);
    return s;
}

static std::wstring acp_to_wide(std::string const& s) {
    if (s.empty()) {
        return {};
    }
    int const n = MultiByteToWideChar(CP_ACP, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (n <= 0) {
        return {};
    }
    std::wstring w(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_ACP, 0, s.data(), static_cast<int>(s.size()), &w[0], n);
    return w;
}

// _popen is the ANSI CRT spawn. A nrnpyenv.py path outside the active code
// page is not a char*. _wpopen is the wide form of that same API. The caller
// extra-wraps a command that starts with a quote (cmd.exe /c strips one pair).
static std::string check_output_w(std::wstring const& command) {
    std::FILE* const p = _wpopen(command.c_str(), L"r");
    if (!p) {
        throw std::runtime_error("_wpopen(" + wide_to_utf8(command) + ") failed");
    }
    std::string output;
    std::array<char, 1024> buffer{};
    while (std::fgets(buffer.data(), buffer.size() - 1, p)) {
        output += buffer.data();
    }
    if (auto const code = _pclose(p)) {
        std::ostringstream err;
        err << "'" << wide_to_utf8(command)
            << "' did not terminate cleanly, pclose returned non-zero (" << code
            << ") after the following output had been read:\n"
            << output;
        throw std::runtime_error(err.str());
    }
    return output;
}
#endif

// nrnpyenv.sh rejects non-CPython and a free-threaded build while it is choosing an
// interpreter. When NRN_PYLIB, NRN_PYTHONEXE, and NRN_PYTHONVERSION are already set,
// that script does not run. Check the executable before dlopen. -S skips site, so a
// sitecustomize print is not part of the answer.
static void ensure_cpython(std::string const& pyexe) {
    std::string command = popen_command("\"" + pyexe +
                                        "\" -S -c \"import sys; print(sys.implementation.name)\"");
    std::string name;
    try {
        name = check_output(std::move(command));
    } catch (std::exception const& e) {
        throw std::runtime_error("Could not run '" + pyexe + "' to check the interpreter.\n" +
                                 e.what());
    }
    while (!name.empty() && (name.back() == '\n' || name.back() == '\r')) {
        name.pop_back();
    }
    if (name != "cpython") {
        throw std::runtime_error(
            "This NEURON installation supports CPython only (this interpreter is " + name + ").");
    }
}

// Print 0 or 1 and exit 0. check_output throws on any other status, so a failed
// launch stays "Could not run" and is not reported as free-threaded. 1 and "1"
// are a free-threaded build. 0, "0", and None are a GIL build.
static void ensure_gil_build(std::string const& pyexe) {
    std::string command = popen_command("\"" + pyexe +
                                        "\" -S -c \"import sysconfig; v = sysconfig.get_config_var"
                                        "('Py_GIL_DISABLED'); print(1 if v in (1, '1') else 0)\"");
    std::string answer;
    try {
        answer = check_output(std::move(command));
    } catch (std::exception const& e) {
        throw std::runtime_error("Could not run '" + pyexe +
                                 "' to check whether the build is free-threaded.\n" + e.what());
    }
    while (!answer.empty() && (answer.back() == '\n' || answer.back() == '\r')) {
        answer.pop_back();
    }
    if (answer == "1") {
        throw std::runtime_error(
            "This NEURON installation does not support a free-threaded CPython build "
            "(Py_GIL_DISABLED).");
    }
    if (answer != "0") {
        throw std::runtime_error("Could not read Py_GIL_DISABLED from '" + pyexe + "' (got '" +
                                 answer + "').");
    }
}

/**
 * @brief Figure out which Python to load.
 *
 * When dynamic Python support is enabled, NEURON needs to figure out which
 * libpythonX.Y to load. This can be steered both using commandline options and
 * by using environment variables. The logic is as follows:
 *
 * * the -pyexe argument to nrniv (special) takes precedence over NRN_PYTHONEXE
 *   and we have to assume that the other NRN_PY* environment variables are not
 *   compatible with it and use nrnpyenv.sh to find compatible values for them,
 *   i.e. -pyexe implies that NRN_PY* are ignored.
 * * if -pyexe is *not* passed, then we examine the NRN_PY* environment
 *   variables:
 *   * if all of them are set, nrnpyenv.sh is not run. The executable must be
 *     CPython, and Py_GIL_DISABLED must not say the build is free-threaded.
 *     The three values are otherwise assumed to form a coherent set
 *   * if only some, or none, of them are set, nrnpyenv.sh (Unix/MinGW) or
 *     nrnpyenv.py (MSVC Windows) is run to fill in the missing values.
 *     NRN_PYTHONEXE is an input to those helpers, so if this is set then we
 *     will not search $PATH
 * * At startup all ranks call this (collective): rank 0 finds out and tells the others. A rank that
 *   first needs Python later finds out for itself.
 * * An absolute path from -pyexe or NRN_PYTHONEXE is kept as the program name.
 *   The script still supplies the library and the version. The probe's
 *   sys.executable replaces the path only when NEURON searched for the
 *   interpreter, or the supplied path is not absolute. Replacing an absolute
 *   path would change the spelling of sys.executable (macOS /tmp and
 *   /private/tmp) and the directory Python searches for pyvenv.cfg.
 */
static void set_nrnpylib(bool collective) {
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
            // the environment specified everything, so nrnpyenv.sh will not run
            ensure_cpython(nrnpy_pyexe);
            ensure_gil_build(nrnpy_pyexe);
            return;
        }
    }
    // See the comment above. Rank 0 is the only one that has read -pyexe or the
    // environment; the broadcast below publishes the kept path.
    bool const keep_supplied_pyexe = !nrnpy_pyexe.empty() &&
                                     std::filesystem::path{nrnpy_pyexe}.is_absolute();
    // Populate missing values using nrnpyenv.sh (Unix/MinGW) or nrnpyenv.py
    // (MSVC: no MinGW bash; Git bash still needs cygcheck). Pass the
    // possibly-null value of nrnpy_pyexe, which may have come from -pyexe or
    // NRN_PYTHONEXE. Do all of this on rank 0, and broadcast afterwards.
    if (!collective || nrnmpi_myid_world == 0) {
        // Construct a command to execute
#if defined(_WIN32) && !defined(MINGW)
        std::wstring command_w;
#endif
        auto const command = [&]() -> std::string {
#ifdef MINGW
            std::string bnrnhome{neuron_home}, fnrnhome{neuron_home};
            std::replace(bnrnhome.begin(), bnrnhome.end(), '/', '\\');
            std::replace(fnrnhome.begin(), fnrnhome.end(), '\\', '/');
            return bnrnhome + R"(\mingw\usr\bin\bash )" + fnrnhome + "/bin/nrnpyenv.sh " +
                   nrnpy_pyexe + " --NEURON_HOME=" + fnrnhome;
#elif defined(_WIN32)
            // pythonXY.dll is a Windows ABI fact; do not require bash/cygcheck.
            auto quote = [](std::wstring const& s) { return L"\"" + s + L"\""; };
            // nrnpyenv.py is installed next to nrniv.exe. Unix uses
            // neuron_home/../../bin (neuron_home is prefix/share/nrn). Windows
            // setneuronhome is two dirs up from the EXE (the prefix), so that
            // relative path misses the wheel. The path is wide: a long or
            // non-ACP install does not fit GetModuleFileNameA's MAX_PATH.
            auto const exe = nrn_win_module_path(nullptr);
            if (exe.empty()) {
                throw std::runtime_error("GetModuleFileNameW failed for nrnpyenv.py");
            }
            std::wstring const script = (exe.parent_path() / L"nrnpyenv.py").wstring();
            std::wstring const py = nrnpy_pyexe.empty() ? std::wstring{L"python"}
                                                        : acp_to_wide(nrnpy_pyexe);
            if (py.empty()) {
                throw std::runtime_error("NRN_PYTHONEXE is not a valid ANSI path");
            }
            // MSVC _wpopen uses cmd.exe /c. If the command starts with ", cmd
            // strips the first and last quote, so "python" "script.py" becomes
            // python" "script.py. Extra-wrap the command.
            command_w = quote(quote(py) + L" " + quote(script));
            return wide_to_utf8(command_w);
#else
            return "bash " + std::string{neuron_home} + "/../../bin/nrnpyenv.sh " + nrnpy_pyexe;
#endif
        }();
        // Execute the command, capture its stdout and wrap that in a C++ stream. This will throw if
        // the commnand fails.
#if defined(_WIN32) && !defined(MINGW)
        std::istringstream cmd_stdout{check_output_w(command_w)};
#else
        std::istringstream cmd_stdout{check_output(command)};
#endif
        std::string line;
        // if line is of the form:
        // export FOO="bar"
        // then proc_line(x, "FOO") sets x to bar
        auto const proc_line =
            [&](std::string_view line, auto& glob_var, std::string_view env_var) {
                std::string_view const suffix{"\""};
                auto const prefix = "export " + std::string{env_var} + "=\"";
                if (starts_with(line, prefix) && ends_with(line, suffix)) {
                    line.remove_prefix(prefix.size());
                    line.remove_suffix(suffix.size());
                    if (keep_supplied_pyexe && env_var == "NRN_PYTHONEXE") {
                        return;
                    }
                    if (!glob_var.empty() && glob_var != line) {
                        Printf(fmt::format(
                                   "WARNING: overriding {} = {} with {}\n", env_var, glob_var, line)
                                   .c_str());
                    }
                    glob_var = line;
                }
            };
        // Process the output of nrnpyenv.sh / nrnpyenv.py line by line
        while (std::getline(cmd_stdout, line)) {
            for (auto& [glob_var, env_var]: params) {
                proc_line(line, glob_var, env_var);
            }
        }
        // After having run the helper, we should know everything about the Python library that is
        // to be loaded.
        if (!all_set()) {
            std::ostringstream err;
            err << "After running nrnpyenv (" << command << ") with output:\n"
                << cmd_stdout.str()
                << "\nwe are still missing information about the Python to be loaded:\n"
                << "  nrnpy_pyexe=" << nrnpy_pyexe << '\n'
                << "  nrnpy_pylib=" << nrnpy_pylib << '\n'
                << "  nrnpy_pyversion=" << nrnpy_pyversion << '\n';
            throw std::runtime_error(err.str());
        }
    }
#if NRNMPI
    if (collective && nrnmpi_numprocs_world > 1) {  // 0 broadcasts to everyone else.
        nrnmpi_str_broadcast_world(nrnpy_pyexe, 0);
        nrnmpi_str_broadcast_world(nrnpy_pylib, 0);
        nrnmpi_str_broadcast_world(nrnpy_pyversion, 0);
    }
#endif
}
#endif

#if USE_PYTHON
#ifdef NRNPYTHON_DYNAMICLOAD
// The compiled modules of the neuron package (nmodl, rxd's geometry) need one of these.
static bool supported(std::string const& pyversion) {
    if (!neuron::config::python_limited_api) {
        auto const& versions = neuron::config::supported_python_versions;
        return std::find(versions.begin(), versions.end(), pyversion) != versions.end();
    }
    int major{}, minor{};
    // The limited API is the CPython 3 stable ABI. A future major is not covered.
    return std::sscanf(pyversion.c_str(), "%d.%d", &major, &minor) == 2 && major == 3 &&
           minor >= 12;
}

static bool pylib_known{};

void* neuron::python::nrnpy_open_libpython() {
    if (!pylib_known) {
        set_nrnpylib(false);
        pylib_known = true;
    }
    if (!supported(nrnpy_pyversion)) {
        std::ostringstream err;
        err << "Python " << nrnpy_pyversion << " is not supported by this NEURON installation (";
        if (neuron::config::python_limited_api) {
            err << "requires CPython 3.12 or a newer 3.x minor, with the GIL";
        } else {
            err << "supported:";
            for (auto const& version: neuron::config::supported_python_versions) {
                err << ' ' << version;
            }
        }
        err << "). If you set NRN_PYLIB, NRN_PYTHONEXE or NRN_PYTHONVERSION, try unsetting them.";
        throw std::runtime_error(err.str());
    }
    auto* const handle = dlopen(nrnpy_pylib.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
#if DARWIN
        nrn_possible_mismatched_arch(nrnpy_pylib.c_str());
#endif
        throw std::runtime_error("Could not dlopen NRN_PYLIB: " + nrnpy_pylib);
    }
    return handle;
}
#else
void* neuron::python::nrnpy_open_libpython() {
    return nullptr;
}
#endif
#endif

void nrnpython_reg() {
#if USE_PYTHON
    neuron::python::register_python_object();
    if (nrn_nopython) {
        return;
    }
    neuron::python::register_launcher();
#if NRNMPI && defined(NRNPYTHON_DYNAMICLOAD)
    if (nrnmpi_numprocs_world > 1) {
        // Only some ranks may ever use Python, and rank 0 tells them which.
        try {
            set_nrnpylib(true);
            pylib_known = true;
        } catch (std::exception const& e) {
            Fprintf(stderr, "Could not determine Python library details: %s\n", e.what());
            exit(1);
        }
    }
#endif
#else
    class2oc(
        "PythonObject",
        [](Object*) -> void* { return nullptr; },
        [](void*) {},
        nullptr,
        nullptr,
        nullptr);
#endif
}
