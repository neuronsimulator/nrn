## This is the top level Makefile.am.
#AUTOMAKE_OPTIONS = foreign dist-zip dist-tarZ
AUTOMAKE_OPTIONS = foreign dist-zip
ACLOCAL_AMFLAGS = -I m4

host_cpu = @host_cpu@

NVER = $(PACKAGE_VERSION)

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
	nrnversion.sh hg2nrnversion_h.sh extend_depcomp.sh Copyright

if BUILD_PARANEURON
	PARANEURON='yes'
else
	PARANEURON='no'
endif

if BUILD_CYGWIN
#install from the build directories to the mswin destination
#uses the classical positions of files
mswin:
	top_srcdir=$(top_srcdir) marshal_dir=$(marshal_dir) \
	top_builddir=$(top_builddir) \
	ivbindir=$(IV_LIBDIR)/../bin \
	CC=$(CC) \
	host_cpu=$(host_cpu) \
	$(top_srcdir)/src/mswin/batch/instal.sh

endif

if BUILD_MINGW
#install from the build directories to the mswin destination
#uses the classical positions of files
mswin:
	top_srcdir=$(top_srcdir) marshal_dir=$(marshal_dir) \
	top_builddir=$(top_builddir) \
	ivbindir=$(IV_LIBDIR)/../bin \
	CC=$(CC) \
	PARANEURON=$(PARANEURON) \
	host_cpu=$(host_cpu) \
	$(top_srcdir)/src/mswin/batch/instalmingw.sh

endif

if BUILD_NRNPYTHON
install-exec-hook:
	echo "enter install-exec-hook in nrn/Makefile.am"
	if test "@ac_pysetup@" != "no" ; then \
		cd src/nrnpython ; \
		$(PYTHON) setup.py install @ac_pysetup@ ;\
	fi
	echo "leave install-exec-hook in nrn/Makefile.am"
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
	scp $(RPM_TARGET) $(ALPHADIR)/nrn-"`(export NSRC='$(srcdir)' ; sh $(srcdir)/nrnversion.sh 2)`".$(host_cpu).rpm
	scp `dirname $(RPM_TARGET)`/nrn*.deb $(ALPHADIR)/nrn-"`(export NSRC='$(srcdir)' ; sh $(srcdir)/nrnversion.sh 2)`".$(host_cpu).deb

else
endif

if MAC_DARWIN
ivlibdir = @IV_LIBDIR@
after_install:
	carbon="@enable_carbon@" sh $(srcdir)/src/mac/after-install.sh "$(host_cpu)" "$(prefix)" "$(top_srcdir)" "$(ivlibdir)"

dmg:
	carbon="@enable_carbon@" sh $(srcdir)/src/mac/mkdmg.sh NEURON-"`(export NSRC='$(srcdir)' ; sh $(srcdir)/nrnversion.sh)`"

pkg:
	/usr/local/bin/packagesbuild $(top_builddir)/src/mac/macdist.pkgproj

alphadist:
if UniversalMacBinary
	a=$(ALPHADIR)/nrn-"``(export NSRC='$(srcdir)' ; sh $(srcdir)/nrnversion.sh 2)`"-MacOSX"`sw_vers -productVersion`".dmg && \
	echo $$a && \
	scp $(HOME)/NEURON-"`(export NSRC='$(srcdir)' ; sh $(srcdir)/nrnversion.sh)`".dmg "$$a"
else
	a=$(ALPHADIR)/nrn-$(NVER)."$(host_cpu)"-apple-osx"`sw_vers -productVersion`".pkg && \
	echo "scp $(top_builddir)/src/mac/build/NEURON-$(NVER).pkg $$a" && \
	scp $(top_builddir)/src/mac/build/NEURON-$(NVER).pkg $$a
endif

else
if BUILD_CYGWIN
alphadist:
	a=$(ALPHADIR)/nrn-"`(export NSRC='$(srcdir)' ; sh $(srcdir)/nrnversion.sh 2)`".$(host)-setup.exe &&\
	echo $$a && \
	scp src/mswin/nrn"`(export NSRC='$(srcdir)' ; sh $(srcdir)/nrnversion.sh 3)`"setup.exe $$a
else
if BUILD_MINGW
alphadist:
	a=$(ALPHADIR)/nrn-"`(export NSRC='$(srcdir)' ; sh $(srcdir)/nrnversion.sh 2)`".$(host)-setup.exe && \
	echo $$a && \
	scp src/mswin/nrn"`(export NSRC='$(srcdir)' ; sh $(srcdir)/nrnversion.sh 3)`"setup-$(host_cpu).exe $$a
else
alphadist:
	a=$(ALPHADIR)/nrn-"`(export NSRC='$(srcdir)' ; sh $(srcdir)/nrnversion.sh 2)`".tar.gz && \
	echo $$a && \
	scp nrn-"`(export NSRC='$(srcdir)' ; sh $(srcdir)/nrnversion.sh)`".tar.gz $$a
endif
endif
endif
