# a lot of this copied from nrn/share/lib/python/neuron/rxd/geometry3d/CMakeLists.txt

# ~~~
# Following set in src/nrnpython/CMakeLists.txt should be made global.
# These are used in setup.py.
# ~~~
set(NRN_SRCDIR ${PROJECT_SOURCE_DIR})
set(CC ${CMAKE_C_COMPILER})
set(CXX ${CMAKE_CXX_COMPILER})

nrn_configure_file(setup.py src/neuronmusic)

if(NRN_ENABLE_MUSIC
   AND NRN_ENABLE_MODULE_INSTALL
   AND NRN_ENABLE_PYTHON)

  # Prepare a shell script to run python setup.py
  file(
    WRITE ${CMAKE_CURRENT_BINARY_DIR}/runpy.sh
    "\
#!bash\n\
set -ex\n\
echo runpy $*\n\
pyexe=$1\n\
mingw=${MINGW}\n\
shift\n\
export LDCSHARED=\"${CMAKE_C_COMPILER} ${CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS}\"\n\
export LDCXXSHARED=\"${CMAKE_CXX_COMPILER} ${CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS}\"\n\
export SETUPTOOLS_USE_DISTUTILS=stdlib
if test x$mingw = x1 ; then\n\
  pyver=`$pyexe -c 'import sys; print (sys.version_info[0]); quit()'`\n\
  echo pyver=$pyver\n\
  if test x$pyver = x3 ; then # python3.x builds with msvc\n\
    . ${PROJECT_SOURCE_DIR}/mingw_files/vcenv.sh\n\
    $pyexe setup.py build_ext --build-lib=${NRN_PYTHON_BUILD_LIB}\n\
  fi\n\
else #mac/linux does not need anything special\n\
  $pyexe setup.py build --build-lib=${NRN_PYTHON_BUILD_LIB}\n\
fi\n\
")

  # ~~~
  # Cython generates *.cpp from *.pyx. Make a list of the cpp files
  # so the rxd_extensions targets can depend on them.
  # These cpp files are given a false dependency on setup.py so that a change
  # to setup.py.in in the end causes setup.py to execute and which apparently
  # rebuilds only if a cpp file has changed.
  # ~~~

  set(name neuronmusic)
  set(cppname ${CMAKE_CURRENT_BINARY_DIR}/neuronmusic.cpp)
  set(pyxname ${CMAKE_CURRENT_SOURCE_DIR}/neuronmusic.pyx)

  add_custom_command(
    OUTPUT ${cppname}
    COMMAND ${CYTHON_EXECUTABLE} -2 ${name}.pyx -o ${cppname}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${pyxname} ${CMAKE_CURRENT_BINARY_DIR}/setup.py)

  # Want the cython custom command to generate only once no matter how many pythons
  add_custom_target(neuronmusic_cython_generated DEPENDS ${cppname})

  add_custom_target(
    neuronmusicextension ALL
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${cppname})

  # ~~~
  # For each python detected / provided by user, build the extensions
  # we do not care about the target names but they must be unique, hence the
  # index at the end of each name. Notice that the unique target runs
  # its COMMAND only if a DEPENDS is out of date (which is the case if setup.py.in)
  # is out of date (see the CYTHON executable custom_command)
  # ~~~
  foreach(pyexe ${NRN_PYTHON_EXE_LIST})
    add_custom_command(
      TARGET neuronmusicextension
      POST_BUILD
      COMMAND bash ${CMAKE_CURRENT_BINARY_DIR}/runpy.sh "${pyexe}"
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Building neuronmusic module with: ${pyexe}")
  endforeach(pyexe)

  add_dependencies(neuronmusicextension nrniv_lib neuronmusic_cython_generated)

  # ~~~
  # (Copied from src/nrnpython/CMakeLists.txt)
  # neuron module (possibly with multiple extension versions) was built
  # in NRN_PYTHON_BUILD_LIB. Not a problem if install overwrites multiple
  # times to same install folder or if each install ends up in different
  # place.
  # ~~~
  file(
    WRITE ${CMAKE_CURRENT_BINARY_DIR}/neuronmusic_module_install.sh
    "\
#!bash\n\
echo 'Installing python module using:'\n\
set -ex\n\
cd ${CMAKE_CURRENT_BINARY_DIR}\n\
export SETUPTOOLS_USE_DISTUTILS=stdlib
$1 setup.py --quiet build --build-lib=${NRN_PYTHON_BUILD_LIB} install ${NRN_MODULE_INSTALL_OPTIONS}\n\
")
  foreach(pyexe ${NRN_PYTHON_EXE_LIST})
    # install(CODE ...) only takes a single CMake code expression, so we can't easily roll our own
    # check on the return code. Modern CMake versions support the COMMAND_ERROR_IS_FATAL option,
    # which will cause the installation to abort if the shell script returns an error code.
    if(${CMAKE_VERSION} VERSION_LESS "3.19")
      install(
        CODE "execute_process(COMMAND bash ${CMAKE_CURRENT_BINARY_DIR}/neuronmusic_module_install.sh ${pyexe})"
      )
    else()
      install(
        CODE "execute_process(COMMAND bash ${CMAKE_CURRENT_BINARY_DIR}/neuronmusic_module_install.sh ${pyexe} COMMAND_ERROR_IS_FATAL LAST)"
      )
    endif()
  endforeach(pyexe)

endif()
