/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#ifndef usenrnmpi_h
#define usenrnmpi_h

/* define to 1 if you want MPI specific features activated
   (optionally provided by CMake option NRNMPI) */
#ifndef NRNMPI
#define NRNMPI 1
#endif

/* define to 1 if want multisend spike exchange available */
#ifndef NRN_MULTISEND
#define NRN_MULTISEND 1
#endif

/* define to 1 if you want parallel distributed cells (and gap junctions) */
#define PARANEURON 1

/* define to 1 if you want mpi dynamically loaded instead of linked normally */
#undef NRNMPI_DYNAMICLOAD

/* define to 1 if you want the MUSIC - MUlti SImulation Coordinator */
#undef NRN_MUSIC

/* define to the dll path if you want to load automatically */
#undef DLL_DEFAULT_FNAME

/* Number of times to retry a failed open */
#undef FILE_OPEN_RETRY

/* Define to 1 for possibility of rank 0 xopen/ropen a file and broadcast everywhere */
#undef USE_NRNFILEWRAP

#endif
