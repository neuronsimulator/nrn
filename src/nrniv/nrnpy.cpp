#include <../../nrnconf.h>
// For Linux and Max OS X,
// Solve the problem of not knowing what version of Python the user has by
// possibly deferring linking to libnrnpython.so to run time using the proper
// Python interface

#include <../nrnpython/nrnpython_config.h>
#include <stdio.h>
#include <InterViews/resource.h>
#include "nrnoc2iv.h"
#include "classreg.h"

extern "C" {
extern void (*p_nrnpython_start)(int);
void nrnpython();
static void (*p_nrnpython_real)();
static void (*p_nrnpython_reg_real)();
}

#if NRNPYTHON_DYNAMICLOAD
#include <dlfcn.h>
extern "C" {
extern char* neuron_home;
}
static char* ver[] = {"3.0", "2.6", "2.5", "2.4", "2.3", 0};
static int iver; // which python is loaded?
static void* python_already_loaded();
static void* load_python();
static void load_nrnpython();
#else
extern "C" {
extern void nrnpython_start(int);
extern void nrnpython_reg_real();
extern void nrnpython_real();
}
#endif

void nrnpython() {
#if USE_PYTHON
	if (p_nrnpython_real) {
		(*p_nrnpython_real)();
		return;
	}
#endif	
	ret(0.);
}

// Stub class for when Python does not exist
static void* p_cons(Object*) {
	return 0;
}
static void p_destruct(void* v) {
}
static Member_func p_members[] = {0,0};

void nrnpython_reg() {
	//printf("nrnpython_reg in nrnpy.cpp\n");
#if USE_PYTHON
#if NRNPYTHON_DYNAMICLOAD
	void* handle = python_already_loaded();
	if (!handle) { // embed python
		handle = load_python();
	}
	if (handle) {
		load_nrnpython();
	}
#else
	p_nrnpython_start = nrnpython_start;
	p_nrnpython_real = nrnpython_real;
	p_nrnpython_reg_real = nrnpython_reg_real;
#endif
	if (p_nrnpython_reg_real) {
		(*p_nrnpython_reg_real)();
		return;
	}
#endif
	class2oc("PythonObject", p_cons, p_destruct, p_members);
}

#if NRNPYTHON_DYNAMICLOAD // to end of file

// important dlopen flags :
// RTLD_NOLOAD returns NULL if not open, or handle if it is resident.

static void* ver_dlo(int flag) {
	for (int i = 0; ver[i]; ++i) {
		char name[100];
#if DARWIN
		sprintf(name, "libpython%s.dylib", ver[i]);
#else
		sprintf(name, "libpython%s.so", ver[i]);
#endif
		void* handle = dlopen(name, flag);
		iver = i;
		if (handle) {
			return handle;
		}
	}
	iver = -1;
	return NULL;
}

static void* python_already_loaded() {
	void* handle = ver_dlo(RTLD_NOW|RTLD_GLOBAL|RTLD_NOLOAD);
	//printf("python_already_loaded %d\n", iver);
	return handle;
}

static void* load_python() {
	void* handle = ver_dlo(RTLD_NOW|RTLD_GLOBAL);
	//printf("load_python %d\n", iver);
	return handle;
}

static void* load_sym(void* handle, char* name) {
	void* p = dlsym(handle, name);
	if (!p) {
		printf("Could not load %s\n", name);
		exit(1);
	}
	return p;
}

static void load_nrnpython() {
	char name[100];
#if DARWIN
	sprintf(name, "%s/../../umac/lib/libnrnpython%c%c.dylib", neuron_home, ver[iver][0], ver[iver][2]);
#else
	sprintf(name, "libnrnpython%c%c.so", ver[iver][0], ver[iver][2]);
#endif
	void* handle = dlopen(name, RTLD_NOW);
	if (!handle) {
		printf("Could not load %s\n", name);
	}
	p_nrnpython_start = (void(*)(int))load_sym(handle, "nrnpython_start");
	p_nrnpython_real = (void(*)())load_sym(handle, "nrnpython_real");
	p_nrnpython_reg_real = (void(*)())load_sym(handle, "nrnpython_reg_real");
}

#endif
