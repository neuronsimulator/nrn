include(FindSLURM)
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

add_subdirectory (integration)
#add_subdirectory (regression)
add_subdirectory (validation)
