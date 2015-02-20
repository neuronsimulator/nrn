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

libnrnoc_la_SOURCES = cabcode.c capac.c clamp.c eion.c extcelln.c \
	fadvance.c fstim.c hh.c  feature.c init.c nrnnemo.c passive.c \
	passive0.c point.c psection.c solve.c synapse.c treeset.c hoc_init.c \
	code.c hoc_oop.c stim.c syn.c vclmp.c seclist.c method3.c \
	apcount.c hocprax.c svclmp.c oclmp.c xmech.c secref.c \
	ldifus.c hocusr.c nrnversion.c nrnversion.h \
	netstim.c intfire1.c intfire2.c intfire4.c expsyn.c exp2syn.c \
	ppmark.c pattern.c nrntimeout.c

## 
## The list of .c files which are actually built during the build procedure.
## These files should not be packaged in a distribution.
## Some of these are made from .mod files, and the others are soft linked from
## other directories.
##
BUILT_SOURCES = hh.c feature.c passive.c stim.c syn.c vclmp.c \
	apcount.c svclmp.c oclmp.c xmech.c \
	netstim.c intfire1.c intfire2.c intfire4.c expsyn.c exp2syn.c \
	ppmark.c pattern.c $(srcdir)/nrnversion.h

MODFILES = hh.mod feature.mod passive.mod stim.mod syn.mod \
	vclmp.mod apcount.mod svclmp.mod oclmp.mod xmech.mod \
	netstim.mod intfire1.mod intfire2.mod intfire4.mod \
	expsyn.mod exp2syn.mod ppmark.mod pattern.mod

EXTRA_DIST = $(MODFILES)

PTHREAD_CC=@PTHREAD_CC@
PTHREAD_CFLAGS=@PTHREAD_CFLAGS@
PTHREAD_LIBS=@PTHREAD_LIBS@
AM_CFLAGS = @MINGW_CFLAG@ $(PTHREAD_CFLAGS)
nsrc=$(top_srcdir)/src
includes = -I$(top_builddir)/src/oc -I$(nsrc)/oc -I$(nsrc)/scopmath -I$(nsrc)/sparse13 -I$(nsrc)/nrnmpi
AM_CPPFLAGS = -DCABLE=1 -DOOP=1 $(includes)
NMODL = ../nmodl/nocmodl

FORCE:

$(srcdir)/nrnversion.h: FORCE
	if test -d $(top_srcdir)/.hg ; then \
		sh $(top_srcdir)/hg2nrnversion_h.sh $(top_srcdir) > $(srcdir)/nrnversion.h.tmp ; \
		if cmp $(srcdir)/nrnversion.h.tmp $(srcdir)/nrnversion.h ; then \
			echo "nrnversion.h not changed" ; \
			rm $(srcdir)/nrnversion.h.tmp ; \
		else \
			mv $(srcdir)/nrnversion.h.tmp $(srcdir)/nrnversion.h ; \
			echo "updated nrnversion.h" ; \
		fi \
	else \
	if test -d $(top_srcdir)/.svn && svnversion $(top_srcdir) ; then \
	if test "`svnversion $(top_srcdir)`" != "`sed -n 's/.*SVN_CHANGESET.*(\([^)]*\).*/\1/p' $(srcdir)/nrnversion.h`" ; then \
	    sh $(top_srcdir)/svn2nrn_h.sh $(top_srcdir) > $(srcdir)/nrnversion.h.tmp; \
	    if test -s $(srcdir)/nrnversion.h.tmp ; then \
		mv $(srcdir)/nrnversion.h.tmp $(srcdir)/nrnversion.h ; \
	    fi ; \
	    echo "updated nrnversion.h" ; \
	else \
		echo "nrnversion.h not changed" ; \
	fi \
	fi \
	fi

nrnversion.o nrnversion.lo: nrnversion.h

hh.c: hh.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/hh )
	sed "s/_reg()/_reg_()/" $(srcdir)/hh.c > hh.tmp
	mv hh.tmp $(srcdir)/hh.c
feature.c: feature.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/feature )
	sed "s/_reg()/_reg_()/" $(srcdir)/feature.c > feature.tmp
	mv feature.tmp $(srcdir)/feature.c
