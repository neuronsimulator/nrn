# Often older version of flex is available in /usr. Even we set PATH for newer flex, CMake will set
# FLEX_INCLUDE_DIRS to /usr/include. This will result in compilation errors. Hence we check for flex
# include directory for the corresponding FLEX_EXECUTABLE. If found, we add that first and then we
# include include path from CMake.

get_filename_component(FLEX_BIN_DIR ${FLEX_EXECUTABLE} DIRECTORY)

if(NOT FLEX_BIN_DIR MATCHES "/usr/bin")
  if(NOT FLEX_INCLUDE_DIRS)
    get_filename_component(FLEX_INCLUDE_DIRS ${FLEX_BIN_DIR} PATH)
    set(FLEX_INCLUDE_DIRS ${FLEX_INCLUDE_DIRS}/include/)
  endif()
  if(EXISTS "${FLEX_INCLUDE_DIRS}/FlexLexer.h")
    message(STATUS " Adding Flex include path as : ${FLEX_INCLUDE_DIRS}")
    include_directories(${FLEX_INCLUDE_DIRS})
  else()
    message(
      FATAL_ERROR
        "${FLEX_INCLUDE_DIRS} does not contain file FlexLexer.h, please set NRN_FLEX_INCLUDE_DIRS manually"
    )
  endif()
endif()

# Try to set include dir manually
set(NRN_FLEX_INCLUDE_DIRS
    "${FLEX_INCLUDE_DIRS}"
    CACHE STRING "The path to the dir containing Flex headers")

if(NOT NRN_FLEX_INCLUDE_DIRS AND NOT EXISTS "${NRN_FLEX_INCLUDE_DIRS}/FlexLexer.h")
  message(FATAL_ERROR "FlexLexer.h not found in ${NRN_FLEX_INCLUDE_DIRS}, "
                      "please set another value for NRN_FLEX_INCLUDE_DIRS")
else()
  include_directories("${NRN_FLEX_INCLUDE_DIRS}")
endif()
