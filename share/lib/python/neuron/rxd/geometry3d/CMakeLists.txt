# =============================================================================
# Cython (pyx) to CPP conversion
# =============================================================================
# ~~~
# The cpp files are consumed to make modules in src/nrnpython/setup.py
#
# Because of problems building modules with MINGW containing Cython
# generated files, the cpp files are consumed to make modules from here,
# so that these modules can be built with the msvc toolchain.
# Maybe someday the src/nrnpython/setup.py
# can build all the extension modules for windows.
#
# Following set in src/nrnpython/CMakeLists.txt should be made global.
# These are used in setup.py.
# ~~~
set(NRN_SRCDIR ${PROJECT_SOURCE_DIR})
set(CC ${CMAKE_C_COMPILER})
set(CXX ${CMAKE_CXX_COMPILER})

if(NRN_WINDOWS_BUILD)
  set(BUILD_MINGW_TRUE "")
  set(BUILD_MINGW_FALSE "#")
else()
  set(BUILD_MINGW_TRUE "#")
  set(BUILD_MINGW_FALSE "")
endif()

# generate setup.py from setup.py.in
nrn_configure_file(setup.py share/lib/python/neuron/rxd/geometry3d)

set(basenames ctng surfaces graphicsPrimitives)

if(NRN_ENABLE_RX3D
   AND NRN_ENABLE_MODULE_INSTALL
   AND NRN_ENABLE_PYTHON)
  # ~~~
  # For MINGW, python3 uses msvc
  # The following shell script carries out the craziness when
  # ${pyexe} setup.py build_ext --build-lib=${NRN_PYTHON_BUILD_LIB}
  # may not be enough
  # ~~~

  # Prepare a shell script to transform cython generated cpp files for mingw
  file(
    WRITE ${CMAKE_CURRENT_BINARY_DIR}/cy_cpp_filt.sh
    "\
#!bash\n\
set -ex\n\
file=$1\n\
mingw=${MINGW}\n\
if test x$mingw = x1 ; then #only MINGW, linux/mac does nothing \n\
  if ! grep -q '_hypot' $file ; then #only transform once\n\
    echo '#define _hypot hypot' > $file.tmp\n\
    cat $file >> $file.tmp\n\
    sed 's/EXTERN_C DL_IMPORT(\\([^)]*\\))/EXTERN_C \\1/' $file.tmp > $file\n\
    rm $file.tmp
  fi\n\
fi\n\
")

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
if test x$mingw = x1 ; then\n\
  pyver=`$pyexe -c 'import sys; print (sys.version_info[0]); quit()'`\n\
  echo pyver=$pyver\n\
  if test x$pyver = x3 ; then # python3.x builds with msvc\n\
    . ${PROJECT_SOURCE_DIR}/mingw_files/vcenv.sh\n\
    $pyexe setup.py build_ext --build-lib=${NRN_PYTHON_BUILD_LIB}\n\
  fi\n\
else #mac/linux does not need anything special\n\
  $pyexe setup.py build_ext --build-lib=${NRN_PYTHON_BUILD_LIB}\n\
fi\n\
")

  # ~~~
  # Cython generates *.cpp from *.pyx. Make a list of the cpp files
  # so the rxd_extensions targets can depend on them.
  # These cpp files are given a false dependency on setup.py so that a change
  # to setup.py.in in the end causes setup.py to execute and which apparently
  # rebuilds only if a cpp file has changed.
  # ~~~
  foreach(basename ${basenames})
    set(bname ${CMAKE_CURRENT_BINARY_DIR}/${basename}.cpp)
    set(sname ${CMAKE_CURRENT_SOURCE_DIR}/${basename}.pyx)

    add_custom_command(
      OUTPUT ${bname}
      COMMAND ${CYTHON_EXECUTABLE} -2 ${basename}.pyx -o ${bname}
      COMMAND bash ${CMAKE_CURRENT_BINARY_DIR}/cy_cpp_filt.sh "${bname}"
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      DEPENDS ${sname} ${CMAKE_CURRENT_BINARY_DIR}/setup.py)

    list(APPEND rxd_cython_cpp_files ${bname})

  endforeach(basename)

  # Want the cython custom command to generate only once no matter how many pythons
  add_custom_target(rxd_cython_generated DEPENDS ${rxd_cython_cpp_files})

  # ~~~
  # For each python detected / provided by user, build the extensions
  # we do not care about the target names but they must be unique, hence the
  # index at the end of each name. Notice that the unique target runs
  # its COMMAND only if a DEPENDS is out of date (which is the case if setup.py.in)
  # is out of date (see the CYTHON executable custom_command)
  # ~~~
  list(LENGTH NRN_PYTHON_EXE_LIST _num_pythons)
  math(EXPR num_pythons "${_num_pythons} - 1")
  foreach(val RANGE ${num_pythons})
    list(GET NRN_PYTHON_EXE_LIST ${val} pyexe)
    add_custom_target(
      rx3dextensions_${val}
      COMMAND bash ${CMAKE_CURRENT_BINARY_DIR}/runpy.sh "${pyexe}"
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      DEPENDS rxd_cython_generated)
    list(APPEND rx3dextensions rx3dextensions_${val})
  endforeach(val)

  # ~~~
  # Always out of date but that does not imply any of the rx3dextension targets
  # are out of date
  # ~~~
  add_custom_target(rx3d ALL DEPENDS ${rx3dextensions})

endif()
