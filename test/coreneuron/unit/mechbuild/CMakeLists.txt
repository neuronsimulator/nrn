include(TestScriptUtils)

set(TEST_LABEL unit modinclude)

# Additional mechs for testing mech inclusion are stored in extra_mech and extra_mech2.

set(extra_mechs_file "${CMAKE_CURRENT_BINARY_DIR}/extra_mechs")
set(extra_mechs_dir1 "${CMAKE_CURRENT_SOURCE_DIR}/extra_mech1")
set(extra_mechs_dir2 "${CMAKE_CURRENT_SOURCE_DIR}/extra_mech2")

file(GLOB_RECURSE extra_mechs1 RELATIVE "${extra_mechs_dir1}" "${extra_mechs_dir1}/*.mod")
file(GLOB_RECURSE extra_mechs2 RELATIVE "${extra_mechs_dir2}" "${extra_mechs_dir2}/*.mod")

set(all_extra_mods ${extra_mechs1} ${extra_mechs2})
file(REMOVE "${extra_mechs_file}")
foreach(mod ${all_extra_mods})
    file(APPEND "${extra_mechs_file}" "${mod}\n")
endforeach()

set(bacm_in "build_and_check_mods.sh.in")

function(make_bacm_test test_name)
    configure_file(${bacm_in} "${CMAKE_CURRENT_BINARY_DIR}/${test_name}.sh" @ONLY)
    add_test_script(${test_name} "${CMAKE_CURRENT_BINARY_DIR}/${test_name}.sh" bash)
endfunction()

macro(list_to_spacesep listvar)
    string(REPLACE ";" " " ${listvar} "${${listvar}}")
endmacro()

# For test purposes, check for presence or absence of these Neurodamus mods:
set(nd_mods "Ca.mod")

# Test: no Neurodamus mechs, but all customs 

set(BACM_CMAKEOPTS "-DCOREBLURON_NEURODAMUS_MECHS=OFF -DCOREBLURON_ADDITIONAL_MECHS=${extra_mechs_file} -DCOREBLURON_ADDITIONAL_MECHPATH='${extra_mechs_dir1};${extra_mechs_dir2}'")

set(BACM_MUST_HAVE ${all_extra_mods})
list_to_spacesep(BACM_MUST_HAVE)

set(BACM_MUST_NOT_HAVE ${nd_mods}) # Neurodamus-derived module
list_to_spacesep(BACM_MUST_NOT_HAVE)

make_bacm_test(bacm-extra-no-nd)

# Test: Neurodamus mechs, only part of extra 

set(BACM_CMAKEOPTS "-DCOREBLURON_NEURODAMUS_MECHS=ON -DCOREBLURON_ADDITIONAL_MECHS=${extra_mechs_file} -DCOREBLURON_ADDITIONAL_MECHPATH='${extra_mechs_dir2}'")

set(BACM_MUST_HAVE ${extra_mechs2} ${nd_mods})
list_to_spacesep(BACM_MUST_HAVE)

set(BACM_MUST_NOT_HAVE ${extra_mechs1})
list_to_spacesep(BACM_MUST_NOT_HAVE)

make_bacm_test(bacm-nd-extra2only)

#

add_test_class(unit)
