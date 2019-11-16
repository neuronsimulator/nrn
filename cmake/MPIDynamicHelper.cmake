# =============================================================================
# Configure support for dynamic MPI initialization
# =============================================================================
# TODO : verify that NRN_ENABLE_MPI_DYNAMIC is valid and determine an include
# directory for each MPI package building libnrnmpi_<mpipkg>.so.
# Depending on the MPIs used NRNMPI_INCLUDE_<mpipkg> will be defined.

if(NRN_ENABLE_MPI)
  if(NRN_ENABLE_MPI_DYNAMIC)
    if("${NRN_MPI_DYNAMIC}" STREQUAL "")
      set(NRNMPI_DYNAMICLOAD 1)
    else()
      message(FATAL_ERROR "NRN_ENABLE_MPI_DYNAMIC doesn't support multiple MPI yet")
    endif()
  endif()
else()
  if(NRN_ENABLE_MPI_DYNAMIC)
    set(NRN_ENABLE_MPI_DYNAMIC "NO")
    message(WARNING "NRN_ENABLE_MPI_DYNAMIC can only be set if MPI is enable (see NRN_ENABLE_MPI)")
  endif()
endif()
