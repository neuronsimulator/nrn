/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================
*/

/*
 * Coreneuron global variables are declared at least in the coreneuron namespace. In ispc it is,
 * however, not possible to access variables within C++ namespaces. To be able to access these
 * variables from ispc kernels, we declare them in global namespace and a C linkage file.
 */

extern "C" {
double ispc_celsius;
}
