add_library(
        sparse13 STATIC
        spalloc.cpp
        spbuild.cpp
        spfactor.cpp
        spoutput.cpp
        spsolve.cpp
        sputils.cpp
        cspalloc.cpp
        cspbuild.cpp
        cspfactor.cpp
        cspoutput.cpp
        cspsolve.cpp
        csputils.cpp
)
set_property(TARGET sparse13 PROPERTY POSITION_INDEPENDENT_CODE ON)
