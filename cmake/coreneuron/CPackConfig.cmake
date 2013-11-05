# Copyright (c) 2013 Fabien Delalondre <fabien.delalondre@epfl.ch>
# Info: http://www.itk.org/Wiki/CMake:Component_Install_With_CPack

set(CPACK_PACKAGE_CONTACT "Aleksandr Ovcharenko <aleksandr.ovcharenko@epfl.ch>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "BBP core simulator")
#set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.txt" )
set(CPACK_PACKAGE_LICENSE "Proprietary")
set(CPACK_PROJECT_NAME "cbluron")
#set(CPACK_PACKAGING_INSTALL_PREFIX "/home/delalond/Dev/bluron-doc/installPackage")

#set(CPACK_DEBIAN_PACKAGE_DEPENDS "${BBPSDK_DEB_DEPENDENCIES},
#  libboost-filesystem-dev, libboost-program-options-dev, libboost-system-dev,
#  libboost-test-dev, libcairo2, curl, libxml2")

include(CommonCPack)

include(oss/GNUModules)
