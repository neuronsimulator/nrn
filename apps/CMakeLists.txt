set(CORENEURON_EXEC_HEADERS)
set(CORENEURON_EXEC_SOURCES main.cpp)
set(CORENEURON_EXEC_LINK_LIBRARIES coreneuron)

# The mechanism registration function will be defined depending on whether we
# are using neurodamus list of mechanisms
if(CORENEURON_NEURODAMUS_MECHS)
  add_definitions(-DNEURODAMUS_MECHS)
endif()

common_application(coreneuron_exec)
