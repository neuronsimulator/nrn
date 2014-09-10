include(FindSLURM)
include(TestScriptUtils)

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
    set(${exeprefix} ${SLURM_SRUN_COMMAND} -n ${nproc} ${SLURM_ACCOUNT_PARAM} PARENT_SCOPE) 
elseif(MPIEXEC)
    set(${exeprefix} ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${nproc} ${MPIEXEC_PREFLAGS} PARENT_SCOPE) 
endif()
endfunction()

# Boost test does not like -Wshadow
remove_word(CMAKE_CXX_FLAGS "-Wshadow")

add_subdirectory (unit/endian)
add_subdirectory (integration)
#add_subdirectory (regression)
add_subdirectory (validation)
