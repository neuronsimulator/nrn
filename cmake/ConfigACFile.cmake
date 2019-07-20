######## COPIED FROM TOP LEVEL
# needed in several subdirectoy configurations
add_definitions(-DHAVE_CONFIG_H)

set (prefix ${CMAKE_INSTALL_PREFIX})
set (host_cpu ${CMAKE_SYSTEM_PROCESSOR})
set (exec_prefix ${prefix})
set (bindir \${exec_prefix}/bin)
set (libdir \${exec_prefix}/lib)
set (PACKAGE "\"nrn\"")

set(USING_CMAKE_FALSE "#")
set(USING_CMAKE_TRUE "")
##########################
