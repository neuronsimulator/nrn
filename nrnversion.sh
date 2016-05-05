#!/bin/sh
# extract commit version info from src/nrnoc/nrnversion.h
# and PACKAGE_VERSION from configure.ac

if test "$NSRC" = "" ; then
	NSRC=$HOME/neuron/nrn
fi

if test -f $NSRC/src/nrnoc/nrnversion.h ; then
	VERHFILE=`cat $NSRC/src/nrnoc/nrnversion.h`
else
	VERHFILE=`sh $NSRC/hg2nrnversion_h.sh`
fi

ver=`sed -n '/^AC_INIT/s/^.*nrn\],\[\(.*\)\].*/\1/p' < configure.ac`

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
	2) echo $ver.$type-$commit ;;
	3) echo ${ver} ;;
	"type") echo $type ;;
	"commit") echo ${type}${commit} ;;
	"local") echo $local ;;
	"base") echo $base ;;
	"global") echo $global ;;
	*) echo $ver ;;
esac

