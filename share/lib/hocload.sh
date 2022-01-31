#!/usr/bin/env bash

# used by the libload command in hoc to find an instance of a procedure,
#	function, or template

# uncomment following line for use with DOS
#NEURONHOME=`d2uenv NEURONHOME`

if [ $TEMP ] ; then
	tmpdir=$TEMP
else
	tmpdir="/tmp"
fi
curdir=`pwd`
names=$tmpdir/oc"$3".hl

if [ ! -f $names ] ; then

paths=". $HOC_LIBRARY_PATH $NEURONHOME/lib/hoc"

spaths=`echo "$paths" | sed 's/:/ /g'`

for p in $spaths
do
#for DOS comment out the egrep and uncomment the grep line
	egrep '^func|^proc|^begintemplate' $p/*.oc $p/*.hoc >> $names 2>/dev/null
#	grep -s '^func|^proc|^begintemplate' $p/\*.oc $p/\*.hoc >> $names
done

fi
if [ "$1" = "begintemplate" ] ; then
file=`sed -n "/:$1 $2\$/ {
	s/:$1 $2.*//p
	q
	}" $names`
else
file=`sed -n "/:$1 $2 *()/ {
	s/:$1 $2.*//p
	q
	}" $names`
fi
if [ -z "$file" ] ; then
	exit 1
fi
echo $file
exit 0


