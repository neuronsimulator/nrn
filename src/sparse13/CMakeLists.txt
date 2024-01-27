add_library(sparse13 STATIC spalloc.cpp spbuild.cpp spfactor.cpp spoutput.cpp spsolve.cpp
                            sputils.cpp)
set_property(TARGET sparse13 PROPERTY POSITION_INDEPENDENT_CODE ON)
