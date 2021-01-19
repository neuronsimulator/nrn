# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

find_program(SPHINX_EXECUTABLE
        NAMES sphinx-build
        DOC "/path/to/sphinx-build")

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Sphinx
        "Failed to find sphinx-build executable"
        SPHINX_EXECUTABLE)