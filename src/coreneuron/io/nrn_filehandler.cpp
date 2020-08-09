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

#include <iostream>
#include "coreneuron/io/nrn_filehandler.hpp"
#include "coreneuron/nrnconf.h"

namespace coreneuron {
FileHandler::FileHandler(const std::string& filename)
    : chkpnt(0), stored_chkpnt(0) {
    this->open(filename);
}

bool FileHandler::file_exist(const std::string& filename) const {
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
}

void FileHandler::open(const std::string& filename, std::ios::openmode mode) {
    nrn_assert((mode & (std::ios::in | std::ios::out)));
    close();
    F.open(filename, mode | std::ios::binary);
    if (!F.is_open()) {
        std::cerr << "cannot open file '" << filename << "'" << std::endl;
    }
    nrn_assert(F.is_open());
    current_mode = mode;
    char version[256];
    if (current_mode & std::ios::in) {
        F.getline(version, sizeof(version));
        nrn_assert(!F.fail());
        check_bbcore_write_version(version);
    }
    if (current_mode & std::ios::out) {
        F << bbcore_write_version << "\n";
    }
}

bool FileHandler::eof() {
    if (F.eof()) {
        return true;
    }
    int a = F.get();
    if (F.eof()) {
        return true;
    }
    F.putback(a);
    return false;
}

int FileHandler::read_int() {
    char line_buf[max_line_length];

    F.getline(line_buf, sizeof(line_buf));
    nrn_assert(!F.fail());

    int i;
    int n_scan = sscanf(line_buf, "%d", &i);
    nrn_assert(n_scan == 1);

    return i;
}

void FileHandler::read_mapping_count(int* gid, int* nsec, int* nseg, int* nseclist) {
    char line_buf[max_line_length];

    F.getline(line_buf, sizeof(line_buf));
    nrn_assert(!F.fail());

    /** mapping file has extra strings, ignore those */
    int n_scan = sscanf(line_buf, "%d %d %d %d", gid, nsec, nseg, nseclist);
    nrn_assert(n_scan == 4);
}

void FileHandler::read_mapping_cell_count(int* count) {
    *count = read_int();
}

void FileHandler::read_checkpoint_assert() {
    char line_buf[max_line_length];

    F.getline(line_buf, sizeof(line_buf));
    nrn_assert(!F.fail());

    int i;
    int n_scan = sscanf(line_buf, "chkpnt %d\n", &i);
    if (n_scan != 1) {
        fprintf(stderr, "no chkpnt line for %d\n", chkpnt);
    }
    nrn_assert(n_scan == 1);
    if (i != chkpnt) {
        fprintf(stderr, "file chkpnt %d != expected %d\n", i, chkpnt);
    }
    nrn_assert(i == chkpnt);
    ++chkpnt;
}

void FileHandler::close() {
    F.close();
}
}  // namespace coreneuron
