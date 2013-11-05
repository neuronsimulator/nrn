
# Copyright (c) 2012-2013, EPFL/Blue Brain Project
#                          Daniel Nachbaur <daniel.nachbaur@epfl.ch>

# HOW-TO:
# This file describes the steps towards a release of any BBP software.
# It contains several make targets which implement one step of the prodecure
# (doxygit, branch, tag) + provides a complete release target for
# executing all the necessary steps in correct order (bbp-release).
#
# Note that the packaging (package, bbp-package) is handled separately
# because this must be called from each of the respective machines to
# build packages. The file containing the bbp-package rule is
# BBPPackages.cmake, included by CommonCPack

if(NOT RELEASE_VERSION)
  return()
endif()

set(GITTARGETS_RELEASE_BRANCH "minor")
include(GitTargets)

# procedure for a release:
# 0. create branch if needed
# 1. create tag on the branch
# 2. create documentation (commit & push to BBPDocumentation)
# 3. create & upload package (must be done seperately on the respective machines)
add_custom_target(bbp-release
  COMMENT "Finished release (branch, tag, documentation) for version ${VERSION}.
       Next steps:
       1. Review, commit & push documentation to BBPDocumentation
       2. Execute 'make bbp-package' on the release machines; push packages to BBPPackages"
  VERBATIM)
add_dependencies(bbp-release tag doxygit bbp-package)
