## Makefile for nocmodl.
## Note that hmodl can be built from this directory too, but it requires
## different flags so we don't bother.  Automake doesn't like to build things
## that way.

bin_PROGRAMS = nocmodl

nocmodl_SOURCES = parse1.y diffeq.y lex.l consist.c deriv.c discrete.c \
	init.c io.c kinetic.c list.c modl.c nocpout.c noccout.c parsact.c \
	partial.c sens.c simultan.c solve.c symbol.c units.c version.c

nsrc=$(top_srcdir)/src
includes = -I$(nsrc)/src/modlunit
AM_CPPFLAGS = -DNMODL=1 -DNOCMODL=1 -DCVODE=1 -DVECTORIZE=1 $(includes)

EXTRA_DIST = cout.c lex.c nmodl.mak

AM_YFLAGS = -d

## The list of .h files that are needed in the distribution.
noinst_HEADERS = extdef.h extdef2.h extdef5.h modl.h symbol.h parse1.h \
	difeqdef.h macnmodl.h carbonnmodl.h nmodlfunc.h


## files that depend on the existence of parse1.h or diffeq.h (none do)
## this allows parallel make -j to work at the beginning when those
## do not exist and dependencies have not yet been determined
consist.o cout.o deriv.o discrete.o init.o io.o kinetic.o lex.o \
list.o noccout.o nocpout.o parsact.o partial.o sens.o simultan.o \
solve.o symbol.o: parse1.h
