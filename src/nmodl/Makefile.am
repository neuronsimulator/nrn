## Makefile for nocmodl.
## Note that hmodl can be built from this directory too, but it requires
## different flags so we don't bother.  Automake doesn't like to build things
## that way.

bin_PROGRAMS = nocmodl

nocmodl_SOURCES = parse1.ypp diffeq.ypp lex.lpp consist.cpp deriv.cpp discrete.cpp \
	init.cpp io.cpp kinetic.cpp list.cpp modl.cpp nocpout.cpp noccout.cpp parsact.cpp \
	netrec_discon.cpp \
	partial.cpp sens.cpp simultan.cpp solve.cpp symbol.cpp units.cpp version.cpp

nsrc=$(top_srcdir)/src
IV_INCLUDES = @IV_INCLUDE@ $(X_CFLAGS)
includes = -I$(nsrc)/src/modlunit $(IV_INCLUDES)
AM_CPPFLAGS = -DNRN_DYNAMIC_UNITS=1 -DNMODL=1 -DNOCMODL=1 -DCVODE=1 -DVECTORIZE=1 $(includes)

EXTRA_DIST = cout.cpp lex.cpp nmodl.mak

AM_YFLAGS = -d

## The list of .h files that are needed in the distribution.
noinst_HEADERS = extdef.h extdef2.h extdef5.h modl.h symbol.h parse1.hpp \
	difeqdef.h macnmodl.h carbonnmodl.h nmodlfunc.h


## files that depend on the existence of parse1.hpp or diffeq.hpp (none do)
## this allows parallel make -j to work at the beginning when those
## do not exist and dependencies have not yet been determined
consist.o cout.o deriv.o discrete.o init.o io.o kinetic.o lex.o \
list.o noccout.o nocpout.o parsact.o partial.o sens.o simultan.o \
solve.o symbol.o netrec_discon.o: parse1.hpp
