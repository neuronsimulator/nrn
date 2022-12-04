#include <nrnpython.h>
#include <nrnpy_utils.h>
#include <stdio.h>
#include <InterViews/resource.h>
#if HAVE_IV
#include <InterViews/session.h>
#endif
#include <nrnoc2iv.h>
#include <nrnpy_reg.h>
#include <hoccontext.h>
#include <string>
#include <ocfile.h>  // bool isDirExist(const std::string& path);

#include <hocstr.h>
extern "C" void nrnpython_real();
extern "C" int nrnpython_start(int);
extern int hoc_get_line();
extern HocStr* hoc_cbufstr;
extern int nrnpy_nositeflag;
extern char* nrnpy_pyhome;
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
void nrnpy_augment_path();
int nrnpy_pyrun(const char*);
extern int (*p_nrnpy_pyrun)(const char*);
extern int nrn_global_argc;
extern char** nrn_global_argv;
#if NRNPYTHON_DYNAMICLOAD
int nrnpy_site_problem;
#endif

extern "C" {
extern void rl_stuff_char(int);
}  // extern "C"

void nrnpy_augment_path() {
    static int augmented = 0;
    if (!augmented && strlen(neuronhome_forward()) > 0) {
        augmented = 1;
        int err = PyRun_SimpleString("import sys");
        assert(err == 0);
#if defined(__linux__) || defined(DARWIN)
        // If /where/installed/lib/python/neuron exists, then append to sys.path
        std::string lib = std::string(path_prefix_to_libnrniv());
#else   // not defined(__linux__) || defined(DARWIN)
        std::string lib = std::string(neuronhome_forward()) + std::string("/lib/");
#endif  // not defined(__linux__) || defined(DARWIN)
        if (isDirExist(lib + std::string("python/neuron"))) {
            std::string cmd = std::string("sys.path.append('") + lib + "python')";
            err = PyRun_SimpleString(cmd.c_str());
            assert(err == 0);
        }
        err = PyRun_SimpleString("sys.path.insert(0, '')");
        assert(err == 0);
    }
}

/** @brief Execute a Python script.
 *  @return 0 on failure, 1 on success.
 */
int nrnpy_pyrun(const char* fname) {
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

static wchar_t** wcargv;

static void del_wcargv(int argc) {
    if (wcargv) {
        for (int i = 0; i < argc; ++i) {
            PyMem_Free(wcargv[i]);
        }
        PyMem_Free(wcargv);
        wcargv = NULL;
    }
}

static void copy_argv_wcargv(int argc, char** argv) {
    del_wcargv(argc);
    // basically a copy of code from Modules/python.c
    wcargv = (wchar_t**) PyMem_Malloc(sizeof(wchar_t*) * argc);
    if (!wcargv) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    for (int i = 0; i < argc; ++i) {
        wcargv[i] = Py_DecodeLocale(argv[i], NULL);
        if (!wcargv[i]) {
            fprintf(stderr, "out of memory\n");
            exit(1);
        }
    }
}

static wchar_t* mywstrdup(char* s) {
    size_t sz = mbstowcs(NULL, s, 0);
    wchar_t* ws = new wchar_t[sz + 1];
    int count = mbstowcs(ws, s, sz + 1);
    return ws;
}

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
        // nrnpy_pyhome hopefully holds the python base root and should
        // work with virtual environments.
        // But use only if not overridden by the PYTHONHOME environment variable.
        char* _p_pyhome = getenv("PYTHONHOME");
        if (_p_pyhome == NULL) {
            _p_pyhome = nrnpy_pyhome;
        }
        if (_p_pyhome) {
            Py_SetPythonHome(mywstrdup(_p_pyhome));
        }
        Py_Initialize();
#if NRNPYTHON_DYNAMICLOAD
        // return from Py_Initialize means there was no site problem
        nrnpy_site_problem = 0;
#endif
        copy_argv_wcargv(nrn_global_argc, nrn_global_argv);
        PySys_SetArgv(nrn_global_argc, wcargv);
        started = 1;
        // see nrnpy_reg.h
        for (int i = 0; nrnpy_reg_[i]; ++i) {
            (*nrnpy_reg_[i])();
        }
        nrnpy_augment_path();
    }
    if (b == 0 && started) {
        PyGILState_STATE gilsav = PyGILState_Ensure();
        Py_Finalize();
        del_wcargv(nrn_global_argc);
        // because of finalize, no PyGILState_Release(gilsav);
        started = 0;
    }
    if (b == 2 && started) {
        int i;
        copy_argv_wcargv(nrn_global_argc, nrn_global_argv);
        PySys_SetArgv(nrn_global_argc, wcargv);
        nrnpy_augment_path();
#if !defined(MINGW)
        // cannot get this to avoid crashing with MINGW
        PyOS_ReadlineFunctionPointer = nrnpython_getline;
#endif
        // Is there a -c "command" or file.py arg.
        bool python_error_encountered{false};
        for (i = 1; i < nrn_global_argc; ++i) {
            char* arg = nrn_global_argv[i];
            if (strcmp(arg, "-c") == 0 && i + 1 < nrn_global_argc) {
                if (PyRun_SimpleString(nrn_global_argv[i + 1])) {
                    python_error_encountered = true;
                }
                break;
            } else if (strlen(arg) > 3 && strcmp(arg + strlen(arg) - 3, ".py") == 0) {
                if (!nrnpy_pyrun(arg)) {
                    python_error_encountered = true;
                }
                break;
            }
        }
        // python_error_encountered dictates whether NEURON will exit with a nonzero
        // code. In noninteractive/batch mode that happens immediately, in
        // interactive mode then we start a Python interpreter first.
        if (nrn_istty_) {
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
    HocTopContextSet {
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
