#pragma once

#include "../../nrnconf.h"
#if defined(HAVE_DLFCN_H)
#include <dlfcn.h>
#endif

#if !defined(HAVE_DLFCN_H) && defined(_WIN32)
#include "../mswin/dlfcn.h"
#define HAVE_DLFCN_H 1
#endif
