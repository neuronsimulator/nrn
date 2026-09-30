# ~~~
# Build share/nrn/demo/release/nrnmech.dll for an MSVC wheel.
# Same CMake nrnivmodl path as binwrapper._nrnivmodl_cmake.
# Wheel-build so neurondemo does not need cl.exe; setup.exe stays MinGW.
#
# Required -D:
#   NRN_DEMO_RELEASE   build-tree share/nrn/demo/release (*.mod copied)
#   NRN_PREFIX         NEURON build prefix (CMAKE_PREFIX_PATH / neuron_DIR)
#   NRN_NRNIVMODL_SRC  .../lib/cmake/neuron/nrnivmodl
#   NRN_DEMO_DLL       destination nrnmech.dll
#   NRN_DEMO_BUILDDIR  nested cmake -B
# Optional: NRN_GENERATOR, NRN_GENERATOR_PLATFORM, NRN_CONFIG
# ~~~

if(NOT NRN_DEMO_RELEASE
   OR NOT NRN_PREFIX
   OR NOT NRN_NRNIVMODL_SRC
   OR NOT NRN_DEMO_DLL
   OR NOT NRN_DEMO_BUILDDIR)
  message(FATAL_ERROR "MsvcWheelDemoNrnmech: missing required -D arguments")
endif()

file(GLOB _nrn_demo_mods "${NRN_DEMO_RELEASE}/*.mod")
if(NOT _nrn_demo_mods)
  message(FATAL_ERROR "MsvcWheelDemoNrnmech: no MOD files in ${NRN_DEMO_RELEASE}")
endif()
list(SORT _nrn_demo_mods)

set(_nrn_demo_cfg)
if(NRN_GENERATOR)
  list(APPEND _nrn_demo_cfg -G "${NRN_GENERATOR}")
endif()
if(NRN_GENERATOR_PLATFORM)
  list(APPEND _nrn_demo_cfg -A "${NRN_GENERATOR_PLATFORM}")
endif()
if(NRN_CONFIG AND NOT NRN_GENERATOR MATCHES "Visual Studio")
  list(APPEND _nrn_demo_cfg "-DCMAKE_BUILD_TYPE=${NRN_CONFIG}")
endif()

message(STATUS "Building demo nrnmech.dll from ${NRN_DEMO_RELEASE}")
execute_process(
  COMMAND
    ${CMAKE_COMMAND} -S "${NRN_NRNIVMODL_SRC}" -B "${NRN_DEMO_BUILDDIR}" -DNRNIVMODL_NEURON=ON
    -DNRNIVMODL_CORENEURON=OFF -DNRNIVMODL_SPECIAL=OFF "-DNRNIVMODL_MOD_FILES=${_nrn_demo_mods}"
    "-DCMAKE_PREFIX_PATH=${NRN_PREFIX}" "-Dneuron_DIR=${NRN_PREFIX}/lib/cmake/neuron"
    ${_nrn_demo_cfg}
  RESULT_VARIABLE _nrn_demo_rc)
if(_nrn_demo_rc)
  message(FATAL_ERROR "MsvcWheelDemoNrnmech: cmake configure failed (${_nrn_demo_rc})")
endif()

set(_nrn_demo_build ${CMAKE_COMMAND} --build "${NRN_DEMO_BUILDDIR}" --parallel)
if(NRN_GENERATOR MATCHES "Visual Studio" AND NRN_CONFIG)
  list(APPEND _nrn_demo_build --config "${NRN_CONFIG}")
endif()
execute_process(COMMAND ${_nrn_demo_build} RESULT_VARIABLE _nrn_demo_rc)
if(_nrn_demo_rc)
  message(FATAL_ERROR "MsvcWheelDemoNrnmech: cmake --build failed (${_nrn_demo_rc})")
endif()

set(_nrn_demo_src)
foreach(
  _nrn_demo_cand IN
  ITEMS "${NRN_DEMO_BUILDDIR}/nrnmech.dll" "${NRN_DEMO_BUILDDIR}/${NRN_CONFIG}/nrnmech.dll"
        "${NRN_DEMO_BUILDDIR}/Release/nrnmech.dll"
        "${NRN_DEMO_BUILDDIR}/RelWithDebInfo/nrnmech.dll")
  if(EXISTS "${_nrn_demo_cand}")
    set(_nrn_demo_src "${_nrn_demo_cand}")
    break()
  endif()
endforeach()
if(NOT _nrn_demo_src)
  message(FATAL_ERROR "MsvcWheelDemoNrnmech: nrnmech.dll was not produced in ${NRN_DEMO_BUILDDIR}")
endif()

get_filename_component(_nrn_demo_dll_dir "${NRN_DEMO_DLL}" DIRECTORY)
file(MAKE_DIRECTORY "${_nrn_demo_dll_dir}")
execute_process(COMMAND ${CMAKE_COMMAND} -E copy "${_nrn_demo_src}" "${NRN_DEMO_DLL}"
                RESULT_VARIABLE _nrn_demo_rc)
if(_nrn_demo_rc)
  message(FATAL_ERROR "MsvcWheelDemoNrnmech: copy to ${NRN_DEMO_DLL} failed (${_nrn_demo_rc})")
endif()
message(STATUS "Installed demo nrnmech.dll -> ${NRN_DEMO_DLL}")
