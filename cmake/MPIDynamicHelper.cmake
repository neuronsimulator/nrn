# =============================================================================
# Configure support for dynamic MPI initialization
# =============================================================================
# ~~~
# Verify that NRN_ENABLE_MPI_DYNAMIC is valid and determine an include
# directory for each MPI package building libnrnmpi_<mpipkg>.so.
# Depending on the MPIs used NRNMPI_INCLUDE_<mpipkg> will be defined.
# A number of other lists needed to construct libnrnmpi_<mpipkg>.so
# are also constructe. The lists are all parallel in the sense that
# corresponding elements are related to the same mpi installation.
# ~~~

set(NRN_MPI_BIN_LIST "" CACHE INTERNAL "" FORCE)
set(NRN_MPI_INCLUDE_LIST "" CACHE INTERNAL "" FORCE)
set(NRN_MPI_LIBNAME_LIST "" CACHE INTERNAL "" FORCE)
set(NRN_MPI_TYPE_LIST "" CACHE INTERNAL "" FORCE)


if(NRN_ENABLE_MPI)
  if(NRN_ENABLE_MPI_DYNAMIC)

    set(NRNMPI_DYNAMICLOAD 1)

    # compute the NRN_MPI_INCLUDE_LIST and NRN_MPI_BIN_LIST
    if("${NRN_MPI_DYNAMIC}" STREQUAL "") # use the MPI already found
      list(APPEND NRN_MPI_INCLUDE_LIST "${MPI_C_HEADER_DIR}")
      string(REGEX REPLACE "/[^/]*$" "/bin" foo "${MPI_C_HEADER_DIR}")
      list(APPEND NRN_MPI_BIN_LIST "${foo}")
    else() # find the mpi's in the ';' separated list of mpi bin folders.
      foreach(bdir ${NRN_MPI_DYNAMIC})
        list(APPEND NRN_MPI_BIN_LIST "${bdir}")
        string(REGEX REPLACE "/[^/]*$" "/include" foo "${bdir}")
        list(APPEND NRN_MPI_INCLUDE_LIST "${foo}")
      endforeach(bdir)
    endif()

    # compute the NRN_MPI_TYPE_LIST and NRN_MPI_LIBNAME_LIST
    foreach(idir ${NRN_MPI_INCLUDE_LIST})
      # ${idir}/mpi.h must exist
      if(NOT EXISTS "${idir}/mpi.h")
        message(FATAL_ERROR "${idir}/mpi.h does not exist")
      endif()

      # ~~~
      # Only know about openmpi, mpich, and ms-mpi.
      # ~~~
      execute_process(COMMAND grep -q "define OPEN_MPI  *1" ${idir}/mpi.h RESULT_VARIABLE result)
      if (result EQUAL 0)
        set(type "ompi")
      else()
        execute_process(COMMAND grep -q "define MPICH  *1" ${idir}/mpi.h RESULT_VARIABLE result)
        if (result EQUAL 0)
          set(type "mpich")
        else()
          execute_process(COMMAND grep -q "define MSMPI_VER " ${idir}/mpi.h RESULT_VARIABLE result)
          if (result EQUAL 0)
            set(type "msmpi")
          else()
            # Perhaps rather set the type to "unknown" and check for no duplicates?
            message(FATAL_ERROR "${idir}/mpi.h does not seem to be either openmpi, mpich, or msmpi")
          endif()
        endif()
      endif()
      list(APPEND NRN_MPI_TYPE_LIST "${type}")
      list(APPEND NRN_MPI_LIBNAME_LIST "nrnmpi_${type}")
    endforeach(idir)
  endif()
else()
  if(NRN_ENABLE_MPI_DYNAMIC)
    set(NRN_ENABLE_MPI_DYNAMIC "NO")
    message(WARNING "NRN_ENABLE_MPI_DYNAMIC can only be set if MPI is enabled (see NRN_ENABLE_MPI)")
  endif()
endif()
