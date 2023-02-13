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

#pragma once

/** @brief Creates directory if doesn't exisit (similar to mkdir -p)
 *  @param Directory path
 *  @return Status
 */
int mkdir_p(const char* path);
