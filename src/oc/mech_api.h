#pragma once
/** @brief Header file included in translated mechanism.
 *
 *  This ensures that all functions that are tacitly part of the NEURON API
 *  usable from .mod files have definitions that are visible when the translated
 *  versions of those .mod files are compiled.
 *
 *  @todo Functions defined in nrniv_mf.h are also used by .mod files, but that
 *        header has to be sandwiched between md1redef.h and md2redef.h, which
 *        we leave to nocmodl.
 */
#include <nrniv/bbsavestate.h>
#include <oc/mcran4.h>
#include <nrnoc/nrncvode.h>
#include <oc/nrnran123.h>
#include <oc/nrnrandom.h>
#include <oc/oc_ansi.h>
#include <nrnoc/nrnversionmacros.h>
#include <scopmath/scoplib.h>

#include <cmath>     // nocmodl uses std::isnan
#include <iostream>  // nocmodl uses std::cerr
