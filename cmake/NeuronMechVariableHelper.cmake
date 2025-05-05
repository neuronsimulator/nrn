# =============================================================================
# Set some variables so (core)NEURON mechanisms can actually be tested without installing it. This
# is handled on a user machine in `neuronConfigcmake.in`
# =============================================================================

set(_prefix "${PROJECT_BINARY_DIR}")
set(_NEURON_MAIN "${PROJECT_SOURCE_DIR}/src/ivoc/nrnmain.cpp")
set(_NEURON_MAIN_INCLUDE_DIR "${_prefix}/include/nrncvode")
set(_NEURON_MECH_REG "${PROJECT_SOURCE_DIR}/cmake/mod_reg_nrn.cpp.in")

set(_CORENEURON_BASE_MOD "${PROJECT_SOURCE_DIR}/src/coreneuron/mechanism/mech/modfile/")
set(_CORENEURON_MAIN "${PROJECT_SOURCE_DIR}/src/coreneuron/apps/coreneuron.cpp")
set(_CORENEURON_MECH_REG "${PROJECT_SOURCE_DIR}/cmake/mod_reg_corenrn.cpp.in")
set(_CORENEURON_MECH_ENG "${PROJECT_SOURCE_DIR}/src/coreneuron/mechanism/mech/enginemech.cpp")
set(_CORENEURON_RANDOM_INCLUDE "${_prefix}/include/coreneuron/utils/randoms")
set(_CORENEURON_FLAGS ${CORENRN_CXX_FLAGS})
separate_arguments(_CORENEURON_FLAGS)
