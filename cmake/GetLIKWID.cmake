include_guard(DIRECTORY)

find_package(likwid REQUIRED)

add_library(nrn_likwid IMPORTED SHARED)
set_target_properties(nrn_likwid PROPERTIES IMPORTED_LOCATION ${LIKWID_LIBRARY})
target_include_directories(nrn_likwid INTERFACE ${LIKWID_INCLUDE_DIRS})
target_compile_definitions(nrn_likwid INTERFACE LIKWID_PERFMON)
