bin_PROGRAMS = modlunit

modlunit_SOURCES = parse1.y lex.l consist.c declare.c init.c io.c \
	kinunit.c list.c model.c nrnunit.c passn.c symbol.c units.c \
	units1.c version.c

## The list of .h files that are needed in the distribution.
noinst_HEADERS = extargs.h extdef.h model.h symbol.h units.h parse1.h \
 macmunit.h modlunit.h

## for building a dos version
EXTRA_DIST = modlunit.mak carbonmunit.h

AM_YFLAGS = -d

AM_CPPFLAGS = -DNRNUNIT=1

## files that depend on the existence of parse1.h
## this allows parallel make -j to work at the beginning when parse1.h
## does not exist and dependencies have not yet been determined
consist.o declare.o init.o io.o kinunit.o lex.o list.o model.o \
nrnunit.o passn.o symbol.o units1.o : parse1.h
