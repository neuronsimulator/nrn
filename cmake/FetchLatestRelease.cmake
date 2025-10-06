# =============================================================================
# Fetch the latest release JSON from GitHub API, and put the information into the variables
# `NRN_WINDOWS_INSTALLER_URL` and `NRN_MACOS_INSTALLER_URL`. Note that, according to the GitHub API
# docs:
#
# > The latest release is the most recent non-prerelease, non-draft release, sorted by the
# created_at attribute. The created_at attribute is the date of the commit used for the release, and
# not the date when the release was drafted or published.
#
# which means that after we make a release on GitHub, this function will always that one, regardless
# of which version tag it uses. This is good because then we do not need to worry about version
# (x-1).y (say, a bugfix for an old release) having the wrong links; if it has chronologically been
# released the latest, the API will fetch that one.
# =============================================================================
function(fetch_latest_release)
  if(CMAKE_VERSION VERSION_LESS "3.19")
    message(FATAL_ERROR "CMake 3.19 or above is required for building the docs")
  endif()
  set(GITHUB_USER "neuronsimulator")
  set(GITHUB_REPO "nrn")
  set(GITHUB_API_VERSION "2022-11-28")
  set(API_URL "https://api.github.com/repos/${GITHUB_USER}/${GITHUB_REPO}/releases/latest")
  set(JSON_FILE "${CMAKE_CURRENT_BINARY_DIR}/latest_release.json")
  set(WINDOWS_INSTALLER_PATTERN ".*\\.w64-mingw.*setup\\.exe$")
  set(MACOS_INSTALLER_PATTERN ".*macosx.*\\.pkg$")

  message(STATUS "Fetching latest release info from ${API_URL}")
  file(
    DOWNLOAD "${API_URL}" "${JSON_FILE}"
    STATUS DOWNLOAD_STATUS
    HTTPHEADER "X-GitHub-Api-Version: ${GITHUB_API_VERSION}"
    TIMEOUT 10)

  list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
  if(NOT STATUS_CODE EQUAL 0)
    message(FATAL_ERROR "Failed to download release information")
  endif()

  file(READ "${JSON_FILE}" JSON_CONTENT)

  string(JSON ASSETS_LENGTH ERROR_VARIABLE JSON_ERROR LENGTH "${JSON_CONTENT}" "assets")
  if(JSON_ERROR)
    message(FATAL_ERROR "Failed to parse JSON: ${JSON_ERROR}")
  endif()

  message(DEBUG "Found ${ASSETS_LENGTH} assets in latest release")

  set(WINDOWS_INSTALLER_URL "")
  set(MACOS_INSTALLER_URL "")

  math(EXPR LAST_INDEX "${ASSETS_LENGTH} - 1")
  foreach(INDEX RANGE 0 ${LAST_INDEX})
    string(
      JSON
      ASSET_NAME
      GET
      "${JSON_CONTENT}"
      "assets"
      ${INDEX}
      "name")

    string(
      JSON
      ASSET_URL
      GET
      "${JSON_CONTENT}"
      "assets"
      ${INDEX}
      "browser_download_url")

    if(ASSET_NAME MATCHES "${WINDOWS_INSTALLER_PATTERN}")
      set(WINDOWS_INSTALLER_URL "${ASSET_URL}")
      message(STATUS "Found Windows installer: ${ASSET_NAME}")
      message(STATUS "  URL: ${ASSET_URL}")
    endif()

    if(ASSET_NAME MATCHES "${MACOS_INSTALLER_PATTERN}")
      set(MACOS_INSTALLER_URL "${ASSET_URL}")
      message(STATUS "Found macOS installer: ${ASSET_NAME}")
      message(STATUS "  URL: ${ASSET_URL}")
    endif()
  endforeach()

  # ===============================================
  # Verify we found both installers
  #
  # TODO should this issue a `FATAL_ERROR` instead?
  # ===============================================
  if(NOT WINDOWS_INSTALLER_URL)
    message(WARNING "Windows installer not found in latest release")
  endif()

  if(NOT MACOS_INSTALLER_URL)
    message(WARNING "macOS installer not found in latest release")
  endif()

  set(NRN_WINDOWS_INSTALLER_URL
      "${WINDOWS_INSTALLER_URL}"
      PARENT_SCOPE)
  set(NRN_MACOS_INSTALLER_URL
      "${MACOS_INSTALLER_URL}"
      PARENT_SCOPE)
endfunction()
