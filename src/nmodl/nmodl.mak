# This is set up to make nocmodl
# to make hmodl do a 
#	del *.o
# and change the BNAME, CFLAGS, and OBJ def

cc = gcc
BIN = /cygdrive/c/nrn/bin
BNAME = $(BIN)/nocmodl.exe
#BNAME = $(BIN)/hmodl.exe

NFLAGS = -DNMODL=1 -DNOCMODL=1 -DVECTORIZE=1 -DCVODE=1 -I.
HFLAGS = -DHMODL=1

CFLAGS = -DWIN32=1 $(NFLAGS)
#CFLAGS = $(HFLAGS)

HOBJ = \
 cout.o \
 hparout.o

NOBJ = \
 noccout.o \
 nocpout.o \
 diffeq.o

OBJ = \
 $(NOBJ) \
 parse1.o \
 consist.o \
 deriv.o \
 discrete.o \
 init.o \
 io.o \
 kinetic.o \
 lex.o \
 list.o \
 modl.o \
 parsact.o \
 partial.o \
 sens.o \
 simultan.o \
 solve.o \
 symbol.o \
 units.o \
 version.o

.c.o:
	$(cc) -c $(CFLAGS) $*.c

$(BNAME): $(OBJ)
	$(cc) -o $(BNAME) $(OBJ) -lm

#$(cc) -c $(CFLAGS) -I../modlunit -DNHOME="$(NEURONHOME)" ../modlunit/units.c

units.o: ../modlunit/units.c ../modlunit/units.h
	$(cc) -c $(CFLAGS) -I../modlunit ../modlunit/units.c

clean:
	rm *.o
