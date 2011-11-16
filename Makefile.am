## This is the top level Makefile.am.
#AUTOMAKE_OPTIONS = foreign dist-zip dist-tarZ
AUTOMAKE_OPTIONS = foreign dist-zip
ACLOCAL_AMFLAGS = -I m4

host_cpu = @host_cpu@

if NMODL_ONLY
SUBDIRS = src
else
SUBDIRS = src bin share
endif
#
# Install libtool; it's needed to compile the "special" program.
#
pkgdata_SCRIPTS = libtool

## Unfortunately nrnconf.h is architecture dependent so the standard
## installation of it does not allow multiarchitectures,
## i.e the following is wrong.
## pkginclude_HEADERS = nrnconf.h
## So we do it as data
libobjdir = $(libdir)
libobj_DATA = nrnconf.h

CONFIGURE_DEPENDENCIES = m4/acx_mpi.m4 m4/ivcheck.m4 m4/java_support.m4 \
	m4/nrnjava.m4 m4/nrnpython.m4 m4/nrnrt.m4 \
	m4/oddities.m4 m4/parcon.m4 m4/readline.m4 \
	m4/withmpi.m4 m4/nrnbg.m4

EXTRA_DIST = $(CONFIGURE_DEPENDENCIES) installunix.html build.sh \
	nrnversion.sh hg2nrnversion_h.sh extend_depcomp.sh

if BUILD_CYGWIN
#install from the build directories to the mswin destination
#uses the classical positions of files
mswin:
	top_srcdir=$(top_srcdir) marshall_dir=$(marshall_dir) \
	top_builddir=$(top_builddir) \
	ivbindir=$(IV_LIBDIR)/../bin \
	CC=$(CC) \
	$(top_srcdir)/src/mswin/batch/instal.sh

endif

ALPHADIR = hines@www.neuron.yale.edu:/home/htdocs/ftp/neuron/versions/alpha

if MAKE_RPMS
rpm: @RPM_TARGET@

.PHONY: rpm

$(RPM_TARGET): $(DISTFILES)
	(cd $(libdir) ; strip -d *.so*)
	(cd @IV_LIBDIR@ ; strip -d *.so*)
	-rm $(pkgdatadir)/demo/neuron
	(cd $(pkgdatadir)/demo/release ; $(bindir)/nrnivmodl ; cd $(host_cpu) ; rm -f *.c *.o *.lo *.mod *.la ; cd ../.. ; ln -s release/$(host_cpu)/special neuron )
	-mkdir -p `dirname $(RPM_TARGET)`
	(cd `dirname $(RPM_TARGET)` && rm -f nrn*.deb nrn*.rpm)
	ln -s / /tmp/nrnrpmbld
	$(RPM_PROG) -bb --buildroot /tmp/nrnrpmbld nrnrpm.spec
	@echo Congratulations, $(RPM_TARGET) "(and friends) should now exist."
	(cd `dirname $(RPM_TARGET)` &&  alien --to-deb $(RPM_TARGET))

rpmdist:
	scp $(RPM_TARGET) $(ALPHADIR)/nrn-"`sh $(srcdir)/nrnversion.sh 2`".$(host_cpu).rpm
	scp `dirname $(RPM_TARGET)`/nrn*.deb $(ALPHADIR)/nrn-"`sh $(srcdir)/nrnversion.sh 2`".$(host_cpu).deb

else
endif

if MAC_DARWIN
ivlibdir = @IV_LIBDIR@
after_install:
	carbon="@enable_carbon@" sh $(srcdir)/src/mac/after-install.sh "$(host_cpu)" "$(prefix)" "$(top_srcdir)" "$(ivlibdir)"

dmg:
	carbon="@enable_carbon@" sh $(srcdir)/src/mac/mkdmg.sh NEURON-"`sh $(srcdir)/nrnversion.sh`"

alphadist:
if UniversalMacBinary
	scp $(HOME)/NEURON-"`sh $(srcdir)/nrnversion.sh`".dmg $(ALPHADIR)/nrn-"`sh $(srcdir)/nrnversion.sh 2`"-MacOSX"`sw_vers -productVersion`".dmg
else
	scp $(HOME)/NEURON-"`sh $(srcdir)/nrnversion.sh`".dmg $(ALPHADIR)/nrn-"`sh $(srcdir)/nrnversion.sh 2`"-$(host_triplet).dmg
endif

else
if BUILD_CYGWIN
alphadist:
	scp src/mswin/nrn"`sh $(srcdir)/nrnversion.sh 3`"setup.exe $(ALPHADIR)/nrn-"`sh $(srcdir)/nrnversion.sh 2`"-setup.exe
else
alphadist:
	scp nrn-"`sh $(srcdir)/nrnversion.sh`".tar.gz $(ALPHADIR)/nrn-"`sh $(srcdir)/nrnversion.sh 2`".tar.gz
endif
endif

