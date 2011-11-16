## Subdirectories must be specified in the order they have to be compiled.
## A library is produced from _each_ subdirectory.  This is because automake
## (version 1.4) doesn't yet work well with source files in multiple 
## directories.
##
## The order of compilation is important in some cases.  Directories are
## searched in the order they are specified.

READLINE_SUBDIRS = readline
MEMACS_SUBDIRS = memacs
NMODL_SUBDIRS = e_editor modlunit nmodl
NRNOC_SUBDIRS = sparse sparse13 scopmath nrnmpi oc nrnoc
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
else
nrnpythonbuild =
endif

if BUILD_NEURONMUSIC
neuronmusicbuild = neuronmusic
else
neuronmusicbuild =
endif

npydir = npy24 npy25 npy26 npy27

if BUILD_NRNPYTHON_DYNAMIC
#does not work with parallel make (-j)

nrnpydynam = $(npydir)

all-recursive: npy27/Makefile

npy27/Makefile: nrnpython/Makefile $(srcdir)/nrnpython/nrnpydynam.sh
	bash $(srcdir)/nrnpython/nrnpydynam.sh $(srcdir) $(DEPDIR) $(host)

else
nrnpydynam =
endif

if BUILD_CYGWIN
mos2nrnbuild =
else
mos2nrnbuild = uxnrnbbs
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
	$(nrnjavabuild) $(ivosbuild) $(nrnpythonbuild) $(nrnpydynam) \
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
