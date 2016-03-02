include(FindSLURM)
include(TestScriptUtils)
#include(CommonCTest)

macro(remove_word var word)
string(REPLACE " " ";" ${var} "${${var}}")
list(REMOVE_ITEM ${var} "${word}")
string(REPLACE ";" " " ${var} "${${var}}")
endmacro()

function(make_mpi_exeprefix nproc exeprefix)
if(SLURM_FOUND AND SLURM_SACCTMGR_COMMAND)
    if("$ENV{SLURM_ACCOUNT}" STREQUAL "")
	execute_process(COMMAND ${SLURM_SACCTMGR_COMMAND} -n list user format=DefaultAccount user=$ENV{USER}
			COMMAND tr -d [:space:]
			OUTPUT_VARIABLE DFLTACCT
			ERROR_QUIET)
        if (DFLTACCT)
	    set(SLURM_ACCOUNT_PARAM "--account=${DFLTACCT}")
        endif()
    endif()
    set(${exeprefix} ${SLURM_SRUN_COMMAND} -n ${nproc} ${SLURM_OPTIONS} ${SLURM_ACCOUNT_PARAM} PARENT_SCOPE) 
elseif(MPIEXEC)
    set(${exeprefix} ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${nproc} ${MPIEXEC_PREFLAGS} PARENT_SCOPE) 
endif()
endfunction()

# Boost test does not like -Wshadow
remove_word(CMAKE_CXX_FLAGS "-Wshadow")

add_subdirectory(unit/endian)
add_subdirectory(unit/sdprintf)
add_subdirectory(unit/mechbuild)
add_subdirectory(unit/omp)
add_subdirectory(unit/cmdline_interface)
add_subdirectory(unit/alignment)
add_subdirectory(unit/queueing)
add_subdirectory(integration)
add_subdirectory(regression)
add_subdirectory(validation)

if(CORENEURON_PRODUCTION_TESTS)
    add_subdirectory(production)
endif()
