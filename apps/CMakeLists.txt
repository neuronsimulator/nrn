set(COREBLURON_EXEC_HEADERS)
set(COREBLURON_EXEC_SOURCES main.cpp)
set(COREBLURON_EXEC_LINK_LIBRARIES corebluron hdf5)

# The mechanism registration function will be defined depending on whether we
# are using neurodamus list of mechanisms
if(COREBLURON_NEURODAMUS_MECHS)
  add_definitions(-DNEURODAMUS_MECHS)
endif()

common_application(corebluron_exec)
