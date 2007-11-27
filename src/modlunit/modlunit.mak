cc = gcc
BIN = /cygdrive/c/nrn/bin
BNAME = $(BIN)/modlunit.exe

CFLAGS = -DNRNUNIT=1 -DWIN32=1 -I.

OBJ = \
 parse1.o \
 consist.o \
 declare.o \
 init.o \
 io.o \
 kinunit.o \
 lex.o \
 list.o \
 model.o \
 nrnunit.o \
 passn.o \
 symbol.o \
 units.o \
 units1.o \
 version.o

.c.o:
	$(cc) -c $(CFLAGS) $*.c

$(BNAME): $(OBJ)
	$(cc) -o $(BNAME) $(OBJ) -lm

units.o: units.c
	$(cc) -c $(CFLAGS) units.c

clean:
	rm *.o

