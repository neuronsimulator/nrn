# =============================================================================
# Configure support for dynamic MPI initialization
# =============================================================================
# TODO : verify that NRN_ENABLE_MPI_DYNAMIC is valid and determine an include
# directory for each MPI package building libnrnmpi_<mpipkg>.so.
# Depending on the MPIs used NRNMPI_INCLUDE_<mpipkg> will be defined.

if (NOT NRN_ENABLE_MPI AND NOT NRN_ENABLE_MPI_DYNAMIC MATCHES "NO")
  set(NRN_ENABLE_MPI_DYNAMIC "NO")
  message(WARNING "NRN_ENABLE_MPI_DYNAMIC can be set if MPI is enable (see NRN_ENABLE_MPI)")
endif()

if (NRN_ENABLE_MPI_DYNAMIC MATCHES "NO")
  # do nothing
elseif (NRN_ENABLE_MPI_DYNAMIC MATCHES "YES")
  set(NRNMPI_DYNAMICLOAD 1)
else()
  message(FATAL_ERROR "NRN_ENABLE_MPI_DYNAMIC doesn't support multiple MPI yet, set YES or NO")
endif()
