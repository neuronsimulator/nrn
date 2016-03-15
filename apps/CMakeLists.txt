include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/coreneuron ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_BINARY_DIR}/coreneuron)

# The mechanism registration function will be defined depending on whether we
# are using neurodamus list of mechanisms
if(ADDITIONAL_MECHS)
    add_definitions(-DADDITIONAL_MECHS)
endif()

FILE(GLOB coreneuron_exec_src "*.c*")

add_executable(coreneuron_exec ${coreneuron_exec_src})

set_target_properties(coreneuron_exec PROPERTIES
                            OUTPUT_NAME "coreneuron_exec"
                            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

target_link_libraries(coreneuron_exec coreneuron ${MPI_C_LIBRARIES} ${MPI_CXX_LIBRARIES})

install(TARGETS coreneuron_exec
         DESTINATION ${BIN_INSTALL_DIR}/)

