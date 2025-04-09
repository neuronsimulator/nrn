set(SOURCE_FILES
    cabpump.mod
    cachan1.mod
    camchan.mod
    capmpr.mod
    capump.mod
    invlfire.mod
    khhchan.mod
    nacaex.mod
    nachan.mod
    release.mod)

if(WIN32)
  file(MAKE_DIRECTORY "${NRN_BUILD_SHARE_DIR}/demo/release")
endif()

file(COPY ${SOURCE_FILES} DESTINATION "${NRN_BUILD_SHARE_DIR}/demo/release")
