#!sh
set -e

NEURONHOME=$N
M=$N/lib

# get the last argument
for last; do true; done

OUTPUTDIR=.
# if the last argument begins with --NEURON_HOME=
if [[ $last == "--OUTPUTDIR="* ]] ; then
OUTPUTDIR=${last#"--OUTPUTDIR="}
# remove the last argument
set -- "${@:1:$(($#-1))}"
fi


if test $# -gt 0
then
	files=$*
else
	files=`ls -1 *.[mM][oO][dD]`
fi
prefixes=`echo $files | sed 's/\.[mM][oO][dD]//g'`

#echo "COMPLETE FILE NAMES"
#echo $files
#echo "TRUNCATED FILE NAMES"
#echo $prefixes

MODOBJS=
if [ `echo "\n"` ]
then
	newline="\n"
else
	newline="\\\\n"
fi

echo '#include <stdio.h>
#include "hocdec.h"
#define IMPORT extern __declspec(dllimport)
IMPORT int nrnmpi_myid, nrn_nobanner_;
' > mod_func.c

for i in $prefixes
do
	echo extern void _"$i"_reg"();" >> mod_func.c
done

echo '
void modl_reg(){
	//nrn_mswindll_stdio(stdin, stdout, stderr);
    if (!nrn_nobanner_) if (nrnmpi_myid < 1) {
	fprintf(stderr, "Additional mechanisms from files'$newline'");
' >> mod_func.c

for i in $files
do
	echo 'fprintf(stderr," '$i'");' >>mod_func.c
done
echo 'fprintf(stderr, "'$newline'");
    }' >>mod_func.c

echo -n 'MODOBJFILES=' >$$.tmp
for i in $prefixes
do
	echo _"$i"_reg"();" >> mod_func.c
	echo -n " $i.o" >> $$.tmp
done
echo "}" >> mod_func.c

#echo ' "' >> $$.tmp
# cat $$.tmp
make -f $M/mknrndll.mak "`cat $$.tmp`" nrnmech.dll TARGET="$$.nrnmech.dll"
mv $$.nrnmech.dll "$OUTPUTDIR/nrnmech.dll"
rm $$.tmp

