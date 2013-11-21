include_directories(${CMAKE_SOURCE_DIR}/src)
link_directories (${CMAKE_BINARY_DIR}/src/simcore ${CMAKE_BINARY_DIR}/src/mech)

set(LIBRARY corebluron coremech)

add_executable(coreron main.cpp)
target_link_libraries (coreron ${LIBRARY})
