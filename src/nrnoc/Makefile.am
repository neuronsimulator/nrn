## The program that we make from this directory.
bin_PROGRAMS = nrnoc

nrnoc_SOURCES = ocmain.c nrnnoiv.c ocnoiv.c cprop.c

nrnoc_LDADD = ../oc/modlreg.o libnrnoc.la ../oc/liboc.la @MEMACSLIBLA@ \
	../scopmath/libscopmath.la ../sparse13/libsparse13.la \
	../nrnmpi/libnrnmpi.la \
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

HOC_E = ../e_editor/hoc_ed
HOC_E_DEP = ../e_editor/hoc_ed

PTHREAD_CC=@PTHREAD_CC@
PTHREAD_CFLAGS=@PTHREAD_CFLAGS@
PTHREAD_LIBS=@PTHREAD_LIBS@
AM_CFLAGS = @MINGW_CFLAG@ $(PTHREAD_CFLAGS)
AM_CPPFLAGS = -DCABLE=1 -DOOP=1
nsrc=$(top_srcdir)/src
INCLUDES = -I$(top_builddir)/src/oc -I$(nsrc)/oc -I$(nsrc)/scopmath -I$(nsrc)/sparse13
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
		export MODLUNIT ; $(NMODL) $(srcdir)/$(*F) )
	sed "s/_reg()/_reg_()/" $(srcdir)/$(*F).c > $*.tmp
	mv $*.tmp $(srcdir)/$(*F).c
feature.c: feature.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/$(*F) )
	sed "s/_reg()/_reg_()/" $(srcdir)/$(*F).c > $*.tmp
	mv $*.tmp $(srcdir)/$(*F).c
passive.c: passive.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/$(*F) )
	sed "s/_reg()/_reg_()/" $(srcdir)/$(*F).c > $*.tmp
	mv $*.tmp $(srcdir)/$(*F).c
stim.c: stim.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/$(*F) )
	sed "s/_reg()/_reg_()/" $(srcdir)/$(*F).c > $*.tmp
	mv $*.tmp $(srcdir)/$(*F).c
syn.c: syn.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/$(*F) )
	sed "s/_reg()/_reg_()/" $(srcdir)/$(*F).c > $*.tmp
	mv $*.tmp $(srcdir)/$(*F).c
expsyn.c: expsyn.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/$(*F) )
	sed "s/_reg()/_reg_()/" $(srcdir)/$(*F).c > $*.tmp
	mv $*.tmp $(srcdir)/$(*F).c
exp2syn.c: exp2syn.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/$(*F) )
	sed "s/_reg()/_reg_()/" $(srcdir)/$(*F).c > $*.tmp
	mv $*.tmp $(srcdir)/$(*F).c
vclmp.c: vclmp.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/$(*F) )
	sed "s/_reg()/_reg_()/" $(srcdir)/$(*F).c > $*.tmp
	mv $*.tmp $(srcdir)/$(*F).c
apcount.c: apcount.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/$(*F) )
	sed "s/_reg()/_reg_()/" $(srcdir)/$(*F).c > $*.tmp
	mv $*.tmp $(srcdir)/$(*F).c
svclmp.c: svclmp.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/$(*F) )
	sed "s/_reg()/_reg_()/" $(srcdir)/$(*F).c > $*.tmp
	mv $*.tmp $(srcdir)/$(*F).c
oclmp.c: oclmp.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/$(*F) )
	sed "s/_reg()/_reg_()/" $(srcdir)/$(*F).c > $*.tmp
	mv $*.tmp $(srcdir)/$(*F).c
xmech.c: xmech.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/$(*F) )
	sed "s/_reg()/_reg_()/" $(srcdir)/$(*F).c > $*.tmp
	mv $*.tmp $(srcdir)/$(*F).c
netstim.c: netstim.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/$(*F) )
	sed "s/_reg()/_reg_()/" $(srcdir)/$(*F).c > $*.tmp
	mv $*.tmp $(srcdir)/$(*F).c
intfire1.c: intfire1.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/$(*F) )
	sed "s/_reg()/_reg_()/" $(srcdir)/$(*F).c > $*.tmp
	mv $*.tmp $(srcdir)/$(*F).c
intfire2.c: intfire2.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/$(*F) )
	sed "s/_reg()/_reg_()/" $(srcdir)/$(*F).c > $*.tmp
	mv $*.tmp $(srcdir)/$(*F).c
intfire4.c: intfire4.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/$(*F) )
	sed "s/_reg()/_reg_()/" $(srcdir)/$(*F).c > $*.tmp
	mv $*.tmp $(srcdir)/$(*F).c
ppmark.c: ppmark.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/$(*F) )
	sed "s/_reg()/_reg_()/" $(srcdir)/$(*F).c > $*.tmp
	mv $*.tmp $(srcdir)/$(*F).c
pattern.c: pattern.mod $(NMODL)
	(MODLUNIT=$(top_srcdir)/share/lib/nrnunits.lib ; \
		export MODLUNIT ; $(NMODL) $(srcdir)/$(*F) )
	sed "s/_reg()/_reg_()/" $(srcdir)/$(*F).c > $*.tmp
	mv $*.tmp $(srcdir)/$(*F).c

## Force make to make hocusr.h even before it's done dependency scanning.
hocusr.lo: hocusr.h

## This uses a truly antique unix tool to convert a .h file into
## the static data necessary for hoc to link to all its functions.
## This wouldn't be hard to replace with a perl script....
hocusr.h: $(nsrc)/nrnoc/neuron.h $(nsrc)/oc/hocinc $(HOC_E_DEP)
	cat $(nsrc)/nrnoc/neuron.h > tmp.c
	@CPP@ -I$(nsrc)/nrnoc -I$(nsrc)/oc tmp.c | sed '/^#/d' > neuron.tmp
	$(HOC_E) - neuron.tmp < $(nsrc)/oc/hocinc
	cat neuron.tmp temp2hoc > hocusr.h
	rm -f temp1hoc temp2hoc neuron.tmp tmp.c

## Header files that need to be installed:
pkginclude_HEADERS = neuron.h md1redef.h md2redef.h section.h nrnoc_ml.h \
	cabvars.h membdef.h membfunc.h \
	options.h nrnredef.h multisplit.h multicore.h nmodlmutex.h

## Header files that need to be included in the distribution but not installed:
noinst_HEADERS = hocusr.h \
	osxdlfcn.h osxdlfcn.c multicore.c

## We have to play a trick on automake to get it to install the .o files in
## an architecture-dependent subdirectory.  (Apparently automake's authors
## never heard of installing .o files.)  We pretend to create a separate
## directory (which is really just $(pkglibdir)) and we install the .o files
## by calling them "DATA".
## Argh!!!!!
libobjdir = $(libdir)

## The following object files need to be installed:
libobj_DATA = ocmain.o nrnnoiv.o ocnoiv.o cprop.o

