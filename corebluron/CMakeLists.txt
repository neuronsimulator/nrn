include_directories(utils/randoms)

set(COREBLURON_PUBLIC_HEADERS
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

set(COREBLURON_HEADERS
nrnmpi/mpispike.h
nrnmpi/nrnmpidec.h
nrnmpi/nrnmpi_def_cinc.h
nrnmpi/nrnmpi.h
nrnmpi/nrnmpi_impl.h
nrnmpi/nrnmpiuse.h
nrnoc/membdef.h
nrnoc/nrnoc_decl.h
nrnoc/nrnpthread.h
nrniv/arraypool.h
nrniv/htlist.h
nrniv/ivlist.h
nrniv/ivocvect.h
nrniv/ivtable.h
nrniv/netcon.h
nrniv/netcvode.h
nrniv/nrn_assert.h
nrniv/nrnhash_alt.h
nrniv/nrnoptarg.h
nrniv/nrnhash.h
nrniv/nrniv_decl.h
nrniv/nrniv_mf.h
nrniv/nrnmutdec.h
nrniv/nrn_datareader.h
nrniv/output_spikes.h
nrniv/pool.h
nrniv/prcellstate.cpp
nrniv/sptbinq.h
nrniv/sptree.h
nrniv/structpool.h
nrniv/tqueue.h
nrniv/vrecitem.h
mech/cfile/cabvars.h
)

set(COREBLURON_SOURCES
${COMMON_SOURCES}
nrnmpi/nrnmpi.c
nrnmpi/mpispike.c
nrnoc/capac.c
nrnoc/eion.c
nrnoc/finitialize.c
nrnoc/fadvance_core.c
nrnoc/hocstub.c
nrnoc/solve_core.c
nrnoc/treeset_core.c
nrnoc/nrnoc_aux.c
nrnoc/register_mech.c
nrnoc/multicore.c
nrnoc/nrntimeout.c
nrniv/corebluron_main.cpp
nrniv/cvodestb.cpp
nrniv/main1.cpp
nrniv/netpar.cpp
nrniv/netcvode.cpp
nrniv/tqueue.cpp
nrniv/htlist.cpp
nrniv/ivlistimpl.cpp
nrniv/ivvect.cpp
nrniv/mk_mech.cpp
nrniv/nrn_setup.cpp
nrniv/nrn_datareader.cpp
nrniv/nrnoptarg.cpp
nrniv/output_spikes.cpp
nrniv/patternstim.cpp
nrniv/vrecord.cpp
utils/randoms/nrnran123.c
utils/memory_utils.cpp
utils/sdprintf.cpp
)

option(COREBLURON_NEURODAMUS_MECHS "Compile CoreBluron with all available mechanisms from Neurodamus" ON)
if(COREBLURON_NEURODAMUS_MECHS)
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
  # Because nocmodl_core wants to write the .c file in the same directory
  # as the mod file, we copy the mod file to the binary directory first
  #
  # The macro appends the names of the output files to MOD2C_${name}_OUTPUTS

  macro(mod2c_target name input)
    get_filename_component(mod2c_source_ ${input} ABSOLUTE)
    get_filename_component(mod2c_modname_ ${input} NAME)
    string(REGEX REPLACE "\\.mod$" ".c" mod2c_cname_ "${mod2c_modname_}")

    set(mod2c_output_ "${CMAKE_CURRENT_BINARY_DIR}/${mod2c_cname_}")
    list(APPEND MOD2C_${name}_OUTPUTS "${mod2c_output_}")

    add_custom_command(OUTPUT "${mod2c_output_}"
      DEPENDS ${input}
      COMMAND ${CMAKE_COMMAND} -E copy "${mod2c_source_}" "${CMAKE_CURRENT_BINARY_DIR}"
      COMMAND ${MOD2C} "${mod2c_modname_}"
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
  endmacro()

  set(MODLIBPATH "${CMAKE_CURRENT_SOURCE_DIR}/mech/neurodamus/lib/modlib")
  set(STDMODLIBPATH "${CMAKE_CURRENT_SOURCE_DIR}/mech/modfile")

  set(MECH_MODS_STD expsyn.mod  hh.mod  netstim.mod  passive.mod  pattern.mod stim.mod)

  set(MODLISTFILE "${CMAKE_CURRENT_SOURCE_DIR}/mech/coreneuron_modlist.txt")
  if(EXISTS "${MODLISTFILE}")
    file(READ "${MODLISTFILE}" MECH_MODS)
    string(REGEX REPLACE "[ \t\n]+" ";" MECH_MODS "${MECH_MODS}")
    list(REMOVE_ITEM MECH_MODS "")
  else()
    message(FATAL_ERROR "Cannot find coreneuron_modlist.txt listing .mod files to include. Consider setting COREBLURON_NEURODAMUS_MECHS to OFF")
  endif()
  
  foreach(MOD ${MECH_MODS})
      mod2c_target(MECH "${MODLIBPATH}/${MOD}")
  endforeach()

  foreach(MOD ${MECH_MODS_STD})
      mod2c_target(MECH "${STDMODLIBPATH}/${MOD}")
  endforeach()

  # Also need to generate mod_func.c from module list
  set(MOD_FUNC_C "${CMAKE_CURRENT_BINARY_DIR}/mod_func.c")
  set(MOD_FUNC_C_PL "${CMAKE_CURRENT_SOURCE_DIR}/mech/mod_func.c.pl")

  add_custom_command(OUTPUT "${MOD_FUNC_C}"
	DEPENDS "${MODLISTFILE}" "${MOD_FUNC_C_PL}"
    COMMAND perl "${MOD_FUNC_C_PL}" ${MODLISTFILE} > "${MOD_FUNC_C}"
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

  set(COREBLURON_SOURCES ${COREBLURON_SOURCES}
    ${MOD_FUNC_C}
    ${MOD2C_MECH_OUTPUTS})

  set(MECH_FILES ${MECH_FILES}
    ${MOD2C_MECH_OUTPUTS})

endif()

set_source_files_properties(${MECH_FILES} PROPERTIES COMPILE_FLAGS "-Wno-error")

common_library(corebluron)

install(FILES ${CMAKE_BINARY_DIR}/corebluron/CMakeFiles/corebluron.dir/nrniv/corebluron_main.cpp.o DESTINATION lib)
