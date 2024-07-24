Cython_transpile(${CMAKE_CURRENT_SOURCE_DIR}/graphicsPrimitives.pyx
    OUTPUT_VARIABLE _graphics_primitives_sources)

Cython_transpile(${CMAKE_CURRENT_SOURCE_DIR}/ctng.pyx
    OUTPUT_VARIABLE _ctng_sources)

Cython_transpile(${CMAKE_CURRENT_SOURCE_DIR}/surfaces.pyx
    OUTPUT_VARIABLE _surfaces_sources)

include_directories(${Python_NumPy_INCLUDE_DIRS})

Python_add_library(
  graphicsPrimitives
  MODULE ${_graphics_primitives_sources}
  WITH_SOABI
)

Python_add_library(
  ctng
  MODULE ${_ctng_sources}
  WITH_SOABI
)

Python_add_library(
  surfaces
  MODULE ${_surfaces_sources}
  WITH_SOABI
)
