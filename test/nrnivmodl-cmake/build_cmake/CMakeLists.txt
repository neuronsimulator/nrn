cmake_minimum_required(VERSION 3.15)
project(modfile_test CXX)

find_package(neuron REQUIRED)

create_nrnmech(CORENEURON MOD_FILES ${MOD_FILES})
