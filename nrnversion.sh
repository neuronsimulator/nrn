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
type=`sed -n '/SVN_BRANCH/s/.*\(".*"\).*/\1/p' $VERHFILE | sed '
s/""/alpha/
s/"Release.*"/rel/
s/"\(.*\)"/\1/
'`
base=`sed -n '/SVN_BASE_CHANGESET/s/.*"\([0-9]*\)".*/\1/p' $VERHFILE`
commit=`sed -n '/SVN_TREE_CHANGE/s/.*"\(.*\)".*/\1/p' $VERHFILE`

case $1 in
	2) echo $major.$minor.$type-$commit ;;
	3) echo ${major}${minor} ;;
	"major") echo $major ;;
	"minor") echo $minor ;;
	"type") echo $type ;;
	"commit") echo ${type}${commit} ;;
	"base") echo $base ;;
	*) echo $major.$minor ;;
esac

