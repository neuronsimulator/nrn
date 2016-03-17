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
# @author Sam Yates

# Utility functions for manipulating test labels and producing
# tests from scripts:
#
# 1. add_test_class(label [label2 ...])
#
#    Create a target with name test-label (or test-label-label2 etc.)
#    which runs only those tests possessing all of the supplied labels.
#
#
# 2. add_test_label(name label ...)
#
#    Add the given labels to the test 'name'.
#
#
# 3. add_test_script(name script interp)
#
#    Add a test 'name' that runs the given script, using the
#    interpreter 'interp'. If no interpreter is supplied,
#    the script will be run with /bin/sh.
#
#    Uses the following variables to customize the new test:
#    * TEST_LABEL, ${NAME}_TEST_LABEL
#          If defined, apply the label(s) in these variable to the
#          new test.
#    * TEST_ARGS, ${NAME}_TEST_ARGS
#          Additional arguments to pass to the script.
#          ${NAME}_TEST_ARGS takes priority over TEST_ARGS.
#    * TEST_ENVIRONMENT
#          Additional environment variables to define for the test;
#          added to test properties.
#    * TEST_PREFIX, ${NAME}_TEST_PREFIX
#          If defined, preface the interpreter with this prefix.
#          ${NAME}_TEST_PREFIX takes priority over TEST_PREFIX.

function(add_test_label NAME)
    set_property(TEST ${NAME} APPEND PROPERTY LABELS ${ARGN})
    # create test classes for each label
    foreach(L ${ARGN})
        add_test_class(${L})
    endforeach()
endfunction()

function(add_test_script NAME SCRIPT INTERP)
    set(RUN_PREFIX ${TEST_PREFIX})
    if(${NAME}_TEST_PREFIX)
        set(RUN_PREFIX ${${NAME}_TEST_PREFIX})
    endif()

    if(NOT INTERP)
        set(INTERP "/bin/sh")
    endif()

    set(RUN_ARGS ${TEST_ARGS})
    if (${NAME}_TEST_ARGS)
        set(RUN_ARGS ${${NAME}_TEST_ARGS})
    endif()

    set(SCRIPT_PATH "${SCRIPT}")
    if(NOT IS_ABSOLUTE "${SCRIPT_PATH}")
	set(SCRIPT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${SCRIPT_PATH}")
    endif()

    add_test(NAME ${NAME}
        COMMAND ${RUN_PREFIX} ${INTERP} "${SCRIPT_PATH}" ${RUN_ARGS}
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}") 

    # Add test labels
    set(TEST_LABELS ${TEST_LABEL} ${${NAME}_TEST_LABEL})
    if (TEST_LABELS)
        add_test_label(${NAME} ${TEST_LABELS})
    endif()

    if(TEST_ENVIRONMENT)
        set_property(TEST ${NAME} PROPERTY ENVIRONMENT ${TEST_ENVIRONMENT})
    endif()
endfunction()

function(add_test_class)
    string(REPLACE ";" "-" TEST_SUFFIX "${ARGN}")
    string(REPLACE ";" "$$;-L;^" TEST_LOPTS "${ARGN}")

    if(NOT TARGET test-${TEST_SUFFIX})
        add_custom_target("test-${TEST_SUFFIX}"
            COMMAND ${CMAKE_CTEST_COMMAND} -L ^${TEST_LOPTS}$$
            WORKING_DIRECTORY ${${PROJECT_NAME}_BINARY_DIR}
            COMMENT "Running all ${ARGN} tests")
    endif()
endfunction()

