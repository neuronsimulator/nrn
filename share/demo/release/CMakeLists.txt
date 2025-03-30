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
file(COPY ${SOURCE_FILES} DESTINATION ${PROJECT_BINARY_DIR}/share/nrn/demo/release)
