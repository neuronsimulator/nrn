#!/usr/bin/perl
#
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================

#Construct the modl_reg() function from a provided list
#of modules.

#Usage : mod_func.c.pl[MECH1.mod MECH2.mod...]

@mods = @ARGV;
s/\.mod$// foreach @mods;

@mods=sort @mods;

if(!@mods) {
    print STDERR "mod_func.c.pl: No mod files provided";
    print "// No mod files provided
namespace coreneuron {
  void modl_reg() {}
}
";
    exit 0;
}

print << "__eof";
#include <cstdio>
namespace coreneuron {
extern int nrnmpi_myid;
extern int nrn_nobanner_;
extern int @{[join ",\n  ", map{"_${_}_reg(void)"} @mods]};

void modl_reg() {
    if (!nrn_nobanner_ && nrnmpi_myid < 1) {
        fprintf(stderr, " Additional mechanisms from files\\n");
        @{[join "\n        ",
           map{"fprintf(stderr, \" $_.mod\");"} @mods] }
        fprintf(stderr, "\\n\\n");
    }

    @{[join "\n    ", map{"_${_}_reg();"} @mods] }
}
} //namespace coreneuron
__eof
