# =============================================================================
# Parser executable
# =============================================================================

# lexer library links with all parser related files so no need to have parser as a separate library

if(NOT NMODL_AS_SUBPROJECT)
  add_executable(nmodl_parser main_nmodl.cpp)
  add_executable(c_parser main_c.cpp)
  add_executable(units_parser main_units.cpp)

  target_link_libraries(nmodl_parser CLI11::CLI11 lexer util)
  target_link_libraries(c_parser CLI11::CLI11 lexer util)
  target_link_libraries(units_parser CLI11::CLI11 util visitor lexer)
endif()

# =============================================================================
# Install executable
# =============================================================================
if(NOT NMODL_AS_SUBPROJECT)
  install(
    TARGETS nmodl_parser
    DESTINATION ${NMODL_INSTALL_DIR_SUFFIX}bin/parser
    CONFIGURATIONS Debug)
  install(
    TARGETS c_parser
    DESTINATION ${NMODL_INSTALL_DIR_SUFFIX}bin/parser
    CONFIGURATIONS Debug)
  install(
    TARGETS units_parser
    DESTINATION ${NMODL_INSTALL_DIR_SUFFIX}bin/parser
    CONFIGURATIONS Debug)
endif()
