# TEST_LIBRARIES variable is used by the CommonCTest.cmake script to link against the given libraries
set(TEST_LIBRARIES ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} coreneuron)

# CommonCTest, in the current folder recursively compiles targets for *.cpp files using TEST_LIBRARIES
#include(CommonCTest)
