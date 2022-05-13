# =============================================================================
# Platform specific settings
# =============================================================================

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(NRN_LINUX_BUILD TRUE)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(NRN_WINDOWS_BUILD TRUE)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(NRN_MACOS_BUILD TRUE)
endif()
