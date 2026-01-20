#pragma once

#include "../../nrnconf.h"
#if defined(HAVE_DLFCN_H)
#include <dlfcn.h>
#elif defined(MINGW) || defined(_MSC_VER)
#include "../mswin/dlfcn.h"
#define HAVE_DLFCN_H 1
#endif
