/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#pragma once

/* define to 1 if you want MPI specific features activated
   (optionally provided by CMake option NRNMPI) */
#ifndef NRNMPI
#define NRNMPI 1
#endif

/* define to 1 if want multisend spike exchange available */
#ifndef NRN_MULTISEND
#define NRN_MULTISEND 1
#endif

/* define to 1 if you want the MUSIC - MUlti SImulation Coordinator */
#undef NRN_MUSIC

/* define to the dll path if you want to load automatically */
#undef DLL_DEFAULT_FNAME
