noinst_LTLIBRARIES = libshared.la

PARSRC = nvector_parallel.c
PARINC = nvector_parallel.h

if BUILD_PARANEURON
PARSRC1 = $(PARSRC)
PARINC1 = $(PARINC)
else
PARSRC1 =
PARINC1 =

EXTRA_DIST = $(PARSRC) $(PARINC)
endif

AM_CFLAGS = @MINGW_CFLAG@

libshared_la_SOURCES = band.c dense.c iterative.c \
	nvector.c nvector_serial.c smalldense.c spgmr.c sundialsmath.c \
	$(PARSRC1)

noinst_HEADERS = band.h dense.h iterative.h \
	nvector.h nvector_serial.h smalldense.h spgmr.h \
	sundialsmath.h sundialstypes.h \
	$(PARINC1)

