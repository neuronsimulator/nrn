include_directories(utils/randoms)

set(CORENEURON_PUBLIC_HEADERS
${COMMON_INCLUDES}
nrnconf.h
nrnoc/md1redef.h
nrnoc/md2redef.h
nrnoc/membfunc.h
nrnoc/multicore.h
nrnoc/nrnoc_ml.h
utils/randoms/nrnran123.h
utils/endianness.h
utils/swap_endian.h
utils/memory_utils.h
utils/sdprintf.h
mech/cfile/scoplib.h
)

set(CORENEURON_HEADERS
nrnomp/nrnomp.h
nrnmpi/mpispike.h
nrnmpi/nrnmpidec.h
nrnmpi/nrnmpi_def_cinc.h
nrnmpi/nrnmpi.h
nrnmpi/nrnmpi_impl.h
nrnmpi/nrnmpiuse.h
nrnoc/membdef.h
nrnoc/nrnoc_decl.h
nrnoc/nrnpthread.h
nrniv/ivocvect.h
nrniv/netcon.h
nrniv/netcvode.h
nrniv/nrn_assert.h
nrniv/nrn_datareader.h
nrniv/nrn_stats.h
nrniv/nrnoptarg.h
nrniv/nrniv_decl.h
nrniv/nrniv_mf.h
nrniv/nrnmutdec.h
nrniv/output_spikes.h
nrniv/prcellstate.cpp
nrniv/sptbinq.h
nrniv/vrecitem.h
mech/cfile/cabvars.h
)

set(CORENEURON_SOURCES
${COMMON_SOURCES}
nrnomp/nrnomp.c
nrnmpi/nrnmpi.c
nrnmpi/mpispike.c
nrnoc/capac.c
nrnoc/eion.c
nrnoc/finitialize.c
nrnoc/fadvance_core.c
nrnoc/solve_core.c
nrnoc/treeset_core.c
nrnoc/nrnoc_aux.c
nrnoc/register_mech.c
nrnoc/multicore.c
nrnoc/nrntimeout.c
nrniv/coreneuron_main.cpp
nrniv/cvodestb.cpp
nrniv/ivocvect.cpp
nrniv/main1.cpp
nrniv/netpar.cpp
nrniv/netcvode.cpp
nrniv/tqueue.cpp
nrniv/mk_mech.cpp
nrniv/nrn_datareader.cpp
nrniv/nrn_setup.cpp
nrniv/nrn_stats.cpp
nrniv/nrnoptarg.cpp
nrniv/output_spikes.cpp
nrniv/patternstim.cpp
nrniv/vrecord.cpp
utils/randoms/nrnran123.c
utils/memory_utils.cpp
utils/sdprintf.cpp
)

# Compile and include MOD files

find_program(MOD2C mod2c_core)
if(MOD2C STREQUAL "MOD2C-NOTFOUND")
message(FATAL_ERROR "Unable to locate mod2c_core executable")
endif()

# mod2c_core wants MODLUNIT set to point to its units file,
# but CMake does not support setting an environment for custom_commands.

get_filename_component(mod2c_install_prefix ${MOD2C} DIRECTORY)
find_file(MOD2C_UNITS nrnunits.lib HINT "${mod2c_install_prefix}" PATH_SUFFIXES "share")
if(NOT MOD2C_UNITS STREQUAL "MOD2C_UNITS-NOTFOUND")
set(MOD2C env "MODLUNIT=${MOD2C_UNITS}" ${MOD2C})
endif()

# Macro sets up build rule for .c files from .mod files.
# Parameters:
#    name     An arbitrary name to keep track of output .c files
#    input    The path to the mod file
#
# Because mod2c_core wants to write the .c file in the same directory
# as the mod file, we copy the mod file to the binary directory first
#
# The macro appends the names of the output files to MOD2C_${name}_OUTPUTS
# and the names of the mod files (without directories) to MOD2C_${name}_MODS

