#include <nrnpython.h>
#include <stdio.h>
#include <InterViews/resource.h>
#if HAVE_IV
#include <InterViews/session.h>
#endif
#include <nrnoc2iv.h>
#include <nrnpy_reg.h>
#include <hoccontext.h>

extern "C" {
#include <hocstr.h>
void nrnpython_real();
void nrnpython_start(int);
extern int hoc_get_line();
extern HocStr* hoc_cbufstr;
extern int nrnpy_nositeflag;
extern char* hoc_ctp;
extern FILE* hoc_fin;
extern const char* hoc_promptstr;
extern char* neuronhome_forward();
//extern char*(*PyOS_ReadlineFunctionPointer)(FILE*, FILE*, char*);
#if (PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION >= 4)
static char* nrnpython_getline(FILE*, FILE*, const char*);
#elif ((PY_MAJOR_VERSION >= 3) || (PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION > 2))
static char* nrnpython_getline(FILE*, FILE*, char*);
#else
static char* nrnpython_getline(char*);
#endif
extern void rl_stuff_char(int);
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
}

void nrnpy_augment_path() {
	static int augmented = 0;
	char buf[1024];
	if (!augmented && strlen(neuronhome_forward()) > 0) {
		//printf("augment_path\n");
		augmented = 1;
		sprintf(buf, "sys.path.append('%s/lib/python')", neuronhome_forward());
		assert(PyRun_SimpleString("import sys") == 0);
		assert(PyRun_SimpleString(buf) == 0);	
		sprintf(buf, "sys.path.prepend('')");
		assert(PyRun_SimpleString("sys.path.insert(0, '')") == 0);	
	}
}

int nrnpy_pyrun(const char* fname) {
#ifdef MINGW
/*
http://www.megasolutions.net/python/How-to-receive-a-FILE--from-Python-under-MinGW_-38375.aspx
Because microsoft C runtimes are not binary compatible, we can't just
call fopen to get a FILE * and pass that FILE * to another application
or library (Python25.dll in this case) that uses a different version of
the C runtime that this DLL uses.  Using PyFile_AsFile is a
work-around... 
*/
	PyObject* pfo = PyFile_FromString((char*)fname, (char*)"r");
	if (pfo == NULL) {
		PyErr_Print();
		PyErr_Clear();
		return 0;
	}else{
		if (PyRun_AnyFile(PyFile_AsFile(pfo), fname) == -1) {
			PyErr_Print();
			PyErr_Clear();
			return 0;
		}
		Py_DECREF(pfo);
		return 1;
	}
#else
	FILE* fp = fopen(fname, "r");
	if (fp) {
		PyRun_AnyFile(fp, fname);
		fclose(fp);
		return 1;
	}else{
		fprintf(stderr, "Could not open %s\n", fname);
		return 0;
	}
#endif
}

#if PY_MAJOR_VERSION >= 3
static wchar_t** copy_argv_wcargv(int argc, char** argv) {
	// adapted from frozenmain.c, left out error checking
	wchar_t **argv_copy = (wchar_t**)PyMem_Malloc(sizeof(wchar_t*)*argc);
	for (int i=0; i < argc; ++i) {
		size_t argsize = strlen(argv[i]);
		argv_copy[i] = (wchar_t*)PyMem_Malloc((argsize+1)*sizeof(wchar_t));
		int count = mbstowcs(argv_copy[i], argv[i], argsize+1);
	}
	return argv_copy;
}
#endif

