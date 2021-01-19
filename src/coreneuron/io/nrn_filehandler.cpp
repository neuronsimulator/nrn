/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================.
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
