flex_target(modlunit_lex lex.lpp ${CMAKE_CURRENT_BINARY_DIR}/lex.cpp)
bison_target(modlunit_parse parse1.ypp ${CMAKE_CURRENT_BINARY_DIR}/parse1.cpp DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parse1.hpp)

add_executable(modlunit 
    consist.cpp
    declare.cpp
    init.cpp
    io.cpp
    kinunit.cpp
    list.cpp
    model.cpp
    nrnunit.cpp
    passn.cpp
    symbol.cpp
    units.cpp
    units1.cpp
    version.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/lex.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/parse1.cpp")
target_compile_definitions(modlunit PRIVATE NRNUNIT=1)
cpp_cc_configure_sanitizers(TARGET modlunit)
target_include_directories(modlunit PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}")

if(NRN_NMODL_CXX_FLAGS)
  target_compile_options(modlunit PRIVATE ${NRN_NMODL_CXX_FLAGS})
endif()

install(TARGETS modlunit DESTINATION bin)