macro(mod2c_target name input)
    get_filename_component(mod2c_source_ ${input} ABSOLUTE)
    get_filename_component(mod2c_modname_ ${input} NAME)
    string(REGEX REPLACE "\\.mod$" ".c" mod2c_cname_ "${mod2c_modname_}")

    set(mod2c_output_ "${CMAKE_CURRENT_BINARY_DIR}/${mod2c_cname_}")
    list(APPEND MOD2C_${name}_OUTPUTS "${mod2c_output_}")
    list(APPEND MOD2C_${name}_MODS "${mod2c_modname_}")

    add_custom_command(OUTPUT "${mod2c_output_}"
      DEPENDS ${input}
      COMMAND ${CMAKE_COMMAND} -E copy "${mod2c_source_}" "${CMAKE_CURRENT_BINARY_DIR}"
      COMMAND ${MOD2C} "${mod2c_modname_}"
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
endmacro()

# Macro mod2c_from_file attempts to add modules specified in a file.
# Parameters:
#     name        Key for mod2c_target
#     modlist     Path to the file containing the list of mod files.
#     searchpath  List of directories to search for mod files.
#
# This has to be a macro in order to properly accumulate the MOD2C_xxx_OUTPUTS
# variables. Go CMake! You rock!

macro(mod2c_from_file name modlist searchpath)
    if((NOT EXISTS "${modlist}") OR (IS_DIRECTORY "${modlist}"))
        message(WARNING "Unable to read MOD list file: ${modlist}")
    else()
	unset(tmp_path_ CACHE)
	file(READ "${modlist}" mods_)
	string(REGEX REPLACE "[ \t\n]+" ";" mods_ "${mods_}")
	list(REMOVE_ITEM mods_ "")

	foreach(mod_ ${mods_})
	    unset(modpath_)
	    if(NOT IS_ABSOLUTE "${mod_}")
		find_path(tmp_path_ "${mod_}" PATH ${searchpath} NO_DEFAULT_PATH)
		if(tmp_path_)
		    set(modpath_ "${tmp_path_}/${mod_}")
		endif()
		unset(tmp_path_ CACHE)
	    else()
		set(modpath_ "${mod_}")
	    endif()
	    if((NOT EXISTS "${modpath_}") OR (IS_DIRECTORY "${modpath_}"))
		unset(modpath_)
	    endif()

	    if(modpath_)
		mod2c_target(${name} "${modpath_}")
	    else()
		message(WARNING "Unable to locate MOD file: ${mod_}")
	    endif()
	endforeach()
    endif()
endmacro()

# Standard MOD files are always built

set(STDMODLIBPATH "${CMAKE_CURRENT_SOURCE_DIR}/mech/modfile")
set(MECH_STDMODS expsyn.mod  hh.mod  netstim.mod  passive.mod  pattern.mod stim.mod)

foreach(MOD ${MECH_STDMODS})
  mod2c_target(STDMECH "${STDMODLIBPATH}/${MOD}")
endforeach()

# Additional MOD files: these can come from Neurodamus and/or from a configure-time
# variable (see CORENEURON_ADDITIONAL_MECHS below)

set(MECH_FILE_LISTS)

# Neurodamus mechanisms are built by default

option(CORENEURON_NEURODAMUS_MECHS "Compile CoreNeuron with all available mechanisms from Neurodamus" ON)

if(CORENEURON_NEURODAMUS_MECHS)
  set(MODLIBPATH "${CMAKE_CURRENT_SOURCE_DIR}/mech/neurodamus/lib/modlib")
  set(MODLISTFILE "${CMAKE_CURRENT_SOURCE_DIR}/mech/coreneuron_modlist.txt")

  if(EXISTS "${MODLISTFILE}")
    list(APPEND MECH_FILE_LISTS "${MODLISTFILE}")
    mod2c_from_file(OPTMECH "${MODLISTFILE}" "${MODLIBPATH}")
  else()
    message(FATAL_ERROR "Cannot find coreneuron_modlist.txt listing .mod files to include. Consider setting CORENEURON_NEURODAMUS_MECHS to OFF")
  endif()
endif()

# Additional optional mechanisms specified in CORENEURON_ADDITIONAL_MECHS
# and CORENEURON_ADDITIONAL_MECHPATH (set as a PATH cache variable, but
# can in fact be a list of paths.)

set(CORENEURON_ADDITIONAL_MECHPATH "" CACHE PATH "Search path for optional additional mechanism MOD files")
set(CORENEURON_ADDITIONAL_MECHS "" CACHE FILEPATH "File containing list of additional mechanism MOD files")

if(EXISTS ${CORENEURON_ADDITIONAL_MECHS})
    list(APPEND MECH_FILE_LISTS "${CORENEURON_ADDITIONAL_MECHS}")
    mod2c_from_file(OPTMECH "${CORENEURON_ADDITIONAL_MECHS}" "${CORENEURON_ADDITIONAL_MECHPATH}")
endif()

# For 'non-standard' mod files, need to generate registering function in mod_func.c

set(MOD_FUNC_C "${CMAKE_CURRENT_BINARY_DIR}/mod_func.c")
set(MOD_FUNC_C_PL "${CMAKE_CURRENT_SOURCE_DIR}/mech/mod_func.c.pl")

add_custom_command(OUTPUT "${MOD_FUNC_C}"
    DEPENDS ${MECH_FILE_LISTS} "${MOD_FUNC_C_PL}"
    COMMAND perl "${MOD_FUNC_C_PL}" ${MOD2C_OPTMECH_MODS} > "${MOD_FUNC_C}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

# ... and add a reconfigure dependency on the files that listed them

foreach(depfile ${MECH_FILE_LISTS})
    configure_file("${depfile}" "${CMAKE_CURRENT_BINARY_DIR}/dummy_reconfigure_dep")
endforeach()

# Add generated sources from MOD files

set(GENERATED_MECH_C_FILES ${MOD_FUNC_C} ${MOD2C_STDMECH_OUTPUTS} ${MOD2C_OPTMECH_OUTPUTS})
set_source_files_properties(${GENERATED_MECH_C_FILES} PROPERTIES COMPILE_FLAGS "-Wno-error")

set(CORENEURON_SOURCES ${CORENEURON_SOURCES} ${GENERATED_MECH_C_FILES})


common_library(coreneuron)

install(FILES ${CMAKE_BINARY_DIR}/coreneuron/CMakeFiles/coreneuron.dir/nrniv/coreneuron_main.cpp.o DESTINATION lib)
