# pyx->cpp
# the cpp files are consumed to make modules in src/nrnpython/setup.py

set(basenames ctng surfaces graphicsPrimitives)

if (NRN_ENABLE_RX3D AND NRN_ENABLE_MODULE_INSTALL AND NRN_ENABLE_PYTHON)
  foreach(basename ${basenames})
    set(bname ${CMAKE_CURRENT_BINARY_DIR}/${basename}.cpp)
    set(sname ${CMAKE_CURRENT_SOURCE_DIR}/${basename}.pyx)

     add_custom_command(OUTPUT ${bname}.1
      COMMAND ${CYTHON_EXECUTABLE} ${basename}.pyx -o ${bname}.1
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      DEPENDS ${sname} ${PROJECT_BINARY_DIR}/src/nrnpython/setup.py)

   add_custom_command(OUTPUT ${bname}
      # MINGW requres a fix
      COMMAND sh -c "'\
	if test \"${MINGW}\" = \"1\" ; then \
	  if ! grep -q \"_hypot\" ${bname}.1 ; then \
echo \"\#define _hypot hypot\" > ${bname}.2 ; \
cat ${bname}.1 >> ${bname}.2 ; \
sed \"/__pyx_check_sizeof_voidp/s/enum/\\/\\/enum/\" ${bname}.2 > ${bname} ; \
echo modified ${bname} ; \
	  else \
	    cp ${bname}.1 ${bname} ; \
	  fi \
	else \
	  cp ${bname}.1 ${bname} ; \
	fi \
	'"
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      DEPENDS ${bname}.1)

    add_custom_target(${basename}_module ALL
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      DEPENDS ${bname})

    message (STATUS "${CYTHON_EXECUTABLE} ${sname} -> ${bname}")
  endforeach(basename)
endif()
