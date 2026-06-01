# =============================================================================
# Platform specific settings
# =============================================================================

# Get the variant (flavor) of a Linux distribution (debian, fedora, etc.) by parsing the
# `/etc/os-release` file and looking for the value of the `ID` variable
function(get_linux_variant OUT_VAR)
  set(OS_RELEASE_FILE "/etc/os-release")
  if(EXISTS "${OS_RELEASE_FILE}")
    file(STRINGS "${OS_RELEASE_FILE}" OS_RELEASE_CONTENT)
    set(found_id FALSE)
    foreach(line IN LISTS OS_RELEASE_CONTENT)
      string(REGEX MATCH "^ID=.*" ID_LINE "${line}")
      if(ID_LINE)
        # Strip 'ID=' and any surrounding quotes
        string(REGEX REPLACE "^ID=\"?([^\"]*)\"?.*" "\\1" ID_VALUE "${ID_LINE}")
        # Export the result
        set(${OUT_VAR}
            "${ID_VALUE}"
            PARENT_SCOPE)
        set(found_id TRUE)
        break()
      endif()
    endforeach()
    if(NOT found_id)
      set(${OUT_VAR}
          "linux"
          PARENT_SCOPE)
    endif()
  else()
    # File does not exist
    set(${OUT_VAR}
        ""
        PARENT_SCOPE)
  endif()
endfunction()

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(NRN_LINUX_BUILD TRUE)
  get_linux_variant(NRN_LINUX_VARIANT)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(NRN_WINDOWS_BUILD TRUE)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(NRN_MACOS_BUILD TRUE)
endif()
