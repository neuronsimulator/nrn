# Copyright (c) 2013 Fabien Delalondre <fabien.delalondre@epfl.ch>
# Info: http://www.itk.org/Wiki/CMake:Component_Install_With_CPack

set(CPACK_PACKAGE_CONTACT "Aleksandr Ovcharenko <aleksandr.ovcharenko@epfl.ch>")
#set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.txt" )
set(CPACK_PACKAGE_LICENSE "Proprietary")

#set(CPACK_DEBIAN_PACKAGE_DEPENDS "libboost-tests-dev")

include(CommonCPack)
include(GNUModules)
