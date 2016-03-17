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
