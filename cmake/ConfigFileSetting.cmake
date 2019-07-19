# =============================================================================
# Set variables used in various autoconf based scripts
# =============================================================================
# indicate nmodl config is used
add_definitions(-DHAVE_CONFIG_H)

# install prefix, library paths, make sure to escape '$'
set (prefix ${CMAKE_INSTALL_PREFIX})
set (host_cpu ${CMAKE_SYSTEM_PROCESSOR})
set (exec_prefix ${prefix})
set (bindir \${exec_prefix}/bin)
set (libdir \${exec_prefix}/lib)

# name for libraries
nrn_set_string (PACKAGE "nrn")

# =============================================================================
# Comment or empty character to enable/disable cmake specific settings
# =============================================================================
# adding `#` makes a comment in the python whereas empty enable code
set(USING_CMAKE_FALSE "#")
set(USING_CMAKE_TRUE "")
