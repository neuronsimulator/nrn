/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include <cstring>

unsigned strcat_at_pos(char* dest, unsigned start_position, char* src, unsigned src_length) {
    memcpy(dest + start_position, src, src_length);
    dest[start_position + src_length] = '\0';
    return start_position + src_length;
}
