/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

/**
 * @file string_utils.h
 * @brief Utility functions for strings
 *
 */

#pragma once

/** @brief Appends a copy of the source string to the destination string.
 *
 *  A null-character is included at the end of the new string formed by the concatenation of both in
 * destination. It has similar behavior to strcat but better performance in case that it is needed
 * to append a char array to another very large char array.
 *
 *  @param dest Destination string
 *  @param start_position Position of dest to start writing src
 *  @param src Source string
 *  @param src_length Length of src to append to dest
 *  @return Position of the final character of dest after appending src (including the null
 * terminating character)
 */
unsigned strcat_at_pos(char* dest, unsigned start_position, char* src, unsigned src_length);
