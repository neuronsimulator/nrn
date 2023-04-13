#include <nrnpython.h>
#include <nrnpy_utils.h>
#include "oc_ansi.h"
#include <stdio.h>
#include <InterViews/resource.h>
#if HAVE_IV
#include <InterViews/session.h>
#endif
#include <nrnoc2iv.h>
#include <hoccontext.h>
#include <ocfile.h>  // bool isDirExist(const std::string& path);

#include <hocstr.h>
#include "nrn_filesystem.h"

#include <string>
#include <sstream>
extern HocStr* hoc_cbufstr;
extern int nrnpy_nositeflag;
extern char* nrnpy_pyhome;
extern char* nrnpy_pyexe;
extern char* hoc_ctp;
extern FILE* hoc_fin;
extern const char* hoc_promptstr;
extern char* neuronhome_forward();
#if DARWIN || defined(__linux__)
extern const char* path_prefix_to_libnrniv();
#endif
static char* nrnpython_getline(FILE*, FILE*, const char*);
extern int nrn_global_argc;
extern char** nrn_global_argv;
int nrnpy_pyrun(const char*);
extern int (*p_nrnpy_pyrun)(const char*);

static std::string python_sys_path_to_append() {
    std::string path{neuronhome_forward()};
    if (path.empty()) {
        return {};
    }
#if defined(__linux__) || defined(DARWIN)
    // If /where/installed/lib/python/neuron exists, then append to sys.path
    path = path_prefix_to_libnrniv();
#else   // not defined(__linux__) || defined(DARWIN)
    path += "/lib/";
#endif  // not defined(__linux__) || defined(DARWIN)
    path += "python";
    if (isDirExist(path + "/neuron")) {
        return path;
    }
    return {};
}

namespace {
struct PythonConfigWrapper {
    PythonConfigWrapper() {
        PyConfig_InitPythonConfig(&config);
    }
    ~PythonConfigWrapper() {
        PyConfig_Clear(&config);
    }
    operator PyConfig*() {
        return &config;
    }
    PyConfig* operator->() {
        return &config;
    }
    PyConfig config;
};
struct PyMem_RawFree_Deleter {
    void operator()(wchar_t* ptr) const {
        PyMem_RawFree(ptr);
    }
};
PyObject* basic_sys_path{};

/**
 * @brief Reset sys.path to be basic_sys_path and prepend something.
 * @param new_first Path to decode and prepend to sys.path.
 */
void reset_sys_path(std::string_view new_first) {
    PyLockGIL _{};
    auto* const path = PySys_GetObject("path");
    nrn_assert(path);
    // Clear sys.path
    nrn_assert(PyList_SetSlice(path, 0, PyList_Size(path), nullptr) != -1);
    // Decode new_first and make a Python unicode string out of it
    auto* const ustr = PyUnicode_DecodeFSDefaultAndSize(new_first.data(), new_first.size());
    nrn_assert(ustr);
    // Put the decoded string into sys.path
    nrn_assert(PyList_Insert(path, 0, ustr) == 0);
    // Append basic_sys_path to sys.path
    assert(basic_sys_path && PyTuple_Check(basic_sys_path));
    nrn_assert(PySequence_SetSlice(path, 1, 1 + PyTuple_Size(basic_sys_path), basic_sys_path) == 0);
}
}  // namespace

/**
 * @brief Execute a Python script.
 * @return 0 on failure, 1 on success.
 */
int nrnpy_pyrun(const char* fname) {
    // Figure out what sys.path[0] should be; this involves first resolving symlinks in fname and
    // second getting the directory name from it.
    auto const realpath = neuron::std::filesystem::canonical(fname);
    // .string() ensures this is not a wchar_t string on Windows
    auto const dirname = realpath.parent_path().string();
    reset_sys_path(dirname);
#ifdef MINGW
    // perhaps this should be the generic implementation
    auto const sz = strlen(fname) + 40;
    char* cmd = new char[sz];
    std::snprintf(cmd, sz, "exec(open(\"%s\").read(), globals())", fname);
    int err = PyRun_SimpleString(cmd);
    delete[] cmd;
    if (err != 0) {
        PyErr_Print();
        PyErr_Clear();
        return 0;
    }
    return 1;
#else   // MINGW not defined
    FILE* fp = fopen(fname, "r");
    if (fp) {
        int const code = PyRun_AnyFile(fp, fname);
        fclose(fp);
        return !code;
    } else {
        std::cerr << "Could not open " << fname << std::endl;
        return 0;
    }
#endif  // MINGW not defined
}

extern PyObject* nrnpy_hoc();
extern PyObject* nrnpy_nrn();

/** @brief Start the Python interpreter.
 *  @arg b Mode of operation, can be 0 (finalize), 1 (initialize),
 *         or 2 (execute commands/scripts)
 *  @return 0 on success, non-zero on error
 *
 *  There is an internal state variable that stores whether or not Python has
 *  been initialized. Mode 1 only has an effect if Python is not initialized,
 *  while the other modes only take effect if Python is already initialized.
 */
