# Copyright (c) 2017, Blue Brain Project
# All rights reserved.

# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.

## Portability check on Cray system
## This is tested on Piz Daint, Theta and Titan

if(IS_DIRECTORY "/opt/cray")
    set(CRAY_SYSTEM TRUE)
endif()

if(CRAY_SYSTEM)

    # default build type is static for cray
    if(NOT DEFINED COMPILE_LIBRARY_TYPE)
        set(COMPILE_LIBRARY_TYPE "STATIC")
    endif()

    ## Cray wrapper take care of everything!
	set(MPI_LIBRARIES "")
	set(MPI_C_LIBRARIES "")	
	set(MPI_CXX_LIBRARIES "")

    # instead of -rdynamic, cray wrapper needs either -dynamic or -static(default)
    # also cray compiler needs fPIC flag
    if(COMPILE_LIBRARY_TYPE STREQUAL "SHARED")
        set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "-dynamic")

        # TODO: add Cray compiler flag configurations in CompilerFlagsHelpers.cmake
        # Cray compilers need PIC flag
        if(CMAKE_C_COMPILER_IS_CRAY)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
        endif()

    else()
        set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
    endif()

endif()
