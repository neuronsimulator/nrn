# pyx->cpp
# the cpp files are consumed to make modules in src/nrnpython/setup.py

set(basenames ctng surfaces graphicsPrimitives)

if (NRN_ENABLE_RX3D)
  foreach(basename ${basenames})
    set(bname ${CMAKE_CURRENT_BINARY_DIR}/${basename}.cpp)
    set(sname ${CMAKE_CURRENT_SOURCE_DIR}/${basename}.pyx)

    add_custom_command(OUTPUT ${bname}
      COMMAND ${CYTHON_EXECUTABLE} ${basename}.pyx -o ${bname}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      DEPENDS ${sname} ${PROJECT_BINARY_DIR}/src/nrnpython/setup.py)

    add_custom_target(${basename}_module ALL
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      DEPENDS ${bname})

    message (STATUS "${CYTHON_EXECUTABLE} ${sname} -> ${bname}")
  endforeach(basename)
endif()
