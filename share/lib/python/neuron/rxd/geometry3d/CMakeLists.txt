# pyx->cpp
# the cpp files are consumed to make modules in src/nrnpython/setup.py

set(basenames ctng surfaces graphicsPrimitives)

if (NRN_ENABLE_RX3D)
  foreach(basename ${basenames})
    set(bname ${CMAKE_CURRENT_BINARY_DIR}/${basename}.cpp)
    add_custom_command(OUTPUT ${bname}
      COMMAND ${CYTHON_EXECUTABLE} ${basename}.pyx -o ${bname}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      MAIN_DEPENDENCY ${basename.pyx})

    add_custom_target(${basename}_module ALL
      DEPENDS ${bname})

    message (STATUS "${CYTHON_EXECUTABLE} ${basename}.pyx -> ${basename}.cpp")
  endforeach(basename)
endif()
