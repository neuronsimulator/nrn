#
# Add tests based on external repositories
#

# To work around CI failures that seem related to #894 we should run `mpiexec` not
# `/path/to/mpiexec`
set(MPIEXEC_NAME ${MPIEXEC_NAME})

add_subdirectory(ringtest)
add_subdirectory(testcorenrn)
