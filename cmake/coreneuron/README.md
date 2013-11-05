# CMake Modules

This repository contains common CMake modules for BBP projects. It is
the union of the open source CMake modules at
https://github.com/Eyescale/CMake and BBP-specific settings. Typically
you want to use in your top-level CMakeLists.txt:

    set(RELEASE_VERSION OFF) # OFF or ABI version
    set(LAST_RELEASE 1.4.1) # tarball, MacPorts, ...
    set(VERSION_MAJOR "1")
    set(VERSION_MINOR "5")
    set(VERSION_PATCH "0")

    list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake)
    include(Common)

## Howto use in other projects

First integration into another project:

    git remote add -f CMake ssh://bbpgit.epfl.ch/common/CMake.git
    git read-tree --prefix=CMake -u CMake/master
    git commit -am 'Merging CMake subtree'

Setup for new clone of a project:

    git remote add -f CMake ssh://bbpgit.epfl.ch/common/CMake.git

Update:

    git pull -s subtree CMake master
    git push

## Howto update files in the OSS subtree

Do all edits in a clone of https://github.com/Eyescale/CMake and push
them there, then update the oss subtree here:

    [first time only:]
    git remote add -f oss https://github.com/Eyescale/CMake.git

    git pull -s subtree oss master
    git push

