## This Makefile.am installs stuff in $NEURONHOME/lib.  This is now a
## subdirectory of $(prefix)/share, but it used to all be in $(prefix)/lib.
## Now $(prefix)/lib is reserved for architecture-dependent files.

## Files that need to be installed from this directory.
neuronhomedir = $(prefix)/share/@PACKAGE@/lib
data_and_dist = helpdict nrnunits.lib \
	prologue.id shape.cm1 shape.cm2
neuronhome_DATA = $(data_and_dist) nrn.defaults
neuronhome_SCRIPTS = cleanup hocload.sh

EXTRA_DIST = $(data_and_dist) $(neuronhome_SCRIPTS)

DIST_SUBDIRS = auditscripts hoc python

if NRN_PYTHON_ON
SUBDIRS = auditscripts hoc python
else
SUBDIRS = auditscripts hoc
endif

