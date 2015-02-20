##
base = nrn_ni6229
lib_LTLIBRARIES = libni6229.la

sources = nrnni.cpp nrn_aiex2.cpp nrn_aoex4.cpp

libni6229_la_SOURCES = $(sources)
libni6229_la_DEPENDENCIES = $(sources)


NIDDK = @NIDDK@
NIA = $(NIDDK)/100a2
NOD = $(NIA)/make/build

nsrc=$(top_srcdir)/src

IV_INCLUDES = @IV_INCLUDE@
includes = \
	-I$(nsrc)/nrniv -I$(nsrc)/ivoc -I$(nsrc)/nrnoc -I$(nsrc)/oc \
	-I/usr/local/vxipnp/linux/include \
	-I$(NIA)/osinterface -I$(NIA)/chipobjects -I$(NIA)/examples \
	$(IV_INCLUDES)

AM_CPPFLAGS = $(includes)

LO = \
$(NOD)/osiBus.o \
$(NOD)/osiUserCode.o \
$(NOD)/scale.o \
$(NOD)/tMSeries.o \
$(NOD)/tTIO.o \
$(NOD)/gpctex1.o \
$(NOD)/common.o \
$(NOD)/ai.o $(NOD)/ao.o

libni6229_la_LIBADD = $(LO)

noinst_HEADERS = nrndaq.h

EXTRA_DIST = notes_niddk run .runinfo test1.hoc test5.hoc
