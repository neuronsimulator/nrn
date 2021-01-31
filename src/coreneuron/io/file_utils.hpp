/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

/**
 * @file file_utils.h
 * @brief Utility functions for file/directory management
 *
 */

#ifndef NRN_FILE_UTILS
#define NRN_FILE_UTILS

/** @brief Creates directory if doesn't exisit (similar to mkdir -p)
 *  @param Directory path
 *  @return Status
 */
int mkdir_p(const char* path);

#endif /* ifndef NRN_FILE_UTILS */
