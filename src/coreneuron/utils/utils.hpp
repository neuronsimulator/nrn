/*
# =============================================================================
# Copyright (c) 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#pragma once

namespace coreneuron {
extern void nrn_abort(int errcode);
extern void nrn_fatal_error(const char* msg);
extern double nrn_wtime(void);
}  // namespace coreneuron
