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
#include "nrnrandom.h"
#include "nrnran123.h"
#include "nrnversionmacros.h"
#include "scoplib.h"
