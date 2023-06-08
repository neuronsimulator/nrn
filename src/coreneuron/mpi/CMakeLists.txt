# for non-dynamic mpi mode just build object files
if(NRN_ENABLE_MPI)
  set(MPI_LIB_FILES "lib/mpispike.cpp" "lib/nrnmpi.cpp")
  if(NRN_ENABLE_MPI_DYNAMIC)
    # this is where we handle dynamic mpi library build store mpi library targets that will be built
    list(APPEND corenrn_mpi_targets "")

    # ~~~
    # if coreneuron is build as part of neuron then check if NEURON has created
    # list of libraries that needs to be built. We use neuron cmake variables here because
    # we don't need to duplicate CMake code into coreneuron (we want to have unified cmake
    # project soon). In the absense of neuron just build a single library libcorenrn_mpi.
    # This is mostly used for the testing.
    #
    # from neuron we know how many different libraries needs to be built, their names
    # include paths to be used for building shared libraries. Iterate through those
    # and build separate library for each MPI distribution. For example, following
    # libraries are created:
    # - libcorenrn_mpich.so
    # - libcorenrn_ompi.so
    # - libcorenrn_mpt.so
    # ~~~
    list(LENGTH NRN_MPI_LIBNAME_LIST _num_mpi)
    math(EXPR num_mpi "${_num_mpi} - 1")
    foreach(val RANGE ${num_mpi})
      list(GET NRN_MPI_INCLUDE_LIST ${val} include)
      list(GET NRN_MPI_LIBNAME_LIST ${val} libname)

      add_library(core${libname}_lib SHARED ${MPI_LIB_FILES})
      target_link_libraries(core${libname}_lib ${CORENRN_CALIPER_LIB})
      target_include_directories(
        core${libname}_lib
        PUBLIC ${include}
        PRIVATE ${PROJECT_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/generated)

      # ~~~
      # TODO: somehow mingw requires explicit linking. This needs to be verified
      # when we will test coreneuron on windows.
      # ~~~
      if(MINGW) # type msmpi only
        add_dependencies(core${libname}_lib coreneuron-core)
        target_link_libraries(core${libname}_lib ${MPI_C_LIBRARIES} coreneuron-core)
      endif()
      set_property(TARGET core${libname}_lib PROPERTY OUTPUT_NAME core${libname})
      list(APPEND corenrn_mpi_targets "core${libname}_lib")
    endforeach(val)

    set_target_properties(
      ${corenrn_mpi_targets}
      PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
                 LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
                 POSITION_INDEPENDENT_CODE ON)
    install(TARGETS ${corenrn_mpi_targets} DESTINATION lib)
  else(NRN_ENABLE_MPI_DYNAMIC)
    add_library(${CORENRN_MPI_LIB_NAME} OBJECT ${MPI_LIB_FILES})
    target_include_directories(
      ${CORENRN_MPI_LIB_NAME} PRIVATE ${MPI_C_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/src
                                      ${CMAKE_BINARY_DIR}/generated)
    target_link_libraries(${CORENRN_MPI_LIB_NAME} ${CORENRN_CALIPER_LIB})
    set_property(TARGET ${CORENRN_MPI_LIB_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
  endif(NRN_ENABLE_MPI_DYNAMIC)
endif(NRN_ENABLE_MPI)
