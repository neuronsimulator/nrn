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
#include <ocfile.h> // bool isDirExist(const std::string& path);

#include <hocstr.h>
extern "C" void nrnpython_real();
extern "C" void nrnpython_start(int);
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
// extern char*(*PyOS_ReadlineFunctionPointer)(FILE*, FILE*, char*);
#if (PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION >= 4)
static char* nrnpython_getline(FILE*, FILE*, const char*);
#elif((PY_MAJOR_VERSION >= 3) || \
      (PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION > 2))
static char* nrnpython_getline(FILE*, FILE*, char*);
#else
static char* nrnpython_getline(char*);
#endif
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
} // extern "C"

void nrnpy_augment_path() {
  static int augmented = 0;
  if (!augmented && strlen(neuronhome_forward()) > 0) {
    augmented = 1;
    int err = PyRun_SimpleString("import sys");
    assert(err == 0);
#if defined(__linux__) || defined(DARWIN)
    // If /where/installed/lib/python/neuron exists, then append to sys.path
    std::string lib = std::string(path_prefix_to_libnrniv());
#if !defined(NRNCMAKE)
    // For an autotools build on an x86_64, it ends with x86_64/lib/
    const char* lastpart = NRNHOSTCPU "/lib/";
    if (lib.length() > strlen(lastpart)) {
      size_t pos = lib.length() - strlen(lastpart);
      pos = lib.find(lastpart, pos);
      if (pos != std::string::npos) {
        lib.replace(pos, std::string::npos, "lib/");
      }
    }
#endif //!NRNCMAKE
#else // not defined(__linux__) || defined(DARWIN)
    std::string lib = std::string(neuronhome_forward()) + std::string("/lib/");
#endif //not defined(__linux__) || defined(DARWIN)
    if (isDirExist(lib + std::string("python/neuron"))) {
      std::string cmd = std::string("sys.path.append('") + lib + "python')";
      err = PyRun_SimpleString(cmd.c_str());
      assert(err == 0);
    }
    err = PyRun_SimpleString("sys.path.insert(0, '')");
    assert(err == 0);
  }
}

int nrnpy_pyrun(const char* fname) {
#ifdef MINGW
#if PY_MAJOR_VERSION >= 3
  // perhaps this should be the generic implementation
  char* cmd = new char[strlen(fname) + 40];
  sprintf(cmd, "exec(open(\"%s\").read(), globals())", fname);
  int err = PyRun_SimpleString(cmd);
  delete [] cmd;
  if (err != 0) {
    PyErr_Print();
    PyErr_Clear();
    return 0;
  }
  return 1;
#else // PY_MAJOR_VERSION < 3
  /*
  http://www.megasolutions.net/python/How-to-receive-a-FILE--from-Python-under-MinGW_-38375.aspx
  Because microsoft C runtimes are not binary compatible, we can't just
  call fopen to get a FILE * and pass that FILE * to another application
  or library (Python25.dll in this case) that uses a different version of
  the C runtime that this DLL uses.  Using PyFile_AsFile is a
  work-around...
  */
  PyObject* pfo = PyFile_FromString(fname, (char*)"r");
  if (pfo == NULL) {
    PyErr_Print();
    PyErr_Clear();
    return 0;
  } else {
    char* cmd = NULL;
    if (PyRun_AnyFile(PyFile_AsFile(pfo), fname) == -1) {
      PyErr_Print();
      PyErr_Clear();
      Py_DECREF(pfo);
      if (cmd) { delete [] cmd; }
      return 0;
    }
    Py_DECREF(pfo);
    if (cmd) { delete [] cmd; }
    return 1;
  }
#endif // PY_MAJOR_VERSION < 3
#else // MINGW not defined
  FILE* fp = fopen(fname, "r");
  if (fp) {
    PyRun_AnyFile(fp, fname);
    fclose(fp);
    return 1;
  } else {
    fprintf(stderr, "Could not open %s\n", fname);
    return 0;
  }
#endif // MINGW not defined
}

#if PY_MAJOR_VERSION >= 3
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
  // basically a copy of code from Modules/python.cpp
  wcargv = (wchar_t**)PyMem_Malloc(sizeof(wchar_t*) * argc);
  if (!wcargv) {
    fprintf(stderr, "out of memory\n");
    exit(1);
  }
  for (int i = 0; i < argc; ++i) {
#if 1
    wcargv[i] = Py_DecodeLocale(argv[i], NULL);
    if (!wcargv[i]) {
      fprintf(stderr, "out of memory\n");
      exit(1);
    }
#else
#ifdef HAVE_BROKEN_MBSTOWCS
    size_t argsize = strlen(argv[i]);
#else
    size_t argsize = mbstowcs(NULL, argv[i], 0);
#endif
    if (argsize == (size_t)-1) {
      fprintf(stderr, "Could not convert argument %d to string\n", i);
      exit(1);
    }
    wcargv[i] = (wchar_t*)PyMem_Malloc((argsize + 1) * sizeof(wchar_t));
    if (!wcargv[i]) {
      fprintf(stderr, "out of memory\n");
      exit(1);
    }
    int count = mbstowcs(wcargv[i], argv[i], argsize + 1);
    if (count == (size_t)-1) {
      fprintf(stderr, "Could not convert argument %d to string\n", i);
      exit(1);
    }
#endif
  }
}

