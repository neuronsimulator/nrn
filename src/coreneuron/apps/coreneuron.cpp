/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#include <coreneuron/engine.h>
#include "coreneuron/utils/profile/profiler_interface.h"

int main(int argc, char** argv) {
    coreneuron::Instrumentor::init_profile();
    auto solve_core_result = solve_core(argc, argv);
    coreneuron::Instrumentor::finalize_profile();
    return solve_core_result;
}
