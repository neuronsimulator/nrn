# =============================================================================
# Platform specific settings
# =============================================================================
if(CMAKE_HOST_WIN32)
    set(NRN_WINDOWS_BUILD TRUE)
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(NRN_MACOS_BUILD TRUE)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "CYGWIN")
  set(CYGWIN 1)
endif()
