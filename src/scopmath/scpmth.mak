LIBR = /cygdrive/c/nrn/lib
LNAME = $(LIBR)/libscpmt.a
	
cc = gcc
CFLAGS = -mno-cygwin -DHOC=1 -I/cygdrive/c/nrn/lib -I../mswin -I/cygdrive/c/nrn/mingw

OBJ = \
 abort.o\
 advance.o\
 boundary.o\
 crank.o\
 crout.o\
 deflate.o\
 dimplic.o\
 euler.o\
 expfit.o\
 exprand.o\
 f2cmisc.o\
 factoria.o\
 force.o\
 gauss.o\
 gear.o\
 getmem.o\
 harmonic.o\
 hyperbol.o\
 invert.o\
 lag.o\
 legendre.o\
 newton.o\
 normrand.o\
 perpulse.o\
 perstep.o\
 poisrand.o\
 poisson.o\
 praxis.o\
 pulse.o\
 quad.o\
 ramp.o\
 revhyper.o\
 revsawto.o\
 revsigmo.o\
 romberg.o\
 runge.o\
 sawtooth.o\
 schedule.o\
 sigmoid.o\
 simeq.o\
 sparse.o\
 spline.o\
 squarewa.o\
 ssimplic.o\
 step.o\
 threshol.o\
 tridiag.o
 
.c.o:
	$(cc) -c $(CFLAGS) $*.c

$(LNAME): $(OBJ)
	ar cr $(LNAME) *.o
	ranlib $(LNAME)
