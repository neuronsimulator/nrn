#
# Add tests based on external repositories
#

# FindMPI broke backward compatibility with v3.19
if(${CMAKE_VERSION} VERSION_LESS "3.10")
  get_filename_component(MPIEXEC_NAME ${MPIEXEC} NAME)
else()
  get_filename_component(MPIEXEC_NAME ${MPIEXEC_EXECUTABLE} NAME)
endif()

# To work around CI failures that seem related to #894 we should run `mpiexec` not
# `/path/to/mpiexec`
set(MPIEXEC_NAME ${MPIEXEC_NAME})

add_subdirectory(ringtest)
add_subdirectory(testcorenrn)
