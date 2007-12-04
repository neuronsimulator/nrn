SUBDIRS = shared cvodes ida

# prefer the following but on max osx, libtoold links the libraries
# twice causing multiply defined names. So use the lo files instead.
#SUBLIBS = shared/libshared.la cvodes/libcvodes.la ida/libida.la
SUBLIBS = shared/*.lo cvodes/*.lo ida/*.lo

AM_CFLAGS = @MINGW_CFLAG@

lib_LTLIBRARIES = libsundials.la
libsundials_la_SOURCES =
libsundials_la_LIBADD = $(SUBLIBS)

EXTRA_DIST = README
