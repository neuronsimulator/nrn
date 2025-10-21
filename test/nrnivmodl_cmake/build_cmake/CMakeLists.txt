cmake_minimum_required(VERSION 3.15)
project(modfile_test LANGUAGES C CXX @CUDA_STRING@)

find_package(neuron REQUIRED)

create_nrnmech(NEURON ${CORENEURON} SPECIAL MOD_FILES ${MOD_FILES})
