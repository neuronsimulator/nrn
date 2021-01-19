/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#pragma once

#include <cstddef>
#include "coreneuron/mechanism/mechanism.hpp"

namespace coreneuron {

extern int v_structure_change;
extern int diam_changed;
extern int structure_change_cnt;

extern char* pnt_name(Point_process* pnt);

extern void nrn_exit(int);

extern void* emalloc(size_t size);
extern void* ecalloc(size_t n, size_t size);
extern void* erealloc(void* ptr, size_t size);

extern double* makevector(size_t size); /* size in bytes */
extern double** makematrix(size_t nrow, size_t ncol);
void freevector(double*);
void freematrix(double**);

extern void hoc_execerror(const char*, const char*); /* print and abort */
extern void hoc_warning(const char*, const char*);

extern double hoc_Exp(double x);

// defined in eion.cpp and this file included in translated mod files.
extern double nrn_nernst(double ci, double co, double z, double celsius);
extern double nrn_ghk(double v, double ci, double co, double z);

}  // namespace coreneuron
