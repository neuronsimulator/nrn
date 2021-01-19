/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#ifndef lpt_h
#define lpt_h

#include <vector>

std::vector<size_t>* lpt(size_t nbag, std::vector<size_t>& pieces, double* bal = nullptr);

double load_balance(std::vector<size_t>&);
#endif
