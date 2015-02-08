noinst_LTLIBRARIES = libcvodes.la

nsrc=$(top_srcdir)/src
includes = -I$(nsrc)/sundials/shared
AM_CPPFLAGS = $(includes)
AM_CFLAGS = @MINGW_CFLAG@

libcvodes_la_SOURCES = \
	cvband.c \
	cvbandpre.c \
	cvbbdpre.c \
	cvdense.c \
	cvdiag.c \
	cvodea.c \
	cvodes.c \
	cvodesio.c \
	cvspgmr.c

noinst_HEADERS = \
	cvband.h \
	cvband_impl.h \
	cvbandpre.h \
	cvbandpre_impl.h \
	cvbbdpre.h \
	cvbbdpre_impl.h \
	cvdense.h \
	cvdense_impl.h \
	cvdiag.h \
	cvdiag_impl.h \
	cvodea.h \
	cvodea_impl.h \
	cvodes.h \
	cvodes_impl.h \
	cvspgmr.h \
	cvspgmr_impl.h
