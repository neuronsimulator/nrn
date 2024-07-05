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

readarray -td '' mods < <(printf '%s\0' "${BASH_ARGV[@]%%.mod}" | env LC_ALL=C sort -z)

cat <<EOF
#include <cstdio>
namespace coreneuron {
extern int nrnmpi_myid;
extern int nrn_nobanner_;
$(
  for mod in "${mods[@]}"; do
    printf "extern int _%s_reg(void);\n" "$mod"
  done
)

void modl_reg() {
    if (!nrn_nobanner_ && nrnmpi_myid < 1) {
        fprintf(stderr, " Additional mechanisms from files\\n");
$(
  for mod in "${mods[@]}"; do
    printf "        fprintf(stderr, \" %s.mod\");\n" "$mod"
  done
)
        fprintf(stderr, "\\n\\n");
    }

$(
  for mod in "${mods[@]}"; do
    printf "    _%s_reg();\n" "$mod"
  done
)
}
} //namespace coreneuron
EOF
