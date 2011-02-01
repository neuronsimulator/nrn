## The hoc_e editor is an ancient line editor used to do some
## filtering that is somewhat beyond sed. It used to be in the oc directory
## but was moved here to make it easier to do cross compilation since this
## as well as modlunit and nocmodl must be compiled for execution on the
## build machine.

## Because of the possibility of copyright issues, the hoc_e editor 
## (an extended version of the Unix version 6 ed has been replaced by
## the Gnu version of ed. This is standard with linux and cygwin but to
## get it on the mac we copied the gnu source of ed-1.5 here, which was
## obtained from
## http://mirrors.kernel.org/gnu/ed/ed-1.5.tar.gz

bin_PROGRAMS = hoc_ed

hoc_ed_SOURCES = buffer.c carg_parser.c global.c io.c \
	main.c main_loop.c regex.c signal.c

AM_CPPFLAGS = -DPROGVERSION=\"ed-1.5\"
noinst_HEADERS = ed.h carg_parser.h

INCLUDES = -I$(top_srcdir)/src/oc
