flex_target(nocmodl_lex lex.lpp ${CMAKE_CURRENT_BINARY_DIR}/lex.cpp)
bison_target(nocmodl_parse parse1.ypp ${CMAKE_CURRENT_BINARY_DIR}/parse1.cpp DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parse1.hpp)
bison_target(nocmodl_diffeq diffeq.ypp ${CMAKE_CURRENT_BINARY_DIR}/diffeq.cpp DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/diffeq.hpp)

add_custom_target(
  nocmodl_generated_files
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/lex.cpp" "${CMAKE_CURRENT_BINARY_DIR}/parse1.hpp" "${CMAKE_CURRENT_BINARY_DIR}/parse1.cpp"
  "${CMAKE_CURRENT_BINARY_DIR}/diffeq.hpp" "${CMAKE_CURRENT_BINARY_DIR}/diffeq.cpp")
      # add_dependencies(generated_source_files nocmodl_generated_files)
add_executable(nocmodl 
    consist.cpp
    deriv.cpp
    discrete.cpp
    init.cpp
    io.cpp
    kinetic.cpp
    list.cpp
    modl.cpp
    nocpout.cpp
    noccout.cpp
    parsact.cpp
    netrec_discon.cpp
    simultan.cpp
    solve.cpp
    symbol.cpp
    ../modlunit/units.cpp
    version.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/lex.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/parse1.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/diffeq.cpp")
cpp_cc_configure_sanitizers(TARGET nocmodl)
target_compile_definitions(nocmodl PRIVATE COMPILE_DEFINITIONS NMODL=1 CVODE=1)

# Otherwise the generated code in the binary directory does not find headers in the modlunit source
# directory and the source files in the source directory do not find generated headers in the binary
# directory. TODO: (see also coreneuron) fix adding a dependency on CLI11::CLI11 when CLI11 is a
# submodule. Right now this doesn't work because the CLI11 targets are not exported/installed but
# coreneuron-core is.
get_target_property(CLI11_HEADER_DIRECTORY CLI11::CLI11 INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(nocmodl PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" "${CLI11_HEADER_DIRECTORY}" "${PROJECT_SOURCE_DIR}/src")
if(NRN_NMODL_CXX_FLAGS)
  target_compile_options(nocmodl PRIVATE ${NRN_NMODL_CXX_FLAGS})
endif()

install(TARGETS nocmodl DESTINATION bin)
