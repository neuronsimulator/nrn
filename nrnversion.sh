#!/bin/sh
# extract commit version info from src/nrnoc/nrnversion.h
# and PACKAGE_VERSION from configure.ac

if test "$NSRC" = "" ; then
	NSRC=$HOME/neuron/nrn
fi

if test -f $NSRC/src/nrnoc/nrnversion.h ; then
	VERHFILE=`cat $NSRC/src/nrnoc/nrnversion.h`
elif test -d $NSRC/.git ; then
	VERHFILE=`sh $NSRC/git2nrnversion_h.sh`
elif test -d $NSRC/.hg ; then
	VERHFILE=`sh $NSRC/hg2nrnversion_h.sh`
fi

ver=`sed -n '/^AC_INIT/s/^.*nrn\],\[\(.*\)\].*/\1/p' < $NSRC/configure.ac`

HG=HG
global=`echo "$VERHFILE" | sed -n '/HG_CHANGESET/s/.*"\(.*\)".*/\1/p'`
if test "$global" = "" ; then
  global=`echo "$VERHFILE" | sed -n '/GIT_CHANGESET/s/.*"\(.*\)".*/\1/p'`
  HG=GIT
fi

type=`echo "$VERHFILE" | sed -n /${HG}'_BRANCH/s/.*\(".*"\).*/\1/p' | sed '
s/"trunk"/alpha/
s/"Release.*"/rel/
s/"\(.*\)"/\1/
'`
local=`echo "$VERHFILE" | sed -n /${HG}'_LOCAL/s/.*"\(.*\)".*/\1/p'`
base=`echo $global | sed 's/\+//'`
commit=`echo "$VERHFILE" | sed -n /${HG}'_LOCAL/s/.*"\(.*\)".*/\1/p'`
date=`echo "$VERHFILE" | sed -n /${HG}'_DATE/s/.*"\(.*\)".*/\1/p'`

case $1 in
	2) echo $ver.$type-$commit ;;
	3) echo ${ver} ;;
	"type") echo $type ;;
	"commit") echo ${type}${commit} ;;
	"local") echo $local ;;
	"base") echo $base ;;
	"global") echo $global ;;
	"date") echo $date ;;
	*) echo $ver ;;
esac

