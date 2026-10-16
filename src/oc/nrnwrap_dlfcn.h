#pragma once

#include "../../nrnconf.h"
#if defined(HAVE_DLFCN_H)
#include <dlfcn.h>
#endif

#if !defined(HAVE_DLFCN_H) && defined(MINGW)
#include "../mswin/dlfcn.h"
#define HAVE_DLFCN_H 1
#endif
