#!/bin/sh
# extract major minor and commit version number from src/nrnoc/nrnversion.h
if test "$NSRC" = "" ; then
	NSRC=$HOME/neuron/nrn
fi

if test -f $NSRC/src/nrnoc/nrnversion.h ; then
	VERHFILE=`cat $NSRC/src/nrnoc/nrnversion.h`
else
	VERHFILE=`sh $NSRC/hg2nrnversion_h.sh`
fi

major=`echo "$VERHFILE" | sed -n '/NRN_MAJOR_VERSION/s/.*"\([0-9]*\)".*/\1/p'`
minor=`echo "$VERHFILE" | sed -n '/NRN_MINOR_VERSION/s/.*"\([0-9]*\)".*/\1/p'`
type=`echo "$VERHFILE" | sed -n '/HG_BRANCH/s/.*\(".*"\).*/\1/p' | sed '
s/"trunk"/alpha/
s/"Release.*"/rel/
s/"\(.*\)"/\1/
'`
local=`echo "$VERHFILE" | sed -n '/HG_LOCAL/s/.*"\(.*\)".*/\1/p'`
global=`echo "$VERHFILE" | sed -n '/HG_CHANGESET/s/.*"\(.*\)".*/\1/p'`
base=`echo $global | sed 's/\+//'`
commit=`echo "$VERHFILE" | sed -n '/HG_LOCAL/s/.*"\(.*\)".*/\1/p'`

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

