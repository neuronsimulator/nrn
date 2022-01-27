#pragma once
/** @brief Header file included in translated mechanism.
 *  
 *  This ensures that all functions that are tacitly part of the NEURON API
 *  usable from .mod files have definitions that are visible when the translated
 *  versions of those .mod files are compiled.
 */
#include "mcran4.h"
#include "nrncvode.h"
#include "nrnran123.h"
#include "oc_ansi.h"
#include "scoplib.h"
