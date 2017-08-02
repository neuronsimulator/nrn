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
#include "coreneuron/nrniv/nrn_datareader.h"

extern "C" int check_bbcore_write_version(const char*);

data_reader::data_reader(const char* filename, bool reorder) {
    this->open(filename, reorder);
    checkpoint(0);
}

void data_reader::open(const char* filename, bool reorder) {
    reorder_on_read = reorder;

    close();
    F.open(filename);

    char version[256];
    F.getline(version, sizeof(version));
    nrn_assert(!F.fail());
    check_bbcore_write_version(version);
}

int data_reader::read_int() {
    char line_buf[max_line_length];

    F.getline(line_buf, sizeof(line_buf));
    nrn_assert(!F.fail());

    int i;
    int n_scan = sscanf(line_buf, "%d", &i);
    nrn_assert(n_scan == 1);

    return i;
}

void data_reader::read_mapping_count(int* gid, int* nsec, int* nseg, int* nseclist) {
    char line_buf[max_line_length];

    F.getline(line_buf, sizeof(line_buf));
    nrn_assert(!F.fail());

    /** mapping file has extra strings, ignore those */
    int n_scan = sscanf(line_buf, "%d %d %d %d", gid, nsec, nseg, nseclist);
    nrn_assert(n_scan == 4);
}

void data_reader::read_mapping_cell_count(int* count) {
    *count = read_int();
}

void data_reader::read_checkpoint_assert() {
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

void data_reader::close() {
    F.close();
}
