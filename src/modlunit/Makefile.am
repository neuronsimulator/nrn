bin_PROGRAMS = modlunit

modlunit_SOURCES = parse1.ypp lex.lpp consist.cpp declare.cpp init.cpp io.cpp \
	kinunit.cpp list.cpp model.cpp nrnunit.cpp passn.cpp symbol.cpp units.cpp \
	units1.cpp version.cpp

## The list of .h files that are needed in the distribution.
noinst_HEADERS = extargs.h extdef.h model.h symbol.h units.h parse1.hpp \
 macmunit.h modlunit.h

## for building a dos version
EXTRA_DIST = modlunit.mak carbonmunit.h

AM_YFLAGS = -d

IV_INCLUDES = @IV_INCLUDE@ $(X_CFLAGS)
AM_CPPFLAGS = -DNRNUNIT=1 $(IV_INCLUDES)

## files that depend on the existence of parse1.hpp
## this allows parallel make -j to work at the beginning when parse1.hpp
## does not exist and dependencies have not yet been determined
consist.o declare.o init.o io.o kinunit.o lex.o list.o model.o \
nrnunit.o passn.o symbol.o units1.o : parse1.hpp