void nrnpython_start(int b) {
#if USE_PYTHON
	static int started = 0;
//	printf("nrnpython_start %d started=%d\n", b, started);
	if (b == 1 && !started) {
		p_nrnpy_pyrun = nrnpy_pyrun;
		if (nrnpy_nositeflag) {
			Py_NoSiteFlag = 1;
		}
		//printf("Py_NoSiteFlag = %d\n", Py_NoSiteFlag);
		Py_Initialize();
#if NRNPYTHON_DYNAMICLOAD
		// return from Py_Initialize means there was no site problem
		nrnpy_site_problem = 0;
#endif
#if PY_MAJOR_VERSION >= 3
		wchar_t **argv_copy = copy_argv_wcargv(nrn_global_argc, nrn_global_argv);
		PySys_SetArgv(nrn_global_argc, argv_copy);
#else
		PySys_SetArgv(nrn_global_argc, nrn_global_argv);
#endif
		started = 1;
		int i;
		// see nrnpy_reg.h
		for (i=0; nrnpy_reg_[i]; ++i) {
			(*nrnpy_reg_[i])();
		}
		nrnpy_augment_path();
	}
	if (b == 0 && started) {
		PyGILState_STATE gilsav = PyGILState_Ensure();
		Py_Finalize();
		// because of finalize, no PyGILState_Release(gilsav);
		started = 0;
	}
	if (b == 2 && started) {
		int i;
#if (PY_MAJOR_VERSION >= 3)
		// basically a copy of code from Modules/python.c
		wchar_t **argv_copy = (wchar_t**)PyMem_Malloc(sizeof(wchar_t*)*nrn_global_argc);
		if (!argv_copy) {
			fprintf(stderr, "out of memory\n");
			exit(1);
		}
		for (i=0; i < nrn_global_argc; ++i) {
#ifdef HAVE_BROKEN_MBSTOWCS
			size_t argsize = strlen(argv[i]);
#else
			size_t argsize = mbstowcs(NULL, nrn_global_argv[i], 0);
#endif
			size_t count;
			if (argsize == (size_t)-1) {
				fprintf(stderr, "Could not convert argument %d to string\n", i);
				exit(1);
			}
			argv_copy[i] = (wchar_t*)PyMem_Malloc((argsize+1)*sizeof(wchar_t));
			if (!argv_copy[i]) {
				fprintf(stderr, "out of memory\n");
				exit(1);
			}
			count = mbstowcs(argv_copy[i], nrn_global_argv[i], argsize+1);
			if (count == (size_t)-1) {
				fprintf(stderr, "Could not convert argument %d to string\n", i);
				exit(1);
			}
		}
                PySys_SetArgv(nrn_global_argc, argv_copy);
#else
                PySys_SetArgv(nrn_global_argc, nrn_global_argv);
#endif
		nrnpy_augment_path();
#if !defined(MINGW)
		// cannot get this to avoid crashing with MINGW
		PyOS_ReadlineFunctionPointer = nrnpython_getline;
#endif
		// Is there a -c "command" or file.py arg.
		for (i=1; i < nrn_global_argc; ++i) {
			char* arg = nrn_global_argv[i];
			if (strcmp(arg, "-c") == 0 && i+1 < nrn_global_argc) {
				PyRun_SimpleString(nrn_global_argv[i+1]);
				break;
			}else if (strlen(arg) > 3 && strcmp(arg+strlen(arg)-3, ".py") == 0) {
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

void nrnpython_real() {
	int retval = 0;
#if USE_PYTHON
	HocTopContextSet
	PyGILState_STATE gilsav = PyGILState_Ensure();
	retval = PyRun_SimpleString(gargstr(1)) == 0;
	PyGILState_Release(gilsav);
	HocContextRestore
#endif
	hoc_retpushx(double(retval));
}

#if (PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION >= 4)
static char* nrnpython_getline(FILE*, FILE*, const char* prompt) {
#elif ((PY_MAJOR_VERSION >= 3) || (PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION > 2))
static char* nrnpython_getline(FILE*, FILE*, char* prompt) {
#else
static char* nrnpython_getline(char* prompt) {
#endif
	hoc_cbufstr->buf[0] = '\0';
	hoc_promptstr = prompt;
	int r = hoc_get_line();
//printf("r=%d c=%d\n", r, hoc_cbufstr->buf[0]);
	if (r == 1) {
		size_t n = strlen(hoc_cbufstr->buf) + 1;
		hoc_ctp = hoc_cbufstr->buf + n - 1;
		char* p = (char*)PyMem_MALLOC(n);
		if (p == 0) { return 0; }
		strcpy(p, hoc_cbufstr->buf);
		return p;
	}else if (r == EOF) {
		char* p = (char*)PyMem_MALLOC(2);
		if (p == 0) { return 0; }
		p[0] = '\0';
		return p;	
	}
	return 0;
}
