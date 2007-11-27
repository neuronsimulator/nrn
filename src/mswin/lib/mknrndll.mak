
#CFLAGS = -mno-cygwin
CFLAGS = \
-DDLL_EXPORT -DPIC \
-I$N/src/scopmath -I$N/src/nrnoc -I$N/src/oc \
-I$N/lib -I$N/gccinc -I$N/gcc3inc

#CFLAGS = -I$N/../program\ files/metrowerks/codewarrior/msl/msl_c/msl_common -I$N/src/scopmath -I$N/src/nrnoc -I$N/src/oc

# to handle variations of filename extensions
.SUFFIXES: .o .mod .moD .mOd .mOD .Mod .MoD .MOd .MOD

.mod.o:
	nocmodl $*
	gcc $(CFLAGS) -c $*.c
	rm $*.c

# additional rules to handle variations of filename extensions
.moD.o:
	nocmodl $*
	gcc $(CFLAGS) -c $*.c
	rm $*.c

.mOd.o:
	nocmodl $*
	gcc $(CFLAGS) -c $*.c
	rm $*.c

.mOD.o:
	nocmodl $*
	gcc $(CFLAGS) -c $*.c
	rm $*.c

.Mod.o:
	nocmodl $*
	gcc $(CFLAGS) -c $*.c
	rm $*.c

.MoD.o:
	nocmodl $*
	gcc $(CFLAGS) -c $*.c
	rm $*.c

.MOd.o:
	nocmodl $*
	gcc $(CFLAGS) -c $*.c
	rm $*.c

.MOD.o:
	nocmodl $*
	gcc $(CFLAGS) -c $*.c
	rm $*.c


mod_func.o: mod_func.c
	gcc $(CFLAGS) -c $*.c

#nrnmech.dll: mod_func.o $(MODOBJFILES)
#	ld -d -S -x -r -o nrnmech.dll mod_func.o $(MODOBJFILES) -L$N/lib -lscpmt

nrnmech.dll: mod_func.o $(MODOBJFILES)
	gcc -shared -o nrnmech.dll mod_func.o $(MODOBJFILES) \
  -L$N/bin -lhocmodule \
  -L$N/gcclib


#nm nrnmech.dll | mkdll -u > nrnmech.h #will give a list of neuron.exe names
#required by nrnmech.dll

mod_func.o $(MODOBJFILES): $(N)/bin/nrniv.exe


