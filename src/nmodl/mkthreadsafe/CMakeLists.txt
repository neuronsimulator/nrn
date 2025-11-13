# =============================================================================
# mkthreadsafe executable
# =============================================================================

add_executable(mkthreadsafe main.cpp)

target_link_libraries(mkthreadsafe CLI11::CLI11 lexer printer util visitor)

# =============================================================================
# Install executable
# =============================================================================
install(TARGETS mkthreadsafe DESTINATION ${NMODL_INSTALL_DIR_SUFFIX}bin/)
