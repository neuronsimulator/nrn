#!/bin/bash
#
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================

#Construct the modl_reg() function from a provided list
#of modules.

#Usage: mod_func.c.sh [MECH1.mod MECH2.mod...]

if [[ "$#" -eq 0 ]]; then
    >&2 echo "mod_func.c.sh: No mod files provided";
    cat <<EOF
// No mod files provided
namespace coreneuron {
  void modl_reg() {}
}
EOF
    exit 0
fi

# Assume that none of the arguments will contain spaces, as we inject them into C++ code
# later, anyways.
mods=$(for m in "$@"; do echo "${m%%.mod}"; done | env LC_ALL=C sort)

decls=""
prints=""
regs=""
for mod in $mods; do
    decls="${decls}${decls:+,$'\n'  }_${mod}_reg(void)"
    prints="${prints}${prints:+$'\n'        }fprintf(stderr, \" ${mod}.mod\");"
    regs="${regs}${regs:+$'\n'    }_${mod}_reg();"
done

cat <<EOF
#include <cstdio>
namespace coreneuron {
extern int nrnmpi_myid;
extern int nrn_nobanner_;
extern int
  ${decls};

void modl_reg() {
    if (!nrn_nobanner_ && nrnmpi_myid < 1) {
        fprintf(stderr, " Additional mechanisms from files\\n");
        ${prints}
        fprintf(stderr, "\\n\\n");
    }
    ${regs}
}
} //namespace coreneuron
EOF
