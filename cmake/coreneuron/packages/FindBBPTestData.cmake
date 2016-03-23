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


##
## Find package file for the Blue Brain Functional test data files
##

# if not found via common_package
# use conventional cmake to find test data
if(NOT BBPTESTDATA_FILE)
    find_file(BBPTESTDATA_FILE "TestDatasets.h"
                PATH_SUFFIXES "BBPTestData" "BBP"
		HINTS $ENV{BBPTestData_DIR}/include)
   if(BBPTESTDATA_FILE) 
	   message("-- BBP Test Data header file found ${BBPTESTDATA_FILE} ")
   endif()
endif(NOT BBPTESTDATA_FILE)

# detect BBP test Data circuit path
if(BBPTESTDATA_FILE)
    file(READ ${BBPTESTDATA_FILE} BBPTESTDATA_CONFIG_CONTENT)
    string(REGEX MATCH "[ \\t]*BBP_TESTDATA[ \t\"]*([^ \n\t\r\"]*)\"?\r?\n" BBP_TESTDATA_DATA_DIR_RAW  ${BBPTESTDATA_CONFIG_CONTENT})
    set(STRIP ${CMAKE_MATCH_1} BBP_TESTDATA_DATA_DIR)
    set(BBP_TESTDATA_DATA_DIR ${CMAKE_MATCH_1} CACHE STRING "BBP testData user data path")

    if(BBP_TESTDATA_DATA_DIR)
        message("-- BBP Test Data storage directory found ${BBP_TESTDATA_DATA_DIR}")
    else(BBP_TESTDATA_DATA_DIR)
        message("- BBP Test Data storage directory not found")
    endif(BBP_TESTDATA_DATA_DIR)
else(BBPTESTDATA_FILE)
    message(WARNING "-- No BBP TestDATA circuit found, disable tests requiring TestData")
endif(BBPTESTDATA_FILE)
