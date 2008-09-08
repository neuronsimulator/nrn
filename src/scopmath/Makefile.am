## Libraries that we make from this directory:
lib_LTLIBRARIES = libscopmath.la

libscopmath_la_SOURCES = abort.c adams.c adeuler.c adrunge.c advance.c \
	boundary.c crank.c crout.c csoda.c csodabnc.c deflate.c \
	dimplic.c scoperf.c euler.c expfit.c exprand.c f2cmisc.c factoria.c \
	force.c gauss.c gear.c getmem.c harmonic.c heun.c hyperbol.c \
	invert.c lag.c legendre.c newton.c normrand.c perpulse.c \
	perstep.c poisrand.c poisson.c praxis.c pulse.c quad.c ramp.c \
	revhyper.c revsawto.c revsigmo.c romberg.c runge.c \
	sawtooth.c schedule.c seidel.c sigmoid.c simeq.c simplex.c \
	sparse.c vsparse.c spline.c squarewa.c ssimplic.c step.c \
	threshol.c tridiag.c sparse_thread.c newton_thread.c crout_thread.c \
	ssimplic_thread.c

## Header files that need to be included in the distribution:
noinst_HEADERS = errcodes.h f2c.h scopenv.h newton_struct.h

EXTRA_DIST = scpmth.mak

pkginclude_HEADERS = scoplib.h

AM_CPPFLAGS = -DHOC=1
AM_CFLAGS = @MINGW_CFLAG@ 
