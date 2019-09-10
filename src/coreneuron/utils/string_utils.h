/*
Copyright (c) 2019, Blue Brain Project
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

/**
 * @file string_utils.h
 * @brief Utility functions for strings
 *
 */

#ifndef CORENEURON_STRING_UTILS_HPP
#define CORENEURON_STRING_UTILS_HPP

/** @brief Appends a copy of the source string to the destination string.
 *
 *  A null-character is included at the end of the new string formed by the concatenation of both in destination.
 *  It has similar behavior to strcat but better performance in case that it is needed to append a char array to
 *  another very large char array.
 *
 *  @param dest Destination string
 *  @param start_position Position of dest to start writing src
 *  @param src Source string
 *  @param src_length Length of src to append to dest
 *  @return Position of the final character of dest after appending src (including the null terminating character)
 */
unsigned strcat_at_pos(char* dest, unsigned start_position, char* src, unsigned src_length);

#endif /* ifndef CORENEURON_STRING_UTILS_HPP */