extern "C" int nrnpython_start(int b) {
#if USE_PYTHON
    static int started = 0;
    // printf("nrnpython_start %d started=%d\n", b, started);
    if (b == 1 && !started) {
        p_nrnpy_pyrun = nrnpy_pyrun;
        if (nrnpy_nositeflag) {
            Py_NoSiteFlag = 1;
        }
        // Create a Python configuration, see
        // https://docs.python.org/3.8/c-api/init_config.html#python-configuration, so that
        // {nrniv,special} -python behaves as similarly as possible to python. In particular this
        // affects locale coercion. Under some circumstances Python does not straightforwardly
        // handle settings like LC_ALL=C, so using a different configuration can lead to surprising
        // differences.
        PythonConfigWrapper config;
        // nrnpy_pyhome hopefully holds the python base root and should
        // work with virtual environments.
        // But use only if not overridden by the PYTHONHOME environment variable.
        char* _p_pyhome = getenv("PYTHONHOME");
        if (!_p_pyhome) {
            _p_pyhome = nrnpy_pyhome;
        }
        auto const check = [](const char* desc, PyStatus status) {
            if (PyStatus_Exception(status)) {
                std::ostringstream oss;
                oss << desc;
                if (status.err_msg) {
                    oss << ": " << status.err_msg;
                    if (status.func) {
                        oss << " in " << status.func;
                    }
                }
                throw std::runtime_error(oss.str());
            }
        };
        // Virtual environments are discovered by Python by looking for pyvenv.cfg in the directory
        // above sys.executable (https://docs.python.org/3/library/site.html), so we want to make
        // sure that sys.executable is the path to a reasonable choice of Python executable. If we
        // were to let sys.executable be `/some/path/to/arch/special` then we pick up a surprising
        // dependency on whether or not `nrnivmodl` happened to be run in the root directory of the
        // virtual environment
        const char* pyexe{nrnpy_pyexe};
#ifndef NRNPYTHON_DYNAMICLOAD
        // In non-dynamic builds, the -pyexe option has no effect on which Python is linked and
        // used, but it can be used to change PyConfig.program_name. If -pyexe is not passed then
        // we use the Python that was discovered at build time. We have to make an std::string
        // because Python's API requires the null terminator.
        std::string default_python{neuron::config::default_python_executable};
        if (!pyexe && !default_python.empty()) {
            // -pyexe was not passed
            pyexe = default_python.c_str();  // need the null terminator for Python
        }
#endif
        if (!pyexe) {
            throw std::runtime_error("Do not know what to set PyConfig.program_name to");
        }
        // Surprisingly, given the documentation, it seems that passing a non-absolute path to
        // PyConfig.program_name does not lead to a lookup in $PATH, but rather to the real (nrniv)
        // path being placed in sys.executable -- at least on macOS.
        if (auto p = neuron::std::filesystem::path{pyexe}; !p.is_absolute()) {
            throw std::runtime_error(
                "Setting PyConfig.program_name to a non-absolute path is not portable; try passing "
                "an absolute path to -pyexe");
        }
        // TODO: in non-dynamic builds then -pyexe cannot change the used Python version, and `nrniv
        // -pyexe /path/to/python3.10 -python` may well not use Python 3.10 at all. Should we do
        // something about that?
        check("Could not set PyConfig.program_name",
              PyConfig_SetBytesString(config, &config->program_name, pyexe));
        if (_p_pyhome) {
            // Py_SetPythonHome is deprecated in Python 3.11+, write to config.home instead.
            // olupton 2023-04-11 is not sure if this is still needed or useful
            check("Could not set PyConfig.home",
                  PyConfig_SetBytesString(config, &config->home, _p_pyhome));
        }
        // PySys_SetArgv is deprecated in Python 3.11+, write to config.XXX instead.
        // nrn_global_argv contains the arguments passed to nrniv/special, which are not valid
        // Python arguments, so tell Python not to try and parse them. In future we might like to
        // remove the NEURON-specific arguments and pass whatever is left to Python?
        config->parse_argv = 0;
        check("Could not set PyConfig.argv",
              PyConfig_SetBytesArgv(config, nrn_global_argc, nrn_global_argv));
        // Initialise Python
        check("Could not initialise Python", Py_InitializeFromConfig(config));
        // Manipulate sys.path, starting from the default values
        {
            PyLockGIL _{};
            auto* const sys_path = PySys_GetObject("path");
            if (!sys_path) {
                throw std::runtime_error("Could not get sys.path from C++");
            }
            // Append a path to sys.path based on where libnrniv.so is, if it's not already there.
            // Note that this is magic that is specific to launching via nrniv/special and not
            // Python, which is unfortunate for consistency...
            if (auto const path = python_sys_path_to_append(); !path.empty()) {
                auto* ustr = PyUnicode_DecodeFSDefaultAndSize(path.c_str(), path.size());
                assert(ustr);
                auto const already_there = PySequence_Contains(sys_path, ustr);
                assert(already_there != -1);
                if (already_there == 0 && PyList_Append(sys_path, ustr)) {
                    // TODO need to cover this without breaking sys.path consistency tests
                    throw std::runtime_error("Could not append " + path + " to sys.path");
                }
            }
            // To match regular Python, we should also prepend an entry to sys.path:
            // from https://docs.python.org/3/library/sys.html#sys.path:
            // * python -m module command line: prepend the current working directory.
            // * python script.py command line: prepend the script’s directory. If
            //   it's a symbolic link, resolve symbolic links.
            // * python -c code and python (REPL) command lines: prepend an empty
            //   string, which means the current working directory.
            // We only find out later what we are going to do, so for the moment we just save a copy
            // of sys.path and then restore + modify a copy of it before each script or command we
            // execute.
            assert(PyList_Check(sys_path) && !basic_sys_path);
            basic_sys_path = PyList_AsTuple(sys_path);
        }
#ifdef NRNPYTHON_DYNAMICLOAD
        // return from Py_Initialize means there was no site problem
        nrnpy_site_problem = 0;
#endif
        started = 1;
        nrnpy_hoc();
        nrnpy_nrn();
    }
    if (b == 0 && started) {
        PyGILState_STATE gilsav = PyGILState_Ensure();
        assert(basic_sys_path);
        Py_DECREF(basic_sys_path);
        basic_sys_path = nullptr;
        Py_Finalize();
        // because of finalize, no PyGILState_Release(gilsav);
        started = 0;
    }
    if (b == 2 && started) {
        // There used to be a call to PySys_SetArgv here, which dates back to
        // e48d933e03b5c25a454e294deea55e399f8ba1b1 and a comment about sys.argv not being set with
        // nrniv -python. Today, it seems like this is not needed any more.
#if !defined(MINGW)
        // cannot get this to avoid crashing with MINGW
        PyOS_ReadlineFunctionPointer = nrnpython_getline;
#endif
        // Is there a -c "command" or file.py arg.
        bool python_error_encountered{false}, have_reset_sys_path{false};
        for (int i = 1; i < nrn_global_argc; ++i) {
            char* arg = nrn_global_argv[i];
            if (strcmp(arg, "-c") == 0 && i + 1 < nrn_global_argc) {
                // sys.path[0] should be an empty string for -c
                reset_sys_path("");
                have_reset_sys_path = true;
                if (PyRun_SimpleString(nrn_global_argv[i + 1])) {
                    python_error_encountered = true;
                }
                break;
            } else if (strlen(arg) > 3 && strcmp(arg + strlen(arg) - 3, ".py") == 0) {
                if (!nrnpy_pyrun(arg)) {
                    python_error_encountered = true;
                }
                have_reset_sys_path = true;  // inside nrnpy_pyrun
                break;
            }
        }
        // python_error_encountered dictates whether NEURON will exit with a nonzero
        // code. In noninteractive/batch mode that happens immediately, in
        // interactive mode then we start a Python interpreter first.
        if (nrn_istty_) {
            if (!have_reset_sys_path) {
                // sys.path[0] should be 0 for interactive use, but if we're dropping into an
                // interactive shell after executing something else then we don't want to mess with
                // it.
                reset_sys_path("");
            }
            PyRun_InteractiveLoop(hoc_fin, "stdin");
        }
        return python_error_encountered;
    }
#endif
    return 0;
}

extern "C" void nrnpython_real() {
    int retval = 0;
#if USE_PYTHON
    HocTopContextSet
    {
        PyLockGIL lock;
        retval = PyRun_SimpleString(gargstr(1)) == 0;
    }
    HocContextRestore
#endif
    hoc_retpushx(double(retval));
}

static char* nrnpython_getline(FILE*, FILE*, const char* prompt) {
    hoc_cbufstr->buf[0] = '\0';
    hoc_promptstr = prompt;
    int r = hoc_get_line();
    // printf("r=%d c=%d\n", r, hoc_cbufstr->buf[0]);
    if (r == 1) {
        size_t n = strlen(hoc_cbufstr->buf) + 1;
        hoc_ctp = hoc_cbufstr->buf + n - 1;
        char* p = static_cast<char*>(PyMem_RawMalloc(n));
        if (p == 0) {
            return 0;
        }
        strcpy(p, hoc_cbufstr->buf);
        return p;
    } else if (r == EOF) {
        char* p = static_cast<char*>(PyMem_RawMalloc(2));
        if (p == 0) {
            return 0;
        }
        p[0] = '\0';
        return p;
    }
    return 0;
}