passive.c: passive.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/passive )
	sed "s/_reg()/_reg_()/" $(srcdir)/passive.c > passive.tmp
	mv passive.tmp $(srcdir)/passive.c
stim.c: stim.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/stim )
	sed "s/_reg()/_reg_()/" $(srcdir)/stim.c > stim.tmp
	mv stim.tmp $(srcdir)/stim.c
syn.c: syn.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/syn )
	sed "s/_reg()/_reg_()/" $(srcdir)/syn.c > syn.tmp
	mv syn.tmp $(srcdir)/syn.c
expsyn.c: expsyn.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/expsyn )
	sed "s/_reg()/_reg_()/" $(srcdir)/expsyn.c > expsyn.tmp
	mv expsyn.tmp $(srcdir)/expsyn.c
exp2syn.c: exp2syn.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/exp2syn )
	sed "s/_reg()/_reg_()/" $(srcdir)/exp2syn.c > exp2syn.tmp
	mv exp2syn.tmp $(srcdir)/exp2syn.c
vclmp.c: vclmp.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/vclmp )
	sed "s/_reg()/_reg_()/" $(srcdir)/vclmp.c > $*.tmp
	mv $*.tmp $(srcdir)/vclmp.c
apcount.c: apcount.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/apcount )
	sed "s/_reg()/_reg_()/" $(srcdir)/apcount.c > $*.tmp
	mv $*.tmp $(srcdir)/apcount.c
svclmp.c: svclmp.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/svclmp )
	sed "s/_reg()/_reg_()/" $(srcdir)/svclmp.c > $*.tmp
	mv $*.tmp $(srcdir)/svclmp.c
oclmp.c: oclmp.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/oclmp )
	sed "s/_reg()/_reg_()/" $(srcdir)/oclmp.c > $*.tmp
	mv $*.tmp $(srcdir)/oclmp.c
xmech.c: xmech.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/xmech )
	sed "s/_reg()/_reg_()/" $(srcdir)/xmech.c > $*.tmp
	mv $*.tmp $(srcdir)/xmech.c
netstim.c: netstim.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/netstim )
	sed "s/_reg()/_reg_()/" $(srcdir)/netstim.c > $*.tmp
	mv $*.tmp $(srcdir)/netstim.c
intfire1.c: intfire1.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/intfire1 )
	sed "s/_reg()/_reg_()/" $(srcdir)/intfire1.c > $*.tmp
	mv $*.tmp $(srcdir)/intfire1.c
intfire2.c: intfire2.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/intfire2 )
	sed "s/_reg()/_reg_()/" $(srcdir)/intfire2.c > $*.tmp
	mv $*.tmp $(srcdir)/intfire2.c
intfire4.c: intfire4.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/intfire4 )
	sed "s/_reg()/_reg_()/" $(srcdir)/intfire4.c > $*.tmp
	mv $*.tmp $(srcdir)/intfire4.c
ppmark.c: ppmark.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/ppmark )
	sed "s/_reg()/_reg_()/" $(srcdir)/ppmark.c > $*.tmp
	mv $*.tmp $(srcdir)/ppmark.c
pattern.c: pattern.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/pattern )
	sed "s/_reg()/_reg_()/" $(srcdir)/pattern.c > $*.tmp
	mv $*.tmp $(srcdir)/pattern.c

## Force make to make hocusr.h even before it's done dependency scanning.
hocusr.lo: hocusr.h


## This uses a truly antique unix tool to convert a .h file into
## the static data necessary for hoc to link to all its functions.
## This wouldn't be hard to replace with a perl script....
if BUILD_MINGW
else
hocusr.h: $(nsrc)/nrnoc/neuron.h $(nsrc)/oc/mk_hocusr_h.py $(nsrc)/nrnoc/options.h
	cat $(nsrc)/nrnoc/neuron.h > tmp.c
	@CPP@ -I$(nsrc)/nrnoc -I$(nsrc)/oc tmp.c | sed '/^#/d' > neuron.tmp
	$(PYTHON_BLD) $(nsrc)/oc/mk_hocusr_h.py < neuron.tmp > temp2hoc
	cat neuron.tmp temp2hoc |sed 's/"nrnhoc_topology"/"topology"/' > hocusr.h
	rm -f temp1hoc temp2hoc neuron.tmp tmp.c
endif
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

