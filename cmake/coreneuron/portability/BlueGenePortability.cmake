# Copyright (c) 2016, Blue Brain Project
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


##
## Portability check on Blue Gene Q environment

if(IS_DIRECTORY "/bgsys")
    set(BLUEGENE TRUE)
endif()


if(BLUEGENE)
	# define library type to static on BGQ
	set(COMPILE_LIBRARY_TYPE "STATIC")

	## Blue Gene/Q do not support linking with MPI library when compiled with mpicc wrapper
        ## we disable any MPI_X_LIBRARY linking and rely on mpicc wrapper
	set(MPI_LIBRARIES "")
	set(MPI_C_LIBRARIES "")
	set(MPI_CXX_LIBRARIES "")

	## static linking need to be forced on BlueGene
	# Boost need a bit of tuning parameters for static linking
	set(Boost_NO_BOOST_CMAKE TRUE)
        set(Boost_USE_STATIC_LIBS TRUE)

    CHECK_INCLUDE_FILES("spi/include/kernel/memory.h" have_memory_h)
    if(have_memory_h)
        add_definitions("-DHAVE_MEMORY_H")
    endif()

else()

if(NOT DEFINED COMPILE_LIBRARY_TYPE)
    set(COMPILE_LIBRARY_TYPE "SHARED")
endif()

endif()




