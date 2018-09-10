## The program that we make from this directory.
bin_PROGRAMS = nrnoc

nrnoc_SOURCES = ocmain.c nrnnoiv.c ocnoiv.c cprop.c

if BUILD_NRNMPI_DYNAMIC
nrnmpila =
else
nrnmpila = ../nrnmpi/libnrnmpi.la
endif

nrnoc_LDADD = ../oc/modlreg.o libnrnoc.la ../oc/liboc.la @MEMACSLIBLA@ \
	../scopmath/libscopmath.la ../sparse13/libsparse13.la \
	$(nrnmpila) \
	@NRN_READLINE_LIBS@ @PTHREAD_LIBS@

## The shared library that we make from this directory.
lib_LTLIBRARIES = libnrnoc.la 

MODFILES = hh.mod feature.mod passive.mod stim.mod syn.mod \
    vclmp.mod apcount.mod svclmp.mod oclmp.mod xmech.mod   \
    netstim.mod intfire1.mod intfire2.mod intfire4.mod     \
    expsyn.mod exp2syn.mod ppmark.mod pattern.mod

#same as above, can't use $(patsubst %.mod,%.c,$(MODFILES)) due to automake
MODFILES_C = hh.c feature.c passive.c stim.c syn.c \
    vclmp.c apcount.c svclmp.c oclmp.c xmech.c     \
    netstim.c intfire1.c intfire2.c intfire4.c     \
    expsyn.c exp2syn.c ppmark.c pattern.c

libnrnoc_la_SOURCES = $(MODFILES_C) cabcode.c capac.c clamp.c eion.c      \
    extcelln.c fadvance.c fstim.c init.c nrnnemo.c point.c		\
    psection.c solve.c synapse.c treeset.c hoc_init.c code.c hoc_oop.c    \
    seclist.c method3.c hocprax.c secref.c ldifus.c hocusr.c nrnversion.c \
    nrnversion.h nrntimeout.c passive0.c

##
## The list of .c files which are actually built during the build procedure.
## These files should not be packaged in a distribution.
## Some of these are made from .mod files, and the others are soft linked from
## other directories.
##

BUILT_SOURCES = $(MODFILES_C) $(srcdir)/nrnversion.h

EXTRA_DIST = $(MODFILES)

PTHREAD_CC=@PTHREAD_CC@
PTHREAD_CFLAGS=@PTHREAD_CFLAGS@
PTHREAD_LIBS=@PTHREAD_LIBS@
AM_CFLAGS = @MINGW_CFLAG@ $(PTHREAD_CFLAGS)
nsrc=$(top_srcdir)/src
includes = -I$(top_builddir)/src/oc -I$(nsrc)/oc -I$(nsrc)/scopmath -I$(nsrc)/sparse13 -I$(nsrc)/nrnmpi
AM_CPPFLAGS = -DCABLE=1 -DOOP=1 $(includes)
NMODL = ../nmodl/nocmodl
NRNUNITSLIB = $(top_builddir)/share/lib/nrnunits.lib

FORCE:

$(srcdir)/nrnversion.h: FORCE
	if test -d $(top_srcdir)/.git ; then \
		sh $(top_srcdir)/git2nrnversion_h.sh $(top_srcdir) > $(srcdir)/nrnversion.h.tmp ; \
		if cmp $(srcdir)/nrnversion.h.tmp $(srcdir)/nrnversion.h ; then \
			echo "nrnversion.h not changed" ; \
			rm $(srcdir)/nrnversion.h.tmp ; \
		else \
			mv $(srcdir)/nrnversion.h.tmp $(srcdir)/nrnversion.h ; \
			echo "updated nrnversion.h" ; \
		fi \
	else \
		if test ! -f $(srcdir)/nrnversion.h ; then \
			echo '#define GIT_DATE "unknown"' > $(srcdir)/nrnversion.h ; \
			echo '#define GIT_BRANCH "unknown"' >> $(srcdir)/nrnversion.h ; \
			echo '#define GIT_CHANGESET "?"' >> $(srcdir)/nrnversion.h ; \
			echo '#define GIT_LOCAL "?"' >> $(srcdir)/nrnversion.h ; \
			echo "nrnversion.h created" ; \
		else \
			echo "nrnversion.h not changed" ; \
		fi \
	fi

nrnversion.o nrnversion.lo: nrnversion.h

$(MODFILES_C): $(NMODL) $(NRNUNITSLIB)

%.c: %.mod
	(export MODLUNIT=$(NRNUNITSLIB); $(NMODL) $<)
	sed "s/_reg()/_reg_()/" $(srcdir)/$(notdir $@) > $(srcdir)/$(notdir $@).tmp
	mv $(srcdir)/$(notdir $@).tmp $(srcdir)/$(notdir $@)

## Force make to make hocusr.h even before it's done dependency scanning.
hocusr.lo: hocusr.h

hocusr.h: $(nsrc)/nrnoc/neuron.h $(nsrc)/oc/mk_hocusr_h.py $(nsrc)/nrnoc/options.h
	cat $(nsrc)/nrnoc/neuron.h > tmp.c
	@CPP@ -E -I$(nsrc)/nrnoc -I$(nsrc)/oc tmp.c | sed '/^#/d' > neuron.tmp
	$(PYTHON_BLD) $(nsrc)/oc/mk_hocusr_h.py < neuron.tmp > temp2hoc
	cat temp2hoc |sed 's/"nrnhoc_topology"/"topology"/' > hocusr.h
	rm -f temp1hoc temp2hoc neuron.tmp tmp.c

## Header files that need to be installed:
pkginclude_HEADERS = neuron.h md1redef.h md2redef.h section.h nrnoc_ml.h \
	cabvars.h membdef.h membfunc.h nrniv_mf.h nrn_ansi.h nrncvode.h \
	options.h nrnredef.h multisplit.h multicore.h nmodlmutex.h

## Header files that need to be included in the distribution but not installed:
noinst_HEADERS = hocusr.h \
	osxdlfcn.h osxdlfcn.c multicore.c \
	nonvintblock.h nrndae_c.h

## We have to play a trick on automake to get it to install the .o files in
## an architecture-dependent subdirectory.  (Apparently automake's authors
## never heard of installing .o files.)  We pretend to create a separate
## directory (which is really just $(pkglibdir)) and we install the .o files
## by calling them "DATA".
## Argh!!!!!
libobjdir = $(libdir)

## The following object files need to be installed:
libobj_DATA = ocmain.o nrnnoiv.o ocnoiv.o cprop.o

