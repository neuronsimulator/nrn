/*
Copyright (c) 2016, Blue Brain Project
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
 * @file nrnoptarg.h
 * @date 26 Oct 2014
 *
 * @brief structure storing all command line arguments for coreneuron
 *
 * Abandon getopt and wrap ezOptionParser to allow options from a config file
 * as well on command line (command line takes precedence)
 */

#ifndef nrnoptarg_h
#define nrnoptarg_h

#include <string>

// before nrnopt_parse()
void nrnopt_add_flag(const char* names, const char* usage);
void nrnopt_add_int(const char* names, const char* usage, int dflt, int low, int high);
void nrnopt_add_dbl(const char* names, const char* usage, double dflt, double low, double high);
void nrnopt_add_str(const char* names, const char* usage, const char* dflt);

int nrnopt_parse(int argc, const char* argv[]);
void nrnopt_delete();  // when finished getting options, free memory
void nrnopt_show();

// after nrnopt_parse()
bool nrnopt_get_flag(const char* name);
int nrnopt_get_int(const char* name);
double nrnopt_get_dbl(const char* name);
std::string nrnopt_get_str(const char* name);
void nrnopt_modify_dbl(const char* name, double value);

#endif
