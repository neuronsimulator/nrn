#pragma once

#include "nrncore_write/data/cell_group.h"

int nrncore_run(const char* arg);
int nrncore_is_enabled();
int nrncore_psolve(double tstop, int file_mode);
