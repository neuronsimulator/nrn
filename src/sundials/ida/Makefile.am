noinst_LTLIBRARIES = libida.la

nsrc=$(top_srcdir)/src
includes = -I$(nsrc)/sundials/shared
AM_CPPFLAGS = $(includes)
AM_CFLAGS = @MINGW_CFLAG@

libida_la_SOURCES = \
	idaband.c \
	idabbdpre.c \
	ida.c \
	idadense.c \
	idaic.c \
	idaio.c \
	idaspgmr.c

noinst_HEADERS = \
	idaband.h \
	idaband_impl.h \
	idabbdpre.h \
	idabbdpre_impl.h \
	idadense.h \
	idadense_impl.h \
	ida.h \
	ida_impl.h \
	idaspgmr.h \
	idaspgmr_impl.h
