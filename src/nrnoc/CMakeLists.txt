# Compile mod files
set(MODFILE_BASE_NAMES
    apcount
    exp2syn
    expsyn
    feature
    hh
    intfire1
    intfire2
    intfire4
    netstim
    oclmp
    passive
    pattern
    ppmark
    stim
    svclmp
    syn
    vclmp)
foreach(modfile ${MODFILE_BASE_NAMES})
  nocmodl_mod_to_cpp(${modfile})
  list(APPEND NRN_MODFILE_CPP ${CMAKE_CURRENT_BINARY_DIR}/${modfile}.cpp)
endforeach()
set_property(
  SOURCE ${NRN_MODFILE_CPP}
  APPEND
  PROPERTY COMPILE_OPTIONS ${NRN_EXTRA_MECH_CXX_FLAGS})
# we are basically emulating nrnivmodl with CMake here. nrnivmodl uses headers from the include
# directory in the build or install directory.
set_property(
  SOURCE ${NRN_MODFILE_CPP}
  APPEND
  PROPERTY INCLUDE_DIRECTORIES ${PROJECT_BINARY_DIR}/include)
set_source_files_properties(${NRN_MODFILE_CPP} PROPERTIES GENERATED TRUE)

# Create a static library from mod fiels to be included in nrniv
add_library(default_mods STATIC ${NRN_MODFILE_CPP})
set_property(TARGET default_mods PROPERTY POSITION_INDEPENDENT_CODE ON)
