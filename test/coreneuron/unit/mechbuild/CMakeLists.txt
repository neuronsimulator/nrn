# Copyright (c) 2016, Blue Brain Project
# All rights reserved.

# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.

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

# Test: no Neurodamus mechs, but all customs 

set(BACM_CMAKEOPTS "-DADDITIONAL_MECHS=${extra_mechs_file} -DADDITIONAL_MECHPATH='${extra_mechs_dir1};${extra_mechs_dir2}'")

set(BACM_MUST_HAVE ${all_extra_mods})
list_to_spacesep(BACM_MUST_HAVE)

set(BACM_MUST_NOT_HAVE ${nd_mods}) # Neurodamus-derived module
list_to_spacesep(BACM_MUST_NOT_HAVE)

make_bacm_test(bacm-extra-no-nd)

# Test: Neurodamus mechs, only part of extra 

set(BACM_CMAKEOPTS "-DADDITIONAL_MECHS=${extra_mechs_file} -DADDITIONAL_MECHPATH='${extra_mechs_dir2}'")

set(BACM_MUST_HAVE ${extra_mechs2})
list_to_spacesep(BACM_MUST_HAVE)

set(BACM_MUST_NOT_HAVE ${extra_mechs1})
list_to_spacesep(BACM_MUST_NOT_HAVE)

make_bacm_test(bacm-nd-extra2only)

#

add_test_class(unit)