static wchar_t* mywstrdup(char* s) {
  size_t sz = mbstowcs(NULL, s, 0);
  wchar_t* ws = new wchar_t[sz + 1];
  int count = mbstowcs(ws, s, sz + 1);
  return ws;
}
#endif

extern "C" void nrnpython_start(int b) {
#if USE_PYTHON
  static int started = 0;
  //printf("nrnpython_start %d started=%d\n", b, started);
  if (b == 1 && !started) {
    p_nrnpy_pyrun = nrnpy_pyrun;
    if (nrnpy_nositeflag) {
      Py_NoSiteFlag = 1;
    }
    // nrnpy_pyhome hopefully holds the python base root and should
    // work with virtual environments.
    // But use only if not overridden by the PYTHONHOME environment variable.
    char * _p_pyhome = getenv("PYTHONHOME");
    if (_p_pyhome == NULL) {
        _p_pyhome = nrnpy_pyhome;
    }
    if (_p_pyhome) {
#if PY_MAJOR_VERSION >= 3
        Py_SetPythonHome(mywstrdup(_p_pyhome));
#else
        Py_SetPythonHome(_p_pyhome);
#endif
    }
    Py_Initialize();
#if NRNPYTHON_DYNAMICLOAD
    // return from Py_Initialize means there was no site problem
    nrnpy_site_problem = 0;
#endif
#if PY_MAJOR_VERSION >= 3
    copy_argv_wcargv(nrn_global_argc, nrn_global_argv);
    PySys_SetArgv(nrn_global_argc, wcargv);
#else
    PySys_SetArgv(nrn_global_argc, nrn_global_argv);
#endif
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
#if PY_MAJOR_VERSION >= 3
    del_wcargv(nrn_global_argc);
#endif
    // because of finalize, no PyGILState_Release(gilsav);
    started = 0;
  }
  if (b == 2 && started) {
    int i;
#if (PY_MAJOR_VERSION >= 3)
    copy_argv_wcargv(nrn_global_argc, nrn_global_argv);
    PySys_SetArgv(nrn_global_argc, wcargv);
#else
    PySys_SetArgv(nrn_global_argc, nrn_global_argv);
#endif
    nrnpy_augment_path();
#if !defined(MINGW)
    // cannot get this to avoid crashing with MINGW
    PyOS_ReadlineFunctionPointer = nrnpython_getline;
#endif
    // Is there a -c "command" or file.py arg.
    for (i = 1; i < nrn_global_argc; ++i) {
      char* arg = nrn_global_argv[i];
      if (strcmp(arg, "-c") == 0 && i + 1 < nrn_global_argc) {
        PyRun_SimpleString(nrn_global_argv[i + 1]);
        break;
      } else if (strlen(arg) > 3 && strcmp(arg + strlen(arg) - 3, ".py") == 0) {
        nrnpy_pyrun(arg);
        break;
      }
    }
    // In any case start interactive.
    //		PyRun_InteractiveLoop(fopen("/dev/tty", "r"), "/dev/tty");
    PyRun_InteractiveLoop(hoc_fin, "stdin");
  }
  if (b == 3 && started) {
#if HAVE_IV
    if (Session::instance()) {
      Session::instance()->quit();
      rl_stuff_char(EOF);
    }
#endif
  }
#endif
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

#if (PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION >= 4)
static char* nrnpython_getline(FILE*, FILE*, const char* prompt) {
#elif((PY_MAJOR_VERSION >= 3) || \
      (PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION > 2))
static char* nrnpython_getline(FILE*, FILE*, char* prompt) {
#else
static char* nrnpython_getline(char* prompt) {
#endif
  hoc_cbufstr->buf[0] = '\0';
  hoc_promptstr = prompt;
  int r = hoc_get_line();
  // printf("r=%d c=%d\n", r, hoc_cbufstr->buf[0]);
  if (r == 1) {
    size_t n = strlen(hoc_cbufstr->buf) + 1;
    hoc_ctp = hoc_cbufstr->buf + n - 1;
#if (PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION >= 4)
    char* p = static_cast<char*>(PyMem_RawMalloc(n));
#else
    char* p = static_cast<char*>(PyMem_MALLOC(n));
#endif
    if (p == 0) {
      return 0;
    }
    strcpy(p, hoc_cbufstr->buf);
    return p;
  } else if (r == EOF) {
#if (PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION >= 4)
    char* p = static_cast<char*>(PyMem_RawMalloc(2));
#else
    char* p = static_cast<char*>(PyMem_MALLOC(2));
#endif
    if (p == 0) {
      return 0;
    }
    p[0] = '\0';
    return p;
  }
  return 0;
}
