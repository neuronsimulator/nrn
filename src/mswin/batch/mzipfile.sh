cd c:/nrn
rm -f nrnfiles.zip
zip -r nrnfiles.zip lib/*.mak lib/hoc lib/nrn.def \
lib/prologue.id iv lib/*.sh lib/psfilt.sed \
lib/*.cm* lib/cleanup lib/nrnunits.lib \
bin/*.ico examples demo \
src/nrnoc/*.mod lib/helpdict \
src/oc/*.h src/nrnoc/*.h src/scopmath/*.h \
classes/*.jar

zip -d nrnfiles.zip \*.o \*.c
