## This file makes one library out of all of the GNU libg++ pieces that we
## use.  It used to be split into a couple of different libraries but I
## combined it because I can't see any reason to multiply entities 
## unnecessarily.

## The library that we install.  I picked this name so it's not likely to
## conflict with anything anyone else installs.
lib_LTLIBRARIES = libneuron_gnu.la

##change from stream.h to ivstream.h means we have to specify the
##interviews install location
includes = @IV_INCLUDE@
AM_CPPFLAGS = $(includes)
AM_CXXFLAGS = @MINGW_CFLAG@

libneuron_gnu_la_SOURCES = ACG.cpp Binomial.cpp Complex.cpp DiscUnif.cpp \
	Erlang.cpp Geom.cpp HypGeom.cpp LogNorm.cpp MLCG.cpp NegExp.cpp \
	Normal.cpp Poisson.cpp RNG.cpp Random.cpp RndInt.cpp SmplHist.cpp \
	SmplStat.cpp Uniform.cpp Weibull.cpp builtin.cpp d_avec.cpp d_vec.cpp

noinst_HEADERS = ACG.h Binomial.h Complex.h DiscUnif.h Erlang.h \
	Geom.h HypGeom.h LogNorm.h MLCG.h NegExp.h Normal.h Poisson.h RNG.h \
	Random.h RndInt.h SmplHist.h SmplStat.h Uniform.h Weibull.h \
	neuron_gnu_builtin.h d_avec.h d_defs.h d_vec.h neuron_gnu_std.h
