#ifndef nrnwrap_dlfcn_h
#define nrnwrap_dlfcn_h

#include "../../nrnconf.h"
#if defined(HAVE_DLFCN_H)
#include <dlfcn.h>
#endif

#if !defined(HAVE_DLFCN_H) && defined(MINGW)
#include "../mswin/dlfcn.h"
#define HAVE_DLFCN_H 1
#endif

#endif /* nrnwrap_dlfcn_h */
