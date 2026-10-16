#pragma once

// Some things in netpar.cpp that were static but needed by nrnmusic.cpp

#include "netcon.h"
#include <unordered_map>
struct Symbol;

using Gid2PreSyn = std::unordered_map<int, PreSyn*>;

double nrn_usable_mindelay();
Symbol* nrn_netcon_sym();
Gid2PreSyn& nrn_gid2out();
