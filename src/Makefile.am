## Subdirectories must be specified in the order they have to be compiled.
## A library is produced from _each_ subdirectory.  This is because automake
## (version 1.4) doesn't yet work well with source files in multiple 
## directories.
##
## The order of compilation is important in some cases.  Directories are
## searched in the order they are specified.

READLINE_SUBDIRS = readline
MEMACS_SUBDIRS = memacs
if BUILD_MINGW
NMODL_SUBDIRS = modlunit nmodl
else
NMODL_SUBDIRS = modlunit nmodl
endif
NRNOC_SUBDIRS = sparse sparse13 scopmath Random123 nrnmpi oc nrnoc
NRNIV_SUBDIRS = sundials mesch gnu ivoc nrncvode parallel nrniv
IVOS_SUBDIRS = ivos
NRNJAVA_SUBDIRS = nrnjava
NRNPYTHON_SUBDIRS = nrnpython

if NMODL_BUILD
nmodlbuild = $(NMODL_SUBDIRS)
else
nmodlbuild =
endif

if BUILD_MEMACS
memacsbuild = $(MEMACS_SUBDIRS)
else
memacsbuild = 
endif

if BUILD_READLINE
readlinebuild = $(READLINE_SUBDIRS)
else
readlinebuild =
endif

if BUILD_NRNIV
nrnivbuild = $(NRNIV_SUBDIRS)
else
nrnivbuild =
endif

if BUILD_IVOS
ivosbuild = $(IVOS_SUBDIRS)
else
ivosbuild =
endif

if BUILD_NRNJAVA
nrnjavabuild = $(NRNJAVA_SUBDIRS)
else
nrnjavabuild =
endif

if BUILD_NRNPYTHON
nrnpythonbuild = $(NRNPYTHON_SUBDIRS)
endif


if BUILD_NEURONMUSIC
neuronmusicbuild = neuronmusic
else
neuronmusicbuild =
endif

if BUILD_CYGWIN
mos2nrnbuild =
else
if BUILD_MINGW
mos2nrnbuild =
else
mos2nrnbuild = uxnrnbbs
endif
endif

if BUILD_6229
build_6229 = ni_pci_6229
else
build_6229 =
endif

if NMODL_ONLY
SUBDIRS = $(nmodlbuild)
else
SUBDIRS = $(readlinebuild) $(memacsbuild) $(nmodlbuild) $(NRNOC_SUBDIRS) \
	$(nrnjavabuild) $(ivosbuild) $(nrnpythonbuild) \
	$(build_6229) $(nrnivbuild) $(mos2nrnbuild) $(neuronmusicbuild) \
	mac mswin
endif

DIST_SUBDIRS = $(MEMACS_SUBDIRS) $(READLINE_SUBDIRS) $(NMODL_SUBDIRS) $(NRNOC_SUBDIRS) \
	$(NRNJAVA_SUBDIRS) $(NRNIV_SUBDIRS) $(IVOS_SUBDIRS) \
	$(NRNPYTHON_SUBDIRS) mswin uxnrnbbs mac ni_pci_6229 neuronmusic

dist-hook: $(npydir)
	for subdir in $(npydir) ; do \
		test -d "$(distdir)/$$subdir" \
		|| $(mkdir_p) "$(distdir)/$$subdir" \
		|| exit 1; \
	done

install-exec-hook:
	echo "enter install-exec-hook in nrn/src/Makefile.am"
if BUILD_NRNMPI_DYNAMIC
	cd nrnmpi ; \
	NEED_RTLD_GLOBAL='-L$(libdir) -loc -lnrniv' ; \
	export NEED_RTLD_GLOBAL ; \
	$(RM) *.la ; \
	$(MAKE) -e install
endif
	echo "leave install-exec-hook in nrn/src/Makefile.am"

