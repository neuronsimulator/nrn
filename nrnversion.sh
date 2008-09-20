#!/bin/sh
# extract major minor and commit version number from src/nrnoc/nrnversion.h
if test "$NSRC" = "" ; then
	NSRC=$HOME/neuron/nrn
fi
if test ! -f $NSRC/src/nrnoc/nrnversion.c ; then
	echo "$0: Can not find $NSRC/src/nrnoc/nrnversion.c"
	exit 1
fi
VERFILE=$NSRC/src/nrnoc/nrnversion.c
VERHFILE=$NSRC/src/nrnoc/nrnversion.h
major=`sed -n '/NRN_MAJOR_VERSION/s/.*"\([0-9]*\)".*/\1/p' $VERHFILE`
minor=`sed -n '/NRN_MINOR_VERSION/s/.*"\([0-9]*\)".*/\1/p' $VERHFILE`
type=`sed -n '/HG_BRANCH/s/.*\(".*"\).*/\1/p' $VERHFILE | sed '
s/"trunk"/alpha/
s/"Release.*"/rel/
s/"\(.*\)"/\1/
'`
local=`sed -n '/HG_LOCAL/s/.*"\(.*\)".*/\1/p' $VERHFILE`
global=`sed -n '/HG_CHANGESET/s/.*"\(.*\)".*/\1/p' $VERHFILE`
base=`echo $global | sed 's/\+//'`
commit=`sed -n '/HG_LOCAL/s/.*"\(.*\)".*/\1/p' $VERHFILE`

case $1 in
	2) echo $major.$minor.$type-$commit ;;
	3) echo ${major}${minor} ;;
	"major") echo $major ;;
	"minor") echo $minor ;;
	"type") echo $type ;;
	"commit") echo ${type}${commit} ;;
	"local") echo $local ;;
	"base") echo $base ;;
	"global") echo $global ;;
	*) echo $major.$minor ;;
esac

